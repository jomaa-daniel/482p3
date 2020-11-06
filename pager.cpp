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
    available_swap_blocks = swap_blocks;
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
    }
    // else copy the parent PID Arena struct
    else
    {
        arena = new Arena(*pid_to_arena[parent_pid]);
    }

    pid_to_arena[child_pid] = arena;
}


/*
 * vm_map
 *
 * A request by the current process for the lowest invalid virtual page in
 * the process's arena to be declared valid.  On success, vm_map returns
 * the lowest address of the new virtual page.  vm_map returns nullptr if
 * the arena is full.
 *
 * If filename is nullptr, block is ignored, and the new virtual page is
 * backed by the swap file, is initialized to all zeroes (from the
 * application's perspective), and private (i.e., not shared with any other
 * virtual page).  In this case, vm_map returns nullptr if the swap file is
 * out of space.
 *
 * If filename is not nullptr, the new virtual page is backed by the specified
 * file at the specified block and is shared with other virtual pages that are
 * mapped to that file and block.  filename is a null-terminated C string and
 * must reside completely in the valid portion of the arena.  In this case,
 * vm_map returns nullptr if filename is not completely in the valid part of
 * the arena.
 * filename is specified relative to the pager's current working directory.
 */
void *vm_map(const char *filename, unsigned int block)
{

    //Creating virtual page bound checks and moves valid bound
    // as well as checks and moves bound for swap memory.
    //If virtual page constructor throws we catch and return nullptr. 

    //check if filename is nullptr/ if true new page must point to 0 page.

    //Update ppage_to_owners new virtual page points to the 0 page (shared/probably)

    Virtual_Page *vp = nullptr;
    try
    {
        vp = new Virtual_Page();
    }
    catch(NotEnoughVirtualMemory& e)
    {
        if (DEBUG)
        {
            std::cout << e.what();
        }
        return nullptr;
    }
    catch(NotEnoughSwapBlocks& e)
    {
        if (DEBUG)
        {
            std::cout << e.what();
        }
        return nullptr; 
    }
    
    //Returning the previous address since we are using current for our next future vp
    return (void *) current_arena->valid_arena_bound - VM_PAGESIZE;
}




/*
 * vm_fault
 *
 * Called when current process has a fault at virtual address addr.  write_flag
 * is true if the access that caused the fault is a write.
 * Returns 0 on success, -1 on failure.
 */
int vm_fault(const void* addr, bool write_flag)
{
    
}

/*
 * vm_destroy
 *
 * Called when current process exits.  This gives the pager a chance to
 * clean up any resources used by the process.
 */
void vm_destroy()
{
    pid_to_arena.erase(current_running_pid);
}
