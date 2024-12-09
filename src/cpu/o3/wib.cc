#include "cpu/o3/wib.hh"

#include <list>

#include "base/logging.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/limits.hh"
#include "params/BaseO3CPU.hh"

namespace gem5
{

namespace o3
{

WIB::WIB(CPU *_cpu, const BaseO3CPUParams &params)
    : cpu(_cpu),
      numEntries(params.numWIBEntries), //need to add to params
      squashWidth(1),
      stats(_cpu)
{
    numLoads = (int)(numEntries * 0.25);

    headInst = 0;
    tailInst = 1;
    instList.resize(numEntries);
    for (size_t i = 0; i < numEntries; ++i) {
        instList[i] = nullptr;
    }
    loadList.resize(numLoads);
    for (size_t i = 0; i < numLoads; ++i) {
        loadList[i] = nullptr;
    }

    // set size to rows: numLoads, columns: numEntries
    bitMatrix.resize(numLoads);
    for (auto& row : bitMatrix) {
        row.resize(numEntries, false); // Initialize new bits to false
    }

    resetState();
}

void
WIB::resetState()
{
    squashedSeqNum = 0;
    doneSquashing = true;

    for (auto& row : bitMatrix) {
        std::fill(row.begin(), row.end(), false); // Set all bits in each row to false (0)
    }

    numInstsInWIB = 0;
}

std::string
WIB::name() const
{
    return cpu->name() + ".wib";
}

void
WIB::resetEntries()
{
}

size_t
WIB::addColumn(const DynInstPtr &inst)
{
    assert(inst);
    
    size_t colIdx = 0;

    for (int i = 0; i < numLoads; i++) {
      if (!loadList[i]) {
        colIdx = i;
        loadList[colIdx] = inst;
        inst->setWaitColumn(colIdx);
        break;
      }
    }

    // reset column
    for (auto& row: bitMatrix) {
      row[colIdx] = false;
    }
    
    // return the new column idx for the dependent instructions
    return colIdx;
}

void
WIB::removeColumn(const size_t colIdx)
{
    // process the columns rows to send dependendent insts back to issue queue
    loadList[colIdx] = nullptr;
    for (auto& row : bitMatrix) {
        if (row[colIdx]) {
            // TODO: send instruction at current row to be dispatched into IQ
            // instList[colIdx] is inst that needs dispatched
            // MAKE SURE TO CLEAR WAITBIT OF DEST REG WHEN SENT TO DISPATCH
            row[colIdx] = false;
        }
    }
}

void
WIB::squashColumn(const size_t colIdx)
{
    loadList[colIdx] = nullptr;
    for (auto& row : bitMatrix) {
        row[colIdx] = false;
    }
}

void
WIB::squashRow(const size_t rowIdx)
{
    instList[rowIdx] = nullptr;
    std::fill(bitMatrix[rowIdx].begin(), bitMatrix[rowIdx].end(), false);
}

void
WIB::insertInst(const DynInstPtr &inst)
{
    assert(inst);

    tailInst = (tailInst + 1) % numEntries;
    instList[tailInst] = inst;

    ++numInstsInWIB;
}

void
WIB::tagDependentInst(const DynInstPtr &inst, const size_t colIdx) {
    assert(inst);
    for (int i = 0; i < numEntries; i++) {
        if (instList[i]->seqNum == inst->seqNum) {
            bitMatrix[i][colIdx] = true;
            break;
        }
    }
}

unsigned
WIB::numFreeEntries()
{
    return numEntries - numInstsInWIB;
}

void
WIB::doSquash(InstSeqNum squash_num)
{
    // let ROB handle the instruction state on squashes, just need
    // to handle local head/tail index here, and update bitMatrix
    
    size_t currentIdx = tailInst;
    while (instList[currentIdx]->seqNum < squashedSeqNum) {
      squashRow(currentIdx);
      currentIdx = (currentIdx - 1) % numEntries;
    }
    doneSquashing = true;
}

void
WIB::squash(InstSeqNum squash_num)
{
    doneSquashing = false;
    if (numInstsInWIB != 0) {
      doSquash(squash_num);
    }
}

void
WIB::retireHead()
{
    assert(numInstsInWIB > 0);

    // clear head and update head
    instList[headInst] = nullptr;
    headInst = (headInst + 1) % numEntries;

    --numInstsInWIB;
}

} // namespace o3
} // namespace gem5
