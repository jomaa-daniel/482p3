#include "vm_pager.h"
#include "globals.h"
#include "util.h"




/*
 * vm_init
 *
 * Called when the pager starts.  It should set up any internal data structures
 * needed by the pager.
 *
 * vm_init is passed the number of physical memory pages and the number
 * of blocks in the swap file.
 */
void vm_init(unsigned int memory_pages, unsigned int swap_blocks) 
{
    MAX_SWAP_CAPACITY = swap_blocks;
    MAX_PHYSICAL_MEM_SIZE = memory_pages;
    //This zero page should be allocated and 
    //initialized with zeroes when the pager starts,
    //and it should never be evicted
    zero_out_ppage(0);

    // for each physical page put its integer in deque
}

/*
 * vm_create
 * Called when a parent process (parent_pid) creates a new process (child_pid).
 * vm_create should cause the child's arena to have the same mappings and data
 * as the parent's arena.  If the parent process is not being managed by the
 * pager, vm_create should consider the arena to be empty.
 * Note that the new process is not run until it is switched to via vm_switch.
 * Returns 0 on success, -1 on failure.
 */
int vm_create(pid_t parent_pid, pid_t child_pid)
{
    Arena* arena = nullptr;
    // check to see if the parent PID exists, if it does not, zeros
    if (pid_to_arena.find(parent_pid) == end(pid_to_arena))
    {
        //Create empty arena
        // set to zeros
        arena = new Arena();
        pid_to_arena[child_pid] = arena;
    }
    // else copy the parent PID Arena struct
    else
    {
        arena = new Arena(*pid_to_arena[parent_pid]);
    }

}

