#include "cpu/o3/wib.hh"

#include <list>
#include <queue>

#include "base/logging.hh"
#include "cpu/o3/dyn_inst.hh"
#include "cpu/o3/limits.hh"
#include "params/BaseO3CPU.hh"

namespace gem5
{

namespace o3
{

WIB::WIB(CPU *_cpu, IEW *iewStage,
        const BaseO3CPUParams &params)
    : cpu(_cpu),
      iewStage(iewStage),
      numEntries(params.numWIBEntries), //need to add to params
      squashWidth(1)
      // stats(_cpu)
{
    // numLoads = (int)(numEntries * 0.25);
    numLoads = numEntries;

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
    bitMatrix.resize(numEntries);
    for (auto& row : bitMatrix) {
        row.resize(numLoads, false); // Initialize new bits to false
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
WIB::wibEmpty()
{
    // bool same_ptr = false;
    // if (headInst == tailInst) {
    //     same_ptr = true;
    //     std::cout << "should be emtpy, head and tail are same" << std::endl;
    // }
    int load_count = 0;
    int inst_count = 0;
    int inst_on_loads = 0;
    int inst_on_inst = 0;
    for (int i = 0; i < numLoads; i++) {
        if (!loadList[i]) {
          load_count++;
        }
        if (!instList[i]) {
          inst_count++;
        }
        // else if (instList[i]->renamedDestIdx(0)->isWaitBit()) {
        //   inst_on_inst++; 
        // }
        else {
          int total_src_regs = instList[i]->numSrcRegs();
          
          for (int src_reg_idx = 0;
               src_reg_idx < total_src_regs;
               src_reg_idx++)
          {
            if (instList[i]->renamedSrcIdx(src_reg_idx)->isWaitBit()) {
              inst_on_loads++;
            }
          }
        }
    }
    // if (load_count == numLoads) {std::cout << "empty loads" << std::endl;}
    // if (load_count == numLoads & (inst_on_inst > 0 | inst_on_loads > 0)) {
    //     std::cout << numInstsInWIB << "instructions in WIB" << std::endl;
    //     std::cout << "but no loads" << std::endl;
    //     std::cout << "dispatch buffer: " << wibBuffer.size() << std::endl;
    //     std::cout << "writes since squash: " << written_since_squash << std::endl;
    // }
    // if (inst_count == numLoads) {std::cout << "NO INSTRUCTIONS IN WIB" << std::endl;}
    // if (same_ptr & !empty) {std::cout << "ERROR: FIX POINTERS" << std::endl;}
    // if (empty) {std::cout << "WIB IS EMTPY" << std::endl;}
}

void
WIB::wibInsert(const DynInstPtr &inst)
{
    wibBuffer.push(inst);
    // std::cout << "moving instruction that depends on col " << inst->renamedDestIdx(0)->getWaitColumn() << std::endl;
}

size_t
WIB::addColumn(const DynInstPtr &inst)
{
    assert(inst);
    
    size_t colIdx = 0;
    bool found_column = false;

    for (int i = 0; i < numLoads; i++) {
        if (!loadList[i]) {
            found_column = true;
            colIdx = i;
            // std::cout << "adding load to col " << colIdx << std::endl;
            loadList[colIdx] = inst;
            inst->renamedDestIdx(0)->setWaitColumn(colIdx);
            break;
        }
    }

    // if (!found_column) {std::cout << "NO SPACE IN WIB FOR LOAD" << std::endl; }

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
   //  std::cout << "removing column " << colIdx << std::endl;
    // process the columns rows to send dependendent insts back to issue queue
    loadList[colIdx] = nullptr;
    bool any_dispatched = false;
    for (auto& row : bitMatrix) {
        if (row[colIdx]) {
            any_dispatched = true;

            // send instruction at current row to be dispatched into IQ
            // Assuming all inst sent here are load insts
            DynInstPtr inst = instList[colIdx];

            // Make sure there's a valid instruction there.
            assert(inst);

            // clear the waitBit of this instruction
            inst->renamedDestIdx(0)->setWaitBit(false);
            wibInsert(inst);
        }
    }
}

void
WIB::squashColumn(const size_t colIdx)
{
    // std::cout << "squashing column " << colIdx << std::endl;
    // colIdx -> entryIdx
    loadList[colIdx] = nullptr;
    for (auto& row : bitMatrix) {
        row[colIdx] = false;
    }
}

void
WIB::squashRow(const size_t rowIdx)
{
    // std::cout << "squshing row " << rowIdx << std::endl;
    // rowIdx -> loadIdx
    DynInstPtr inst = instList[rowIdx];

    // load instruction with a waitBit dest reg getting squashed
    // if (inst->isLoad() && inst->renamedDestIdx(0)->isWaitBit()) {
        // Update our WIB to set only the loadList to nullptr at
        // the index of the load getting squashed
        // squashColumn(inst->renamedDestIdx(0)->getWaitColumn());
        // for (int colIdx = 0; colIdx < numEntries; colIdx++) {
        //     if (bitMatrix[rowIdx][colIdx]) {
        //         squashColumn(colIdx);
        //     }
        // }
        // Clear the waitBit 
        // inst->renamedDestIdx(0)->setWaitBit(false);
    // }

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
    // std::cout << "tagging dependent insts" << std::endl;
    assert(inst);
    // numLoads < numEntries
    if (colIdx == 0){
        written_since_squash++;
    }
    bool found = false;
    for (int i = 0; i < numEntries; i++) {
        if (i >= bitMatrix.size()) {
            // std::cout << "Error: Row index i=" << i << " out of bounds for bitMatrix." << std::endl;
            break;
        }
        if (colIdx >= bitMatrix[i].size()) {
            // std::cout << "Error: Column index colIdx=" << colIdx 
            //           << " out of bounds for bitMatrix[" << i << "]." << std::endl;
            break;
        }
        if (!instList[i]) {
            // std::cout << "Warning: instList[" << i << "] is null." << std::endl;
            continue;
        }
        if (instList[i]->seqNum == inst->seqNum) {
            // std::cout << inst->seqNum << std::endl;
            found = true;
            // std::cout << "writing 1 to col " << colIdx << " for current instruction" << std::endl;
            // std::cout << "Accessing bitMatrix[" << i << "][" << colIdx << "]" << std::endl;
            bitMatrix[i][colIdx] = true;  // Suspected line
            // std::cout << "Value set successfully at bitMatrix[" << i << "][" << colIdx << "]" << std::endl;
            break;
        }
    }
    // std::cout << "Column " << colIdx << ": ";
    //   for (size_t i = 0; i < bitMatrix.size(); ++i) {
    //       if (colIdx < bitMatrix[i].size()) {
    //           std::cout << bitMatrix[i][colIdx];
    //       } else {
    //           std::cout << "OutOfBounds ";
    //       }
    //   }
    //   std::cout << std::endl;   
    // if (!found) { std::cout << "INSTRUCTION NOT IN WIB" << std::endl;}
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
        // if (instList[currentIdx]->isLoad() & instList[currentIdx]->renamedDestIdx(0)->isWaitBit())
        // {
        //     squashColumn(instList[currentIdx]->renamedDestIdx(0)->getWaitColumn());
        // }
        squashRow(currentIdx);
        currentIdx = (currentIdx - 1) % numEntries;
    }
    doneSquashing = true;
}

void
WIB::squash(InstSeqNum squash_num)
{
    written_since_squash = 0;
    doneSquashing = false;
    if (numInstsInWIB != 0) {
        doSquash(squash_num);
    }
}

void
WIB::retireHead()
{
    // std::cout << "retiring head" << std::endl;
    assert(numInstsInWIB > 0);

    // clear head and update head
    instList[headInst] = nullptr;
    headInst = (headInst + 1) % numEntries;

    --numInstsInWIB;
}

} // namespace o3
} // namespace gem5
