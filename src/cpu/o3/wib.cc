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

WIB::WIB(CPU *_cpu, IEW *iewStage,
        const BaseO3CPUParams &params)
    : cpu(_cpu),
      iewStage(iewStage),
      numEntries(params.numWIBEntries), //need to add to params
      squashWidth(1)
      // stats(_cpu)
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

size_t
WIB::addColumn(const DynInstPtr &inst)
{
    assert(inst);
    
    size_t colIdx = 0;

    for (int i = 0; i < numLoads; i++) {
      if (!loadList[i]) {
        colIdx = i;
        loadList[colIdx] = inst;
        inst->renamedDestIdx(0)->setWaitColumn(colIdx);
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
    bool add_to_iq = false;
    // process the columns rows to send dependendent insts back to issue queue
    loadList[colIdx] = nullptr;
    for (auto& row : bitMatrix) {
        if (row[colIdx]) {
            // TODO: send instruction at current row to be dispatched into IQ
            // instList[colIdx] is inst that needs dispatched
            // MAKE SURE TO CLEAR WAITBIT OF DEST REG WHEN SENT TO DISPATCH
            // Assuming all inst sent here are load insts
            DynInstPtr inst = instList[colIdx];

            // Make sure there's a valid instruction there.
            assert(inst);

            // Be sure to mark these instructions as ready so that the
            // commit stage can go ahead and execute them, and mark
            // them as issued so the IQ doesn't reprocess them.
            
            // FIXME: check for toRename
            // Check for squashed instructions.
            if (inst->isSquashed()) {
                // DPRINTF(WIB, "WIB: Squashed instruction encountered, "
                //         "not adding to IQ.\n");

                // ++iewStats.dispSquashedInsts;

                //Tell Rename That An Instruction has been processed
                // if (inst->isLoad()) {
                //     toRename->iewInfo[0].dispatchedToLQ++;
                // }
                // if (inst->isStore() || inst->isAtomic()) {
                //     toRename->iewInfo[0].dispatchedToSQ++;
                // }

                // toRename->iewInfo[0].dispatched++;

                continue;
            }

            if (iewStage->instQueue.isFull(0)) {
                // Call function to start blocking.
                // block(tid);

                // Set unblock to false. Special case where we are using
                // skidbuffer (unblocking) instructions but then we still
                // get full in the IQ.
                // toRename->iewUnblock[0] = false;

                break;
            }

            if ((inst->isAtomic() && iewStage->ldstQueue.sqFull(0)) ||
                (inst->isLoad() && iewStage->ldstQueue.lqFull(0)) ||
                (inst->isStore() && iewStage->ldstQueue.sqFull(0))) {
                // DPRINTF(WIB, "Issue: LSQ has become full.\n",
                //         inst->isLoad() ? "LQ" : "SQ");

                // Call function to start blocking.
                // block(0);

                // Set unblock to false. Special case where we are using
                // skidbuffer (unblocking) instructions but then we still
                // get full in the IQ.
                // toRename->iewUnblock[0] = false;

                break;
            }

            // hardware transactional memory
            // CPU needs to track transactional state in program order.

            // Otherwise issue the instruction just fine.
            if (inst->isAtomic()) {
                // DPRINTF(WIB, "WIB: Memory instruction "
                //         "encountered, adding to LSQ.\n");

                iewStage->ldstQueue.insertStore(inst);

                // ++iewStats.dispStoreInsts;

                // AMOs need to be set as "canCommit()"
                // so that commit can process them when they reach the
                // head of commit.
                inst->setCanCommit();
                iewStage->instQueue.insertNonSpec(inst);
                add_to_iq = false;

                // ++iewStats.dispNonSpecInsts;

                // toRename->iewInfo[0].dispatchedToSQ++;

            } else if (inst->isLoad()) {
                // DPRINTF(WIB, "WIB: Memory instruction "
                //         "encountered, adding to LSQ.\n");

                // Reserve a spot in the load store queue for this
                // memory access.
                iewStage->ldstQueue.insertLoad(inst);

                // ++iewStats.dispLoadInsts;

                add_to_iq = true;

                // toRename->iewInfo[0].dispatchedToLQ++;

            } else if (inst->isStore()) {
                // DPRINTF(WIB, "WIB: Memory instruction "
                //         "encountered, adding to LSQ.\n");

                iewStage->ldstQueue.insertStore(inst);

                // ++iewStats.dispStoreInsts;

                if (inst->isStoreConditional()) {
                    // Store conditionals need to be set as "canCommit()"
                    // so that commit can process them when they reach the
                    // head of commit.
                    // @todo: This is somewhat specific to Alpha.
                    inst->setCanCommit();
                    iewStage->instQueue.insertNonSpec(inst);
                    add_to_iq = false;

                    // ++iewStats.dispNonSpecInsts;
                } else {
                    add_to_iq = true;
                }

                // toRename->iewInfo[0].dispatchedToSQ++;

            } else if (inst->isReadBarrier() || inst->isWriteBarrier()) {
                // Same as non-speculative stores.
                inst->setCanCommit();
                iewStage->instQueue.insertBarrier(inst);
                add_to_iq = false;

            } else if (inst->isNop()) {
                // DPRINTF(WIB, "WIB: Nop instruction encountered, "
                //         "skipping.\n");

                inst->setIssued();
                inst->setExecuted();
                inst->setCanCommit();

                iewStage->instQueue.recordProducer(inst);

                cpu->executeStats[0]->numNop++;

                add_to_iq = false;
            } else {
                assert(!inst->isExecuted());
                add_to_iq = true;
            }

            if (add_to_iq && inst->isNonSpeculative()) {
                // DPRINTF(IEW, "WIB: Nonspeculative instruction "
                //         "encountered, skipping.\n");

                // Same as non-speculative stores.
                inst->setCanCommit();

                // Specifically insert it as nonspeculative.
                iewStage->instQueue.insertNonSpec(inst);

                // ++iewStats.dispNonSpecInsts;

                add_to_iq = false;
            }

            // If the instruction queue is not full, then add the
            // instruction.
            if (add_to_iq) {
                iewStage->instQueue.insert(inst);
                inst->renamedDestIdx(0)->setWaitBit(false);
                row[colIdx] = false;
            }
            // toRename->iewInfo[0].dispatched++;
            // ++iewStats.dispatchedInsts;
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
