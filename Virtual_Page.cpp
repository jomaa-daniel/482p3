#include "globals.h"

Virtual_Page::Virtual_Page() : block(0), in_swap(false), in_clock(false) 
{
    //iterate valid_arena_bound 
    if (current_arena->valid_arena_bound + VM_PAGESIZE > VM_ARENA_SIZE ||
        current_arena->valid_arena_bound < uintptr_t(VM_ARENA_BASEADDR))
    { 
        throw NotEnoughVirtualMemory();
    }
    //Eager Swap reservation (Make sure virtual page has enough space in swap file)
    if(available_swap_blocks > 0){
        //Iterate swap space
        --available_swap_blocks;
    }
    else{
        throw NotEnoughSwapBlocks();
    }
    uintptr_t vp_num = get_virtual_page_num((void *) current_arena->valid_arena_bound);
    entry = &current_arena->page_table.ptes[vp_num];
    
    //Make sure to set entry to be appropriate for ppage 0.
    entry->ppage = 0;
    entry->read_enable = 1;
    entry->write_enable = 0;

    //Store in arenas virtual page map
    current_arena->virtual_pages[vp_num];

    //Update ppage_to_owner map/ so this virtual page owns 0 page
    if(ppage_to_owners.find(0) == std::end(ppage_to_owners))
    {
        ppage_to_owners[0] = std::unordered_set<Virtual_Page*>();
    }

    ppage_to_owners[0].insert(this);

    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
    current_arena->valid_arena_bound + VM_PAGESIZE - 1;
}

Virtual_Page::Virtual_Page(unsigned int block_, bool in_swap_, bool in_clock_, unsigned int ppage_entry, bool read_enable, bool write_enable, Arena *arena)
: block(block_), in_swap(in_swap), in_clock(in_clock_) 
{
    //iterate valid_arena_bound ????
    
    if (current_arena->valid_arena_bound + VM_PAGESIZE > VM_ARENA_SIZE ||
        current_arena->valid_arena_bound < uintptr_t(VM_ARENA_BASEADDR))
    { 
        throw NotEnoughVirtualMemory();
    }

    //Eager Swap reservation (Make sure virtual page has enough space in swap file)
    if(available_swap_blocks > 0){
        //Iterate swap space
        --available_swap_blocks;
    }
    else{
        throw NotEnoughSwapBlocks();
    }
    uintptr_t vp_num = get_virtual_page_num((void *) arena->valid_arena_bound);
    entry = &arena->page_table.ptes[vp_num];
    
    entry->ppage = ppage_entry;
    entry->read_enable = read_enable;
    entry->write_enable = write_enable;

    //Store in arenas virtual page map
    arena->virtual_pages[vp_num];

    //Update ppage_to_owner map/ so this virtual page owns 0 page
    if(ppage_to_owners.find(ppage_entry) == ppage_to_owners.end())
    {
        ppage_to_owners[ppage_entry] = std::unordered_set<Virtual_Page*>();
    }

        
    ppage_to_owners[ppage_entry].insert(this);

    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
        current_arena->valid_arena_bound + VM_PAGESIZE;
    
}       