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
    for (ThreadID tid = numThreads; tid < MaxThreads; tid++) {
        maxEntries[tid] = 0;
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

} // namespace o3
} // namespace gem5
