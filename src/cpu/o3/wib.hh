#ifndef __CPU_O3_WIB_HH__
#define __CPU_O3_WIB_HH__

#include <string>
#include <utility>
#include <vector>

#include "base/statistics.hh"
#include "base/types.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
#include "cpu/o3/limits.hh"
#include "cpu/reg_class.hh"

namespace gem5
{

struct BaseO3CPUParams;

namespace o3
{

class CPU;

struct DerivO3CPUParams;

/**
 * WIB class.
 */
class WIB
{
  public:
    typedef std::pair<RegIndex, RegIndex> UnmapInfo;
    typedef typename std::list<DynInstPtr>::iterator InstIt;

    /** Possible WIB statuses. */
    enum Status
    {
        Running,
        Idle,
        WIBSquashing
    };

  private:
    /** Per-thread WIB status. */
    Status wibStatus[MaxThreads];

  public:
    WIB(CPU *_cpu, const BaseO3CPUParams &params);

    std::string name() const;

    /** Sets pointer to the list of active threads.
     *  @param at_ptr Pointer to the list of active threads.
     */
    void setActiveThreads(std::list<ThreadID> *at_ptr);

    /** Perform sanity checks after a drain. */
    void drainSanityCheck() const;

    /** Takes over another CPU's thread. */
    void takeOverFrom();

    /** Function to insert an instruction into the WIB. Note that whatever
     *  calls this function must ensure that there is enough space within the
     *  WIB for the new instruction.
     *  @param inst The instruction being inserted into the WIB.
     */
    void insertInst(const DynInstPtr &inst);

    /** Returns the number of total free entries in the WIB. */
    unsigned numFreeEntries();

    /** Returns the number of free entries in a specific WIB paritition. */
    unsigned numFreeEntries(ThreadID tid);

    /** Returns the maximum number of entries for a specific thread. */
    unsigned getMaxEntries(ThreadID tid)
    { return maxEntries[tid]; }

    /** Returns the number of entries being used by a specific thread. */
    unsigned getThreadEntries(ThreadID tid)
    { return threadEntries[tid]; }

    /** Returns if the WIB is full. */
    bool isFull()
    { return numInstsInWIB == numEntries; }

    /** Returns if a specific thread's partition is full. */
    bool isFull(ThreadID tid)
    { return threadEntries[tid] == numEntries; }

    /** Returns if the WIB is empty. */
    bool isEmpty() const
    { return numInstsInWIB == 0; }

    /** Returns if a specific thread's partition is empty. */
    bool isEmpty(ThreadID tid) const
    { return threadEntries[tid] == 0; }

    /** Executes the squash, marking squashed instructions. */
    void doSquash(ThreadID tid);

    /** Squashes all instructions younger than the given sequence number for
     *  the specific thread.
     */
    void squash(InstSeqNum squash_num, ThreadID tid);

    /** Checks if the WIB is still in the process of squashing instructions.
     *  @retval Whether or not the WIB is done squashing.
     */
    bool isDoneSquashing(ThreadID tid) const
    { return doneSquashing[tid]; }

    /** Checks if the WIB is still in the process of squashing instructions for
     *  any thread.
     */
    bool isDoneSquashing();

  private:
    /** Reset the WIB state */
    void resetState();

    /** Pointer to the CPU. */
    CPU *cpu;

    /** Active Threads in CPU */
    std::list<ThreadID> *activeThreads;

    /** Number of instructions in the WIB. */
    unsigned numEntries;

    /** Entries Per Thread */
    unsigned threadEntries[MaxThreads];

    /** Max Insts a Thread Can Have in the WIB */
    unsigned maxEntries[MaxThreads];

    /** WIB List of Instructions */
    std::list<DynInstPtr> instList[MaxThreads];

    /** Number of instructions that can be squashed in a single cycle. */
    unsigned squashWidth;

  private:
    /** Iterator used for walking through the list of instructions when
     *  squashing.  Used so that there is persistent state between cycles;
     *  when squashing, the instructions are marked as squashed but not
     *  immediately removed, meaning the tail iterator remains the same before
     *  and after a squash.
     *  This will always be set to cpu->instList.end() if it is invalid.
     */
    InstIt squashIt[MaxThreads];

  public:
    /** Number of instructions in the WIB. */
    int numInstsInWIB;

    /** Dummy instruction returned if there are no insts left. */
    DynInstPtr dummyInst;

  private:
    /** The sequence number of the squashed instruction. */
    InstSeqNum squashedSeqNum[MaxThreads];

    /** Is the WIB done squashing. */
    bool doneSquashing[MaxThreads];

    /** Number of active threads. */
    ThreadID numThreads;


    struct WIBStats : public statistics::Group
    {
        WIBStats(statistics::Group *parent);

        // The number of wib_reads
        statistics::Scalar reads;
        // The number of wib_writes
        statistics::Scalar writes;
    } stats;
};

} // namespace o3
} // namespace gem5

#endif //__CPU_O3_WIB_HH__
