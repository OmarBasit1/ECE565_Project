#ifndef __CPU_O3_WIB_HH__
#define __CPU_O3_WIB_HH__

#include <string>
#include <utility>
#include <vector>

#include "base/statistics.hh"
#include "base/types.hh"
#include "cpu/inst_seq.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
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

  public:
    WIB(CPU *_cpu, const BaseO3CPUParams &params);

    std::string name() const;

    /** Perform sanity checks after a drain. */
    void drainSanityCheck() const;

    /** Function to insert an instruction into the WIB. Note that whatever
     *  calls this function must ensure that there is enough space within the
     *  WIB for the new instruction.
     *  @param inst The instruction being inserted into the WIB.
     */
    void insertInst(const DynInstPtr &inst);

    /** Retires the head instruction of a specific thread, removing it from the
     *  ROB.
     */
    void retireHead();

    /** Returns the number of total free entries in the WIB. */
    unsigned numFreeEntries();

    /** Returns if the WIB is full. */
    bool isFull()
    { return numInstsInWIB == numEntries; }

    /** Returns if the WIB is empty. */
    bool isEmpty() const
    { return numInstsInWIB == 0; }

    /** Executes the squash, marking squashed instructions. */
    void doSquash(InstSeqNum squash_num);

    /** Squashes all instructions younger than the given sequence number for
     *  the specific thread.
     */
    void squash(InstSeqNum squash_num);

    /** Updates the head instruction with the new oldest instruction. */
    void updateHead();

    /** Updates the tail instruction with the new youngest instruction. */
    void updateTail();

    /** Checks if the WIB is still in the process of squashing instructions.
     *  @retval Whether or not the WIB is done squashing.
     */
    bool isDoneSquashing() const
    { return doneSquashing; }

    /** Checks if the WIB is still in the process of squashing instructions for
     *  any thread.
     */
    bool isDoneSquashing();

  private:
    /** Reset the WIB state */
    void resetState();

    /** Pointer to the CPU. */
    CPU *cpu;

    /** Number of instructions in the WIB. */
    unsigned numEntries;

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
    InstIt squashIt;

  public:
    /** Number of instructions in the WIB. */
    int numInstsInWIB;

    /** Dummy instruction returned if there are no insts left. */
    DynInstPtr dummyInst;

  private:
    /** The sequence number of the squashed instruction. */
    InstSeqNum squashedSeqNum;

    /** Is the WIB done squashing. */
    bool doneSquashing;

    // struct WIBStats : public statistics::Group
    // {
    //     WIBStats(statistics::Group *parent);
    //
    //     // The number of wib_reads
    //     statistics::Scalar reads;
    //     // The number of wib_writes
    //     statistics::Scalar writes;
    // } stats;

  public:
    //columns for each load cache miss instruction
    //rows for each instruction in WIB/ROB that are dependent on the load cache miss
    std::vector<std::vector<bool>> bitMatrix;
    
    // list of instructions active instructions
    std::vector<DynInstPtr> instList;
    // list of load misses
    std::vector<DynInstPtr> loadList;

    // head/tail pointers for instList
    size_t headInst;
    size_t tailInst;

    size_t numLoads;

    // adds new load to WIB, returns the column idx of the bitMatrix
    size_t addColumn(const DynInstPtr &inst);
    // removes column when the load completes
    void removeColumn(const size_t colIdx);

    // squash column if the load miss instruction gets squashed
    void squashColumn(const size_t colIdx);
    // squash row if the instruction gets squashed in issue queue
    void squashRow(const size_t rowIdx);
    
    // tag pretend ready instructions in WIB instead of issueing
    void tagDependentInst(const DynInstPtr &inst, const size_t colIdx);

};

} // namespace o3
} // namespace gem5

#endif //__CPU_O3_WIB_HH__
