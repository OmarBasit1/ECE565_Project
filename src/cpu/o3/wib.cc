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
    //Set Max Entries to Total ROB Capacity
    for (ThreadID tid = 0; tid < numThreads; tid++) {
        maxEntries[tid] = numEntries;
    }

    for (ThreadID tid = numThreads; tid < MaxThreads; tid++) {
        maxEntries[tid] = 0;
    }

    numLoads = (int)(numEntires * 0.25);
    for (ThreadID tid = 0; tid  < MaxThreads; tid++) {
        head[tid] = 0;
        tail[tid] = 0;
        // set size to rows: numLoads, columns: numEntries
        bitMatrix[tid].resize(numLoads);
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

std::vector<bool>*
WIB::addColumn(const DynInstPtr &inst)
{
    assert(inst);
    ThreadID tid = inst->threadNumber;

    size_t newColIdx = tail[tid];

    tail[tid] = (tail[tid] + 1) % numLoads;

    if (tail[tid] == head[tid]) {
      std::cout << "WIB OVERFLOW: numLoads too small in WIB" << std::endl;
      return nullptr;
    }

    // clear column
    for (auto& row: bitMatrix[tid]) {
      row[newColIdx] = false;
    }
    
    // return pointer to the new column
    return &bitMatrix[tid][newColIdx]
}

void
WIB::removeColumn(const DynInstPtr &inst)
{
    size_t oldColIdx = head[tid];
    
    // process the columns rows to send dependencies back to issue queue
    for (auto& row : bitMatrix[tid]) {
        if (bitMatrix[tid][row][oldColIdx]) {
            // send instruction at current row to issue queue
        }
        row[oldColIdx] = false;
    }
    
    head[tid] = (head[tid] + 1) % numLoads;
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
    // stats.writes++;
    DPRINTF(WIB, "[tid:%i] Squashing instructions until [sn:%llu].\n",
            tid, squashedSeqNum[tid]);

    assert(squashIt[tid] != instList[tid].end());

    if ((*squashIt[tid])->seqNum < squashedSeqNum[tid]) {
        DPRINTF(WIB, "[tid:%i] Done squashing instructions.\n",
                tid);

        squashIt[tid] = instList[tid].end();

        doneSquashing[tid] = true;
        return;
    }

    bool wibTailUpdate = false;

    unsigned int numInstsToSquash = squashWidth;

    // If the CPU is exiting, squash all of the instructions
    // it is told to, even if that exceeds the squashWidth.
    // Set the number to the number of entries (the max).
    if (cpu->isThreadExiting(tid))
    {
        numInstsToSquash = numEntries;
    }

    for (int numSquashed = 0;
         numSquashed < numInstsToSquash &&
         squashIt[tid] != instList[tid].end() &&
         (*squashIt[tid])->seqNum > squashedSeqNum[tid];
         ++numSquashed)
    {
        DPRINTF(WIB, "[tid:%i] Squashing instruction PC %s, seq num %i.\n",
                (*squashIt[tid])->threadNumber,
                (*squashIt[tid])->pcState(),
                (*squashIt[tid])->seqNum);

        // Mark the instruction as squashed, and ready to commit so that
        // it can drain out of the pipeline.
        (*squashIt[tid])->setSquashed();

        // (*squashIt[tid])->setCanCommit();


        if (squashIt[tid] == instList[tid].begin()) {
            DPRINTF(WIB, "Reached head of instruction list while "
                    "squashing.\n");

            squashIt[tid] = instList[tid].end();

            doneSquashing[tid] = true;

            return;
        }

        InstIt tail_thread = instList[tid].end();
        tail_thread--;

        if ((*squashIt[tid]) == (*tail_thread))
            robTailUpdate = true;

        squashIt[tid]--;
    }


    // Check if ROB is done squashing.
    if ((*squashIt[tid])->seqNum <= squashedSeqNum[tid]) {
        DPRINTF(WIB, "[tid:%i] Done squashing instructions.\n",
                tid);

        squashIt[tid] = instList[tid].end();

        doneSquashing[tid] = true;
    }

    if (robTailUpdate) {
        updateTail();
    }
}

void
WIB::squash(InstSeqNum squash_num, ThreadID tid)
{
    if (isEmpty(tid)) {
        DPRINTF(WIB, "Does not need to squash due to being empty "
                "[sn:%llu]\n",
                squash_num);

        return;
    }
    DPRINTF(WIB, "Starting to squash within the WIB.\n");

    wibStatus[tid] = WIBSquashing;

    doneSquashing[tid] = false;

    squashedSeqNum[tid] = squash_num;

    if (!instList[tid].empty()) {
        InstIt tail_thread = instList[tid].end();
        tail_thread--;

        squashIt[tid] = tail_thread;

        doSquash(tid);
    }

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
