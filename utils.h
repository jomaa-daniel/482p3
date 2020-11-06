#ifndef _utils_H_
#define _utils_H_

#include "vm_pager.h"
#include "vm_arena.h"
#include "globals.h"
#include <cassert>
unsigned int current_available_block = 0;

struct NotEnoughVirtualMemory : public std::exception
{
    const char * what() const throw ()
    {
        return "Not enough virtual memory";
    }
};

struct NotEnoughSwapBlocks : public std::exception
{
    const char * what() const throw ()
    {
        return "Eager Swap reservation doesn't have enough swap blockss";
    }
};


void zero_out_ppage(unsigned int ppage)
{
    for(int i = ppage; i < VM_PAGESIZE; ++i) {
        ((char *)vm_physmem)[i] = 0;
    }
}

unsigned int get_virtual_page_num(void* addr)
{
    uintptr_t address = (uintptr_t) addr;
    unsigned int mask = 0xFFFFFFFFFFFFF000;

    return address & mask;
}

unsigned int find_open_block() { 
    //check if current is avaiable
    while(swap_block_to_owners.find(current_available_block)
     != swap_block_to_owners.end())
    {
        current_available_block = (current_available_block+1) % MAX_SWAP_CAPACITY;
    }
    return current_available_block;
}


bool check_ppage_owners_age(Physical_Page *phys_page){

    bool recent = (*begin(ppage_to_owners[phys_page->ppage_location]))->entry->read_enable;
    
    // if first one is old:
        // ASSERT ALL of them are old, return true
    
    // if first one is NOT old
        // assert ALL of them are not old and change them to old at the same 


    for( auto vp : ppage_to_owners[phys_page->ppage_location] ){
        assert(recent != vp->entry->read_enable);
        if(vp->entry->read_enable == 1){
            vp->entry->read_enable = 0;
        }
    }

    return recent;
}

// keeps pushing and popping till it finds something to evict
// returns the pointer to a victim
// should we delete here and return a physical page on the stack?
Physical_Page find_victim()
{
    // make sure to keep in parallel with map
    //delete from deque and map
    Physical_Page* next_page = nullptr;
    bool is_recent = false;

    do
    {
        next_page = mem_clock.front();
        mem_clock.pop_front();
        // this makes changes to next page, marking it as old
        is_recent = check_ppage_owners_age(next_page);

        if (is_recent)
        {
            mem_clock.push_back(next_page);            
        } // push to deque
        else // remove from map, delete, return copy
        {
            Physical_Page local_copy(*next_page);
            ppage_in_clock.erase(next_page->ppage_location);
            delete next_page;
            return local_copy;
        }
    }
    while (is_recent);

}

// allocates and adds new physical page to the clock
// make sure to keep parallel with the map
void add_new_page_to_clock(unsigned int ppage_location)
{

    // VM_Fault takes care of write/dirty
    // we need to overwrite dirty in the case of is_swap

    //Check if deque size is less than MAX_PHYSICAL_MEM_SIZE?
    //Make physical page 
    //Go to owners of physical page(virtual pages) and change their settings
    // R:1,W;0 dirty:1 is_mem = 1 is_swap = variable;

    //at least 1 virtual page maps to this physical page
    assert(mem_clock.size() < MAX_PHYSICAL_MEM_SIZE);

    Physical_Page *new_page = new Physical_Page(ppage_location,false);


    // Update virtual pages who point to swap so they 
    //know where their new ppage is and update

    


    
    //Assert write: 0
    mem_clock.push_back(new_page);
    ppage_in_clock[ppage_location] = new_page;
}

// TAKE NOTE THAT THERE IS A POSSIBILITY THAT THE PHYSICAL PAGE HAS NO OWNERS
//  they might have been dealloc while still having v_pages pointing to phys pages
// in the clock
void evict_phys_page(Physical_Page& page)
{

    // propigate is_mem false
    // ASSSERT(R=0,W=0) on every ppage owner
    // if(first time evict){
        //get the new swap block
        // propigat the new swap block}   
    // else use the old swap block IF DIRTY ONLY (from owners list)

    //ASSERT ALL OWNERS HAVE THE SAME SWAP BLOCK

    //Delete your physical page owners form ppage_to_owners

    //Add your owners to swapblock_to_owners 
}


/////////////////// ASSERT FUNCTIONS //////////////////
bool assert_vpage_occurs_once()
{
    // for (auto & p : ppage_to_owners)
    // {
    //     std::unordered_set<VirtualPage
    // }
}

#endif /* _VM_PAGER_H_ */
