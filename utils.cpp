#ifndef _utils_H_
#define _utils_H_
#include "utils.h"
#include "globals.h"

unsigned int current_available_block = 0;


void zero_out_ppage(unsigned int ppage)
{
    for(unsigned int i = ppage; i < VM_PAGESIZE; ++i) {
        ((char *)vm_physmem)[i] = 0;
    }
}

unsigned int get_virtual_page_num(const void* addr)
{
    uintptr_t address = (uintptr_t) addr;
    uintptr_t mask = 0xFFFFFFFFFFFFF000;

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
            assert(vp->entry->write_enable == phys_page->dirty);
            vp->entry->read_enable = 0;
            vp->entry->write_enable = 0;
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
    
    throw std::runtime_error("Could not allocate a new swap block");
}

void write_to_swap(Virtual_Page* page)
{
    // if this is the first eviction of this virtual page, allocate new swap file block
    // update page block
    if (page->in_swap == false)
    {
        page->block = find_open_block();
    }
    
    // TODO copy on write for shared swap files

    // write page to swap block (call flie_write from infrastructure)
    file_write(nullptr, page->block, vm_physmem + (page->entry->ppage * VM_PAGESIZE)); 
}

void evict_phys_page(Physical_Page& page)
{

    // propigate is_mem false
    unordered_set<Virtual_Page*> owners = ppage_to_owners[page.ppage_location]; 

    unsigned int written_to_block;

    if (page.dirty)
    {
        write_to_swap(*begin(owners));
        written_to_block = (*begin(owners))->block;
    }

    for (auto virtual_page : owners)
    {
        assert(virtual_page->entry->read_enable == 0 and 
        virtual_page->entry->write_enable == 0 and virtual_page->in_clock);
        virtual_page->in_clock = false;
        virtual_page->in_swap = true;
        virtual_page->block = written_to_block;       
    }
    // if(first time evict){
        //get the new swap block
        // propigat the new swap block}   
    // else use the old swap block IF DIRTY ONLY (from owners list)
    //ASSERT ALL OWNERS HAVE THE SAME SWAP BLOCK
    
    
    if (swap_block_to_owners.find(written_to_block) != end(swap_block_to_owners))
    {
        assert([&]()->bool
        {
            for (auto vp : swap_block_to_owners[written_to_block])
            {
                assert(vp->entry->ppage == page.ppage_location);
                
                if (ppage_to_owners[vp->entry->ppage].find(vp) 
                == end(ppage_to_owners[vp->entry->ppage]))
                {
                    return false;
                }
            }
            return true;
        }());
    }

    //Add your owners to swapblock_to_owners 
    swap_block_to_owners[written_to_block] = owners;
    

    //Delete your physical page owners from ppage_to_owners
    ppage_to_owners.erase(page.ppage_location);
    
}

// allocates and adds new physical page to the clock
// make sure to keep parallel with the map
Physical_Page* add_new_page_to_clock(unsigned int ppage_location, bool write_flag, Virtual_Page* page)
{

    // VM_Fault takes care of write/dirty
    // we need to overwrite dirty in the case of is_swap

    //Check if deque size is less than MAX_PHYSICAL_MEM_SIZE?
    //Make physical page 
    //Go to owners of physical page(virtual pages) and change their settings
    // R:1,W;0 dirty:1 is_mem = 1 is_swap = variable;

    //at least 1 virtual page maps to this physical page
    assert(mem_clock.size() < MAX_PHYSICAL_MEM_SIZE);
    Physical_Page *new_page = new Physical_Page(ppage_location, write_flag);
    mem_clock.push_back(new_page);
    ppage_in_clock[ppage_location] = new_page;


    // Update virtual pages who point to swap so they 
    //know where their new ppage is and update

    
    // propigate to ppage owners (virtual pages)

    // Writeflag-true : R:1, W:1 , dirty_bit : 1, in_mem= 1
    // Writeflag-false: R:1, W:0, dirty_bit: 0, in_mem=1
    if (page->in_swap and !page->in_clock)
    {
        unordered_set<Virtual_Page*> &owners = swap_block_to_owners[page->block];

        for (auto virtual_page : owners)
        {
            // each virtual page needs to be aware that it has a swap block
            //  despite already being in swap_block_to_owners
            assert(virtual_page->in_swap);

            virtual_page->in_clock = true;
            virtual_page->entry->ppage = ppage_location; // TODO is this right??
            virtual_page->entry->read_enable = 1;
            virtual_page->entry->write_enable = write_flag;
            // ppage_to_owners[page->entry->ppage].insert(virtual_page); // TODO is this right?
        }
        ppage_to_owners[ppage_location] = owners;
    }
    else
    {
        // add new page to the ppage_
        // update the current virtual page to match the page table of the rest of the owners
        // assert that all of the owners share the same state i.e. read/write


    } 
    

    // migrate all of the owners from the swap file to the physical page owners
    
    return new_page;
}


bool assert_not_owner(Virtual_Page* page)
{
    for (auto& page_sets : ppage_to_owners)
    {
        if (page_sets.second.find(page) == end(page_sets.second))
        {
            return false;
        }        
    }
    return true;
}

bool not_valid_addr(uintptr_t addr)
{
        // thats what i thought
    return (addr < uintptr_t(VM_ARENA_BASEADDR) or 
    addr >  (uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE) or 
    addr > current_arena->valid_arena_bound);
}


void handle_fault_clock(bool write_flag, Virtual_Page* page)
{
    //Assert existence of ppage in owners
    assert(ppage_to_owners.find(page->entry->ppage) != end(ppage_to_owners));
    
    //assert this virtual page actually owns this physical page
    assert(ppage_to_owners[page->entry->ppage].find(page) != end(ppage_to_owners[page->entry->ppage]));
    
    //assert this ppage is owned by this virtual page
    assert(ppage_in_clock.find(page->entry->ppage) != end(ppage_in_clock));
    
    // R;0 W;0 Not referenced so update reference -> R:1, W:? in_mem = true
    // R:1 W:0 not dirty so update to dirty R:1, W:1 and ppage=dirty
    // R;0 W;0 SHARED Not referenced so update reference -> R:1, W:? in_mem = true
    // R:1 W:0 SHARED not dirty so update to dirty R:1, W:1 and ppage=dirty
    // R:0 W:1 assert not possible 
    //Propigate to all virtual owners

    //If dirty or write == 1 we must create a new ppage and give it a new swap.

    unordered_set<Virtual_Page*>& associated_pages = ppage_to_owners[page->entry->ppage];
    bool & d_bit = ppage_in_clock[page->entry->ppage]->dirty;
    if (write_flag and ppage_to_owners[page->entry->ppage].size() != 1)
    {
        // give new phys page and swap block, pop from current owners
        page->block = find_open_block();
        swap_block_to_owners[page->block] = unordered_set<Virtual_Page*>();
        swap_block_to_owners[page->block].insert(page);
        Physical_Page *ppage = create_phys_page(page, write_flag);
        //Could have just evicted the physical page that we just copied
        *ppage = ppage_in_clock()
        

        

    // what happens when the last remanining page is dity but not write enabled when its still not old

    }
    else
    {
        // propigate a referenced accross all associated pages
        for (auto assoc_page : associated_pages)
        {
            assert(assoc_page->in_clock);
            assert(page->entry->write_enable ==0 and 
            (page->entry->read_enable == 0 or page->entry->read_enable==1));
            assoc_page->entry->read_enable = 1;
            if (write_flag) // it only reaches this case if there is 
                            // one owner,else it would branch into above if statement
            {
                assoc_page->entry->write_enable = d_bit;
            }
        }
    }
    

}

unsigned int find_ppage()
{
    for(unsigned int i = 1; i < MAX_PHYSICAL_MEM_SIZE; ++i){
        if(ppage_in_clock.find(i) == end(ppage_in_clock)){
            return i;
        }
    }
    assert(false);
    return 0;
}
Physical_Page* create_phys_page(Virtual_Page* page, bool write_flag)
{
    //Need to bring you into memory 
    //Check if there are any empty ppages to add to
    //if not evict and do (associated changes in eviction)

    unsigned int free_ppage = 0;
    if (mem_clock.size() + 1 == MAX_PHYSICAL_MEM_SIZE)
    {
        Physical_Page victim = find_victim();
        evict_phys_page(victim);
        free_ppage = victim.ppage_location;
    }
    
    //Create new ppage object pass ppage and dirty bit accordingly
    // add to the clock
    if(!free_ppage)
    {
        free_ppage = find_ppage();
    }

    return add_new_page_to_clock(free_ppage, write_flag, page);  
}

void handle_fault_swap(bool write_flag, Virtual_Page* page)
{
    // assert that the swap block is in fact valid
    assert(swap_block_to_owners.find(page->block) != end(swap_block_to_owners));

    // assert this virtual page is an owner of the block
    assert(swap_block_to_owners[page->block].find(page) != end(swap_block_to_owners[page->block]));
    
    //assert that this virtual page is not an owner of any ppages.
    assert(assert_not_owner(page));
    
    //Assert you have proper enables
    assert(page->entry->read_enable == 0 and page->entry->write_enable == 0);
    
    create_phys_page(page, write_flag);
    
    file_read(nullptr, page->block, (void *) (uintptr_t(vm_physmem) + (VM_PAGESIZE * page->entry->ppage)));
}

/////////////////// ASSERT FUNCTIONS //////////////////
bool sync_all()
{
   // make sure that number of total virutal pages that are in clock are in sync with the total number of owners in ppage

    // make sure that the number of virutal pages that are in mem are in sync with the total number of owners in swap

    //
}

bool assert_no_dirty_pages(Physical_Page* page)
{
    return false;
}

#endif /* _VM_PAGER_H_ */
