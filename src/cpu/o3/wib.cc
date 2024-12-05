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
      numThreads(params.numThreads),
      stats(_cpu)
{
    // TODO: pwibably want to give each thread numEntries / numThreads entires into WIB

    //Set Max Entries to Total ROB Capacity
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        maxEntries[tid] = numEntries;
    }

    for (ThreadID tid = numThreads; tid < MaxThreads; tid++) {
        maxEntries[tid] = 0;
    }

    for (ThreadID tid = 0; tid  < MaxThreads; tid++) {
        // set size to rows: numEntries/4, columns: numEntries
        bitMatrix[tid].resize((int)(numEntries*0.25));
        for (auto& row : bitMatrix[tid]) {
            row.resize(numEntries, 0); // Initialize new bits to 0
        }
    }

    resetState();
}

void
WIB::resetState()
{
    for (ThreadID tid = 0; tid  < MaxThreads; tid++) {
        threadEntries[tid] = 0;
        squashIt[tid] = instList[tid].end();
        squashedSeqNum[tid] = 0;
        doneSquashing[tid] = true;

        for (auto& row : bitMatrix[tid]) {
            std::fill(row.begin(), row.end(), false); // Set all bits in each row to false (0)
        }
    }
    

    numInstsInWIB = 0;
}

std::string
WIB::name() const
{
    return cpu->name() + ".wib";
}

void
WIB::setActiveThreads(std::list<ThreadID> *at_ptr)
{
    DPRINTF(WIB, "Setting active threads list pointer.\n");
    activeThreads = at_ptr;
}

void
WIB::drainSanityCheck() const
{
    for (ThreadID tid = 0; tid  < numThreads; tid++)
        assert(instList[tid].empty());
    assert(isEmpty());
}

void
WIB::takeOverFrom()
{
    resetState();
}

void
WIB::resetEntries()
{
}

int
WIB::entryAmount(ThreadID num_threads)
{
    if (wibPolicy == SMTQueuePolicy::Partitioned) {
        return numEntries / num_threads;
    } else {
        return 0;
    }
}

int
WIB::countInsts()
{
    int total = 0;

    for (ThreadID tid = 0; tid < numThreads; tid++)
        total += countInsts(tid);

    return total;
}

size_t
WIB::countInsts(ThreadID tid)
{
    return instList[tid].size();
}

void
WIB::insertInst(const DynInstPtr &inst)
{
        assert(inst);

    // stats.writes++;

    DPRINTF(WIB, "Adding inst PC %s to the WIB.\n", inst->pcState());

    assert(numInstsInWIB != numEntries);

    ThreadID tid = inst->threadNumber;

    instList[tid].push_back(inst);

    //Set Up head iterator if this is the 1st instruction in the ROB
    if (numInstsInWIB == 0) {
        head = instList[tid].begin();
        assert((*head) == inst);
    }

    //Must Decrement for iterator to actually be valid  since __.end()
    //actually points to 1 after the last inst
    tail = instList[tid].end();
    tail--;

    inst->setInWIB();

    ++numInstsInWIB;
    ++threadEntries[tid];

    assert((*tail) == inst);

    DPRINTF(WIB, "[tid:%i] Now has %d instructions.\n", tid,
            threadEntries[tid]);

    // add bitvector for dependence stuff
}

unsigned
WIB::numFreeEntries()
{
    return numEntries - numInstsInWIB;
}

unsigned
WIB::numFreeEntries(ThreadID tid)
{
    return maxEntries[tid] - threadEntries[tid];
}

void
WIB::doSquash(ThreadID tid)
{
}

void
WIB::squash(InstSeqNum squash_num, ThreadID tid)
{
  //calls dosquash
}

void
WIB::retireHead(ThreadID tid)
{
    // stats.writes++;

    assert(numInstsInWIB > 0);

    // Get the head ROB instruction by copying it and remove it from the list
    InstIt head_it = instList[tid].begin();

    DynInstPtr head_inst = std::move(*head_it);
    instList[tid].erase(head_it);

    assert(head_inst->readyToCommit());

    DPRINTF(WIB, "[tid:%i] Retiring head instruction, "
            "instruction PC %s, [sn:%llu]\n", tid, head_inst->pcState(),
            head_inst->seqNum);

    --numInstsInWIB;
    --threadEntries[tid];

    head_inst->clearInROB();
    head_inst->setCommitted();

    //Update "Global" Head of WIB
    updateHead();

    // @todo: A special case is needed if the instruction being
    // retired is the only instruction in the WIB; otherwise the tail
    // iterator will become invalidated.

    // since a cpu function assuming ROB call for this will take care of it
    // cpu->removeFrontInst(head_inst);
}

void
WIB::updateHead()
{
    InstSeqNum lowest_num = 0;
    bool first_valid = true;

    // @todo: set ActiveThreads through ROB or CPU
    std::list<ThreadID>::iterator threads = activeThreads->begin();
    std::list<ThreadID>::iterator end = activeThreads->end();

    while (threads != end) {
        ThreadID tid = *threads++;

        if (instList[tid].empty())
            continue;

        if (first_valid) {
            head = instList[tid].begin();
            lowest_num = (*head)->seqNum;
            first_valid = false;
            continue;
        }

        InstIt head_thread = instList[tid].begin();

        DynInstPtr head_inst = (*head_thread);

        assert(head_inst != 0);

        if (head_inst->seqNum < lowest_num) {
            head = head_thread;
            lowest_num = head_inst->seqNum;
        }
    }

    if (first_valid) {
        head = instList[0].end();
    }

}

} // namespace o3
} // namespace gem5
