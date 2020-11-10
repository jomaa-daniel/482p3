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

    return ((address & mask)- uintptr_t(VM_ARENA_BASEADDR))/VM_PAGESIZE;
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

void handle_last_owner(Physical_Page& old_phys_page)
{

    // if there is one more page left, check for a fix
    // that makes sure that the next write will succeed if write enabled

    if (ppage_to_owners[old_phys_page.ppage_location].size() == 1)
    {
        Virtual_Page* last_owner = *begin(ppage_to_owners[old_phys_page.ppage_location]);
        // dont want to accidentally make it life when it is not referenced
        if (last_owner->entry->read_enable == 1)
        {
        // this if there is one owner they should not fault on a write
            last_owner->entry->write_enable = old_phys_page.dirty;
        }
        
    }
}

bool check_ppage_owners_age(Physical_Page *phys_page){

    if (DEBUG)
    cout << "check ppage owners age\n";

    bool recent = (*begin(ppage_to_owners[phys_page->ppage_location]))->entry->read_enable;
    
    // if first one is old:
        // ASSERT ALL of them are old, return true
    
    // if first one is NOT old
        // assert ALL of them are not old and change them to old at the same 


    for( auto vp : ppage_to_owners[phys_page->ppage_location] ){
        assert(recent == vp->entry->read_enable);// CHANGED FROM != to ==
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
    if (DEBUG)
    cout << "write to swap\n";
    // if this is the first eviction of this virtual page, allocate new swap file block
    // update page block
    if (page->in_swap == false)
    {
        page->block = find_open_block();
    }
    
    // TODO copy on write for shared swap files

    // write page to swap block (call flie_write from infrastructure)
    file_write(nullptr, page->block, (void*) (uintptr_t(vm_physmem) + (page->entry->ppage * VM_PAGESIZE))); 
}

void evict_phys_page(Physical_Page& page)
{
    
    // propigate is_mem false
    unordered_set<Virtual_Page*> owners = ppage_to_owners[page.ppage_location]; 

    //Using 99999 as an obvious error checker
    unsigned int written_to_block = 99999;

    if (page.dirty)
    {
        write_to_swap(*begin(owners));
    }

    written_to_block = (*begin(owners))->block;

    for (auto virtual_page : owners)
    {
        assert(virtual_page->entry->read_enable == 0 and 
        virtual_page->entry->write_enable == 0 and virtual_page->in_clock);
        virtual_page->in_clock = false;
        virtual_page->in_swap = true;

        if (page.dirty)
        {
            virtual_page->block = written_to_block;
        }
        
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
Physical_Page* add_new_page_to_clock(unsigned int ppage_location, 
    bool write_flag)
{
    if (DEBUG)
    cout << "add_new_page_to_clock\n";

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
    // add new page to the ppage_
    // update the current virtual page to match the page table of the rest of the owners
    // assert that all of the owners share the same state i.e. read/write
    // fix for: TODO 4,2
    // migrate all of the owners from the swap file to the physical page owners
    
    return new_page;
}


bool assert_not_owner(Virtual_Page* page)
{
    for (auto& page_sets : ppage_to_owners)
    {
        // TODO verify // changed from == to != 
        if (page_sets.second.find(page) != end(page_sets.second))
        {
            return false;
        }        
    }
    return true;
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

bool not_valid_addr(uintptr_t addr)
{
        // thats what i thought
    return (addr < uintptr_t(VM_ARENA_BASEADDR) or 
    addr >  (uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE) or 
    addr > current_arena->valid_arena_bound);
}

Physical_Page* create_phys_page(bool write_flag)
{
    if (DEBUG)
    cout << "create_phys_page\n";
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

    return add_new_page_to_clock(free_ppage, write_flag);  
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
    page_table_entry_t * c = &current_arena->page_table.ptes[4096];
    unordered_set<Virtual_Page*>& associated_pages = 
            ppage_to_owners[page->entry->ppage];

    bool & d_bit = ppage_in_clock[page->entry->ppage]->dirty;

    // copy on write, shared physical ppage
    if (write_flag and ppage_to_owners[page->entry->ppage].size() != 1)
    {
        // give new phys page and swap block, pop from current owners
        // REMOVED THE ADDING TO THE SWAP BLOCK CODE
        
        // create a local copy of phys_page
        Physical_Page  old_ppage = *(ppage_in_clock[page->entry->ppage]);

        Physical_Page * new_ppage = create_phys_page(write_flag);    
        // remove itself as an owner of the old physical page
        ppage_to_owners[page->entry->ppage].erase(page);
        // remove itself as an owner of the old swap file/set swap to false
        if (page->in_swap)
        {
            swap_block_to_owners[page->block].erase(page);
            page->in_swap = false;
        }

        // handle what happens if one owner is left
        handle_last_owner(old_ppage);

        /* 
            this solves the issue of the phys page being dealloc'ed by
            this point because really what are we copying over? a certain
            spot in phys_mem which doesnt change even if we dealloc the phy
            page that points to it.

        */
        *new_ppage = old_ppage;
    
    }
    else
    {
        // propigate a referenced accross all associated pages
        for (auto assoc_page : associated_pages)
        {
            assert(assoc_page->in_clock);
            assert(page->entry->write_enable == 0 and 
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
    
    // adds to clock
    Physical_Page* new_phys_page = create_phys_page(write_flag);

    // TODO propigate moving the swap file from swap to phys
    unordered_set<Virtual_Page*> &owners = swap_block_to_owners[page->block];

    for (auto virtual_page : owners)
    {
        // each virtual page needs to be aware that it has a swap block
        //  despite already being in swap_block_to_owners
        assert(virtual_page->in_swap);

        virtual_page->in_clock = true;
        virtual_page->entry->ppage = new_phys_page->ppage_location; // TODO is this right??
        virtual_page->entry->read_enable = 1;
        virtual_page->entry->write_enable = write_flag;
        // ppage_to_owners[page->entry->ppage].insert(virtual_page); // TODO is this right?
    }

    ppage_to_owners[new_phys_page->ppage_location] = owners;
    file_read(nullptr, page->block, (void *) (uintptr_t(vm_physmem) + (VM_PAGESIZE * page->entry->ppage)));
}

void handle_fault_zero(bool write_flag, Virtual_Page* page)
{
    assert(write_flag);

    // adds to clock
    Physical_Page* new_phys_page = create_phys_page(write_flag);
    zero_out_ppage(new_phys_page->ppage_location);

    page->entry->ppage = new_phys_page->ppage_location;
    page->entry->read_enable = 1;
    page->entry->write_enable = 1;
    page->in_clock = true;
    assert(page->in_swap == false);

    assert (ppage_to_owners.find(new_phys_page->ppage_location) == end(ppage_to_owners));
    
    ppage_to_owners[new_phys_page->ppage_location] = unordered_set<Virtual_Page*>();
    
    ppage_to_owners[new_phys_page->ppage_location].insert(page);
    
    assert(ppage_to_owners.find(0) != end(ppage_to_owners));

    ppage_to_owners[0].erase(page);
}


/////////////////// ASSERT FUNCTIONS //////////////////



bool assert_clock_and_pages_sync()
{
    // count the number of pages in the arena that say they are in clock
    int virtual_page_count = 0;

    for (const auto& arena : pid_to_arena)
    {
        for (const auto virutal_page : arena.second->virtual_pages)
        {
            if (virutal_page.second->in_clock or
                virutal_page.second->entry->ppage == 0)
            {
                ++virtual_page_count;
            }
        }
    }
    
    int owner_count = 0;
    // now count the number of owners in ppage to owners
    for (const auto& owner_set : ppage_to_owners)
    {
        int sz = owner_set.second.size();
        owner_count += sz;
    }

    return owner_count == virtual_page_count;
}

bool assert_swap_and_pages_sync()
{
    int page_count = 0;

    for (const auto& arena : pid_to_arena)
    {
        for (const auto virutal_page : arena.second->virtual_pages)
        {
            if (virutal_page.second->in_swap)
            {
                ++page_count;
            }
        }
    }
    
    int owner_count = 0;
    // now count the number of owners in ppage to owners
    for (const auto& owner_set : swap_block_to_owners)
    {
        int sz = owner_set.second.size();
        owner_count += sz;
    }

    return owner_count == page_count;
}

bool assert_ppage_owners_sync()
{
    for (const auto & owner_set : ppage_to_owners)
    {
        if (!owner_set.second.empty())
        {
            Virtual_Page *base_page = *begin(owner_set.second);
            
            for (const auto& page : owner_set.second)
            {
                if (!(*base_page == *page))
                {
                    return false;
                }
            }
        }
    }

    return true;
}

bool assert_swap_block_owners_sync(){

    for(const auto& block_owner_pair : swap_block_to_owners){
        const auto first_vp = begin(block_owner_pair.second);

        for(const auto vp : block_owner_pair.second){
            if(!(vp == *first_vp)){
                return false;
            }
        }
    }
    return true;
}

bool assert_clock_map_match(){
    
    for(auto ppage : mem_clock){
        if(ppage_in_clock[ppage->ppage_location] != ppage){
            return false;
        }
    }
    return true;
}

bool assert_no_write_on_share(){
    for(const auto& block_owner_pair : swap_block_to_owners){
        //If its shared
        if(block_owner_pair.second.size() > 1){
            for(const auto vp : block_owner_pair.second){
                if(vp->entry->write_enable == 1){
                    return false;
                }
            }
        }
        
    }
    for(const auto& ppage_owner_pair : ppage_to_owners){
        //If its shared
        if(ppage_owner_pair.second.size() > 1){
            for(const auto vp : ppage_owner_pair.second){
                if(vp->entry->write_enable == 1){
                    return false;
                }
            }
        }
    }
    return true;
}

bool assert_same_ppage_same_block()
{
    
    // vp1 :in_mem=true ppage = 3 in_swap = true -> block = 10
    // vp2 :in_mem=true ppage = 3 in_swap = true -> block = 10

    //ppage_to_owners[3] = {vp1, vp2}


    for(const auto ppage : mem_clock){
        if(ppage_to_owners[ppage->ppage_location].size() < 1) return false;

        const unsigned int first_block = 
            (*begin(ppage_to_owners[ppage->ppage_location]))->block;

        for(const auto vp : ppage_to_owners[ppage->ppage_location])
        {
            if(vp->in_swap == true and vp->block != first_block)
            {
                return false;
            }
        }
    }
    return true;
}

bool assert_ppage_map_correctness()
{
    for (const auto & arena : pid_to_arena)
    {
        for (const auto & virtual_page : arena.second->virtual_pages)
        {
           if (virtual_page.second->in_clock)
           {
               auto result = ppage_to_owners[virtual_page.second->entry->ppage].find(virtual_page.second);
                if (result == end(ppage_to_owners[virtual_page.second->entry->ppage]))
                {
                    return false;
                }               
           }
        }
    }

    return true;
}

bool assert_ppage_clock_correctness()
{
    for (const auto & arena : pid_to_arena)
    {
        for (const auto & virtual_page : arena.second->virtual_pages)
        {
           if (virtual_page.second->in_clock)
           {
               auto result = ppage_in_clock.find(virtual_page.second->entry->ppage);
                if (result == end(ppage_in_clock))
                {
                    return false;
                }               
           }
        }
    }

    return true;
}

bool assert_swap_map_correctness()
{
    for (const auto & arena : pid_to_arena)
    {
        for (const auto & virtual_page : arena.second->virtual_pages)
        {
           if (virtual_page.second->in_swap)
           {
               auto result = swap_block_to_owners[virtual_page.second->block].find(virtual_page.second);
                if (result == end(swap_block_to_owners[virtual_page.second->block]))
                {
                    return false;
                }               
           }
        }
    }

    return true;
}

bool assert_zero_page_correctness(){
    for (const auto & arena : pid_to_arena){
        for (const auto & virtual_page : arena.second->virtual_pages){
            if(virtual_page.second->entry->ppage == 0){
                //zero page cant be in clock never evicted
                if(virtual_page.second->in_clock == true){
                    return false;
                }
                if(virtual_page.second->entry->write_enable == true){
                    return false;
                }
                if(virtual_page.second->entry->read_enable == false){
                    return false;
                }
                if(virtual_page.second->in_swap == true){
                    return false;
                }
            }
        }
    }
    return true;
}

bool assert_valid_swap_owners()
{
    for (const auto & swap_block : swap_block_to_owners)
    {
        if (swap_block.second.empty())
        {
            return false;
        }
    }
    return true;
}

void assert_all()
{
    if (DEBUG)
    return;


    assert(assert_clock_and_pages_sync());

    assert(assert_swap_and_pages_sync());

    assert(assert_ppage_owners_sync());

    assert(assert_swap_block_owners_sync());

    assert(assert_clock_map_match());

    assert(assert_no_write_on_share());

    assert(assert_same_ppage_same_block());

    assert(assert_ppage_map_correctness());

    assert(assert_swap_map_correctness());

    assert(assert_zero_page_correctness());
    
    assert(assert_valid_swap_owners());
}
#endif /* _VM_PAGER_H_ */
