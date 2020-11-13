#include "vm_pager.h"
#include "globals.h"
#include "utils.h"
#include <string.h>


std::deque<Physical_Page*> mem_clock;
//Not required for correctness
std::unordered_map<unsigned int, Physical_Page*> ppage_in_clock; 

pid_t current_running_pid;
Arena* current_arena;
std::unordered_map<pid_t, Arena*> pid_to_arena;
std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> swap_block_to_owners;
std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> ppage_to_owners;
std::unordered_map<std::string, std::unordered_set<Virtual_Page*>> filename_to_owners;

unsigned int available_swap_blocks;
unsigned int MAX_SWAP_CAPACITY;
unsigned int MAX_PHYSICAL_MEM_SIZE;

std::string DEFAULT_FILENAME = "danyelnyko";

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
    
        
    assert_all();
    return 0;
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
    //assert_all();
      

    try
    {
        if(filename == nullptr)
        {
            new Virtual_Page();
        }
        else
        {
            // TODO explore what happens when one string is real
            // if ()
            // unsigned int str_len = strlen(filename);
            // if (str_len + uintptr_t(current_arena->valid_arena_bound) > 
            //     uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE)
            // {
            //     return nullptr;
                    // }

            if (not_valid_addr(uintptr_t(filename)))
            {
                return nullptr;
            }

            std::string read_string = "";              
            uintptr_t file_cursor = get_virtual_page_num(filename);
            unsigned int offset = get_offset_num(filename);
            bool found_end = false;

            while (!found_end)
            {
                void *vp_addr = (void*) (uintptr_t(VM_ARENA_BASEADDR) + (file_cursor * VM_PAGESIZE));
                if(not_valid_addr(uintptr_t(vp_addr)))
                {
                    return nullptr;
                }
                //If my page is read enabled i can safely look at the ppage and read the page
                if(current_arena->virtual_pages[file_cursor]->entry->read_enable == 0)
                {
                    vm_fault(vp_addr ,false);
                }
                void *ppage_start = (void *)((current_arena->virtual_pages[file_cursor]->entry->ppage * VM_PAGESIZE) + offset + uintptr_t(vm_physmem));
                void *ppage_end = (void *)((current_arena->virtual_pages[file_cursor]->entry->ppage * VM_PAGESIZE) + VM_PAGESIZE +  uintptr_t(vm_physmem));
                //Safe to read ppage now
                read_string.append(std::move(read_ppage_for_filename(ppage_start, ppage_end, found_end)));
                file_cursor = file_cursor+VM_PAGESIZE;
            }
            
            new Virtual_Page(read_string.data(), block);

        }
    }
    catch(NotEnoughVirtualMemory& e)
    {
        if (DEBUG)
        {
            std::cerr << e.what();
        }
        return nullptr;
    }
    catch(NotEnoughSwapBlocks& e)
    {
        if (DEBUG)
        {
            std::cerr << e.what();
        }
        return nullptr; 
    }
    
    assert_all();

    //Returning the previous address since we are using current for our next future vp
    return (void *) (current_arena->valid_arena_bound - VM_PAGESIZE);
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

    uintptr_t address = uintptr_t(addr);
    //Check if addr is invalid (bound check?)
    if (not_valid_addr(address))
    {
        return -1;
    }
    
    // find the virutal page
    unsigned int base_address = get_virtual_page_num(addr);

    Virtual_Page* current_page =
     current_arena->virtual_pages[base_address];
    
    //assert page_table[addr] is equal to page->entry
    // assert the entry pointers match table and V-page
    
    assert(&(current_arena->page_table.ptes[base_address])
     == current_page->entry);
     
    // can a page ever be not in swap nor mem?
    try
    {
        if (current_page->in_clock)
        {
            handle_fault_clock(write_flag, current_page);
        }
        // only in swap file/Or the 0 page, needs to be brought to phys mem
        else if (current_page->in_swap)
        {
            handle_fault_swap(write_flag, current_page);
        }
        else if(current_page->filename != DEFAULT_FILENAME)
        {
            handle_fault_file(write_flag, current_page);
        }
        else
        {
            handle_fault_zero(write_flag, current_page);
        }        

        assert_all();
        return 0;

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }   
    return -1;
}
/*
 * vm_switch
 *
 * Called when the kernel is switching to a new process, with process
 * identifier "pid".
 */
extern void vm_switch(pid_t pid)
{
    current_running_pid = pid;
    current_arena = pid_to_arena[pid];
    page_table_base_register = &current_arena->page_table;
    assert_all();
}

/*
 * vm_destroy
 *
 * Called when current process exits.  This gives the pager a chance to
 * clean up any resources used by the process.
 */
void vm_destroy()
{
    // this is a debug flag, tell the assertion that clock will move below the max limit
    clock_maxed_flag = false;

    delete current_arena;
    // when we pop the phys pages from the dequeu remembert to keep in sync with the map
    pid_to_arena.erase(current_running_pid);

    // make sure that the last virtual page pointing to
    // a physical page is made the sole owner
    // i.e. if the last owner is dirty, set W = 1 because 
    // he is now able to write with impunity 
    assert_all();
}
