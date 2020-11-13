#ifndef _utils_H_
#define _utils_H_
#include "utils.h"
#include "globals.h"
#include <vector>

unsigned int current_available_block = 0;

// assertion flag
bool clock_maxed_flag = false;

void zero_out_ppage(unsigned int ppage)
{
    for(unsigned int i = ppage * VM_PAGESIZE; 
    i < (ppage * VM_PAGESIZE) + VM_PAGESIZE; ++i) {
        ((char *)vm_physmem)[i] = 0;
    }
}

unsigned int get_virtual_page_num(const void* addr)
{
    uintptr_t address = (uintptr_t) addr;
    uintptr_t mask = 0xFFFFFFFFFFFFF000;

    return ((address & mask)- uintptr_t(VM_ARENA_BASEADDR))/VM_PAGESIZE;
}

unsigned int get_offset_num(const void* addr)
{
    uintptr_t address = (uintptr_t) addr;
    uintptr_t mask = 0x0000000000000FFF;

    return (address & mask);
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

void copy_from_physmem(std::vector<char>& output_buffer, unsigned int ppage_location)
{
    output_buffer.reserve(VM_PAGESIZE);
    int base_lhs = ppage_location * VM_PAGESIZE;

    for (unsigned int i = 0; i < VM_PAGESIZE; ++i)
    {
        output_buffer.push_back(((char *)vm_physmem)[base_lhs + i]);
    }
}
void copy_to_physmem(std::vector<char>& input_buffer, unsigned int ppage_location)
{
    int base_lhs = ppage_location * VM_PAGESIZE;

    for (unsigned int i = 0; i < VM_PAGESIZE; ++i)
    {
        ((char *)vm_physmem)[base_lhs + i] = input_buffer[i];
    }
}


bool check_ppage_owners_age(Physical_Page *phys_page){


    bool recent = (*begin(ppage_to_owners[phys_page->ppage_location]))->entry->read_enable;
    
    // if first one is old:
        // ASSERT ALL of them are old, return true
    
    // if first one is NOT old
        // assert ALL of them are not old and change them to old at the same 


    for ( auto vp : ppage_to_owners[phys_page->ppage_location] ){
        assert(recent == vp->entry->read_enable);// CHANGED FROM != to ==
        if (vp->entry->read_enable == 1){
            if (ppage_to_owners.find(phys_page->ppage_location)->second.size()==1)
            {
                assert(vp->entry->write_enable == phys_page->dirty);    
            }
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


void evict_swap_back_handler(Physical_Page& ppage, unordered_set<Virtual_Page*> &owners)
{
    //Using 99999 as an obvious error checker
    unsigned int written_to_block = 99999;

    if (ppage.dirty)
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

        if (ppage.dirty)
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
                assert(vp->entry->ppage == ppage.ppage_location);
                
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
}

void evict_file_back_handler(Physical_Page& ppage, unordered_set<Virtual_Page*> &owners)
{
    auto vp = *begin(owners);
    if (ppage.dirty)
    {
        //Write back
        file_write(vp->filename.data(), vp->block, (void *) ( uintptr_t(vm_physmem) + (VM_PAGESIZE * vp->entry->ppage)));
    }

    for (const auto & virtual_page : owners)
    {
        assert(virtual_page->entry->read_enable == 0 and 
        virtual_page->entry->write_enable == 0 and virtual_page->in_clock and virtual_page->in_swap==false);
        virtual_page->in_clock = false;
    }
    // if(first time evict){
        //get the new swap block
        // propigat the new swap block}   
    // else use the old swap block IF DIRTY ONLY (from owners list)
    //ASSERT ALL OWNERS HAVE THE SAME SWAP BLOCK
}

void evict_phys_page(Physical_Page& ppage)
{
    
    // propigate is_mem false
    unordered_set<Virtual_Page*> owners = ppage_to_owners[ppage.ppage_location]; 

    
    if((*begin(owners))->filename == DEFAULT_FILENAME){
        evict_swap_back_handler(ppage, owners);
    }
    else{
        evict_file_back_handler(ppage, owners);
    }

    //Delete your physical page owners from ppage_to_owners
    ppage_to_owners.erase(ppage.ppage_location);
    
}

// allocates and adds new physical page to the clock
// make sure to keep parallel with the map
Physical_Page* add_new_page_to_clock(unsigned int ppage_location, 
    bool write_flag)
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
    //Need to bring you into memory 
    //Check if there are any empty ppages to add to
    //if not evict and do (associated changes in eviction)

    unsigned int free_ppage = 0;
    if (mem_clock.size() + 1 == MAX_PHYSICAL_MEM_SIZE)
    {
        // raise a flag that the clock is now at the limit
        clock_maxed_flag = true;

        Physical_Page victim = find_victim();
        evict_phys_page(victim);
        free_ppage = victim.ppage_location;
    }
    
    //Create new ppage object pass ppage and dirty bit accordingly
    // add to the clock
    if(!free_ppage)
    {
        free_ppage = find_ppage();
        //Make sure we dont zero out ppage since 
        // zero_out_ppage(free_ppage);
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
    unordered_set<Virtual_Page*>& associated_pages = 
            ppage_to_owners[page->entry->ppage];

    // WHERE ARE WE SETTING THIS IF IT IS DIRTY??
    bool & d_bit = ppage_in_clock[page->entry->ppage]->dirty;

    // copy on write, shared physical ppage
    if (write_flag and ppage_to_owners[page->entry->ppage].size() != 1 
        and page->filename == DEFAULT_FILENAME)
    {
        // give new phys page and swap block, pop from current owners
        // REMOVED THE ADDING TO THE SWAP BLOCK CODE
        
        // create a local copy of phys_page
        std::vector<char> buffer;
    
        copy_from_physmem(buffer, page->entry->ppage);

        //Handle if there will be 1 more owner after the copy on write is compelete/ we must give him write access if he was dirty
        if(ppage_to_owners[page->entry->ppage].size() == 2)
        {
            for(auto vp : ppage_to_owners[page->entry->ppage])
            {
                    // dont want to accidentally make it life when it is not referenced ONLY IF SWAP BACKED
                    if(vp != page and vp->entry->read_enable == 1 and vp->filename == DEFAULT_FILENAME)
                    {
                        vp->entry->write_enable = ppage_in_clock[vp->entry->ppage]->dirty;
                    }
            }
            
        }

        Physical_Page * new_ppage = create_phys_page(write_flag);    
        // remove itself as an owner of the old physical page
        ppage_to_owners[page->entry->ppage].erase(page);
        // add himself as a n owner of the NEW phyiscal page
        ppage_to_owners[new_ppage->ppage_location].insert(page);
        // change the table entry to the new ppage
        page->entry->ppage = new_ppage->ppage_location;
        // change the write enable
        page->entry->write_enable = 1;
        //Change the read enable in case we evicted the copy on right
        page->entry->read_enable = 1;
        // add page to the clock
        page->in_clock = 1;


        assert(page->entry->read_enable);
        
        // remove itself as an owner of the old swap file/set swap to false
        if (page->in_swap)
        {
            swap_block_to_owners[page->block].erase(page);
            page->in_swap = false;
        }

        copy_to_physmem(buffer, new_ppage->ppage_location);    
    }
    else
    {
        
        // propigate a referenced accross all associated pages
        if (write_flag)
        {
            d_bit = write_flag;
        }

        for (auto assoc_page : associated_pages)
        {
            assert(assoc_page->in_clock);
            assert(page->entry->write_enable == 0 and 
            (page->entry->read_enable == 0 or page->entry->read_enable==1));
            assoc_page->entry->read_enable = 1;
            assoc_page->entry->write_enable = d_bit;          
        }
    }
}


void handle_fault_swap(bool write_flag, Virtual_Page* page)
{
    //assert you are not file backed
    assert(page->filename == DEFAULT_FILENAME);

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
    Physical_Page old_ppage(*new_phys_page);
    file_read(nullptr, page->block, (void *) (uintptr_t(vm_physmem) + (VM_PAGESIZE * page->entry->ppage)));


    // if write and multiple owners: 
    //remove as parent of existing swap block
    //Remove the swap block;
    //Change in swap:false
    //Change in clock: true
    //Set new physpage location
    //Set new ppage_to_owners;
    //Set read: 1, write:1 
    unordered_set<Virtual_Page*> &owners = swap_block_to_owners[page->block];

    //Copy on write
    if (write_flag and owners.size() > 1)
    {
        //Created new physical page and give the dirty bit to him
        Physical_Page * copy_on_write_ppage = create_phys_page(write_flag);
        //Copy the actual memory into there
        *copy_on_write_ppage = *new_phys_page;
        //Set the old guy to be not dirty since we know it was a read
        new_phys_page->dirty = false; 
        // copy on write
        owners.erase(page);
        //Page we just made should have no owners
        assert(ppage_to_owners.find(copy_on_write_ppage->ppage_location) == end(ppage_to_owners));
        // since this is the first vpage on this ppage we should create a new unordered set
        ppage_to_owners[copy_on_write_ppage->ppage_location] = unordered_set<Virtual_Page*>();
        ppage_to_owners[copy_on_write_ppage->ppage_location].insert(page);
        page->in_swap = false;
        page->in_clock = true;
        page->entry->ppage = copy_on_write_ppage->ppage_location;
        page->entry->read_enable = 1;
        page->entry->write_enable = 1;
    }

    for (auto virtual_page : owners)
    {
        // each virtual page needs to be aware that it has a swap block
        //  despite already being in swap_block_to_owners
        assert(virtual_page->in_swap);
        //If we are trying
        
        virtual_page->in_clock = true;
        virtual_page->entry->ppage = new_phys_page->ppage_location; 
        virtual_page->entry->read_enable = 1;
        virtual_page->entry->write_enable = write_flag;
    }
    assert(ppage_to_owners.find(new_phys_page->ppage_location) == end(ppage_to_owners));
    ppage_to_owners[new_phys_page->ppage_location] = owners;
    
    // propigate moving the swap file from swap to phys


}


void handle_fault_file(bool write_flag, Virtual_Page* page)
{
    /*
        this could mean:
        the file is not in phyiscal memory, we need to bring the 
        block into physical memory via file_read, set the write_flag=1
        we also need to propigate to all of the owners of this file 
        that this block is now in physical memory but ONLY the
        people that share this block      
    */
    
    
    Physical_Page * new_ppage = create_phys_page(write_flag);

    file_read(page->filename.data(), page->block, (void*) (uintptr_t(vm_physmem) + (VM_PAGESIZE * new_ppage->ppage_location)));

    assert(ppage_to_owners.find(new_ppage->ppage_location) == end(ppage_to_owners));

    ppage_to_owners[new_ppage->ppage_location].insert(page);

    assert(filename_to_owners.find(page->filename) != end(filename_to_owners));
    assert(filename_to_owners[page->filename].find(page) != end(filename_to_owners[page->filename]));

    unordered_set<Virtual_Page*>& block_owners = filename_to_owners[page->filename];

    // loop over everyone that owns the same block as page in the same file and tell them
    // that they are now in phys mem and add them as owners of this physical page

    for (auto& vpage_owner : block_owners)
    {
        if (vpage_owner->block == page->block)
        {
            vpage_owner->in_clock = true;
            vpage_owner->entry->ppage = new_ppage->ppage_location;
            vpage_owner->entry->read_enable = 1;
            vpage_owner->entry->write_enable = write_flag;
            ppage_to_owners[new_ppage->ppage_location].insert(vpage_owner);
        }
        assert(vpage_owner->in_swap == false);
    }

}

void handle_fault_zero(bool write_flag, Virtual_Page* page)
{
    assert(page->filename == DEFAULT_FILENAME);
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

std::string read_ppage_for_filename(const void * start_addr, const void * end_addr, bool& found_end)
{
    /*

        start at the start_addr in physmem, loop until vm addr % 4096 == 0
        OR found a null terminator

        if addr & 4096 == 0, found_end == false return str;       
        ((char *)vm_physmem)
    
    */
   std::string filename;

    while (*((char *)start_addr) != '\0' and start_addr != end_addr)
    {
        filename.push_back(*((char *)start_addr));
        start_addr = (void *)(uintptr_t(start_addr) + 1);
    }

    found_end = (*((char *)start_addr) == '\0');

    return filename;   
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
    /* */
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
        else if (owner_set.first != 0)
        {
            return false;
        }
    }

    return true;
}

bool assert_swap_block_owners_sync()
{
    for(const auto& block_owner_pair : swap_block_to_owners){
        const auto first_vp = *begin(block_owner_pair.second);

        for(const auto vp : block_owner_pair.second){
            if(!(*vp == *(first_vp)))
            {
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

bool assert_arenas_sync(Arena* a1, Arena* a2)
{
    if (a1->virtual_pages.size() != a2->virtual_pages.size())
    {
        return false;
    }

    if (a1->valid_arena_bound != a2->valid_arena_bound)
    {
        return false;
    }

    // 1: 0x1, 0x2
    // 2: 0x1
    for (auto a1_it = a1->virtual_pages.begin(); 
    a1_it != end(a1->virtual_pages); ++a1_it)
    {
        unsigned int entry1 = a1_it->first;
        if (a2->virtual_pages.find(entry1) == end(a2->virtual_pages))
        {
            return false;
        }
        if (*(a2->virtual_pages[entry1]) == *(a1_it->second));
        
    }
    return true;
}

bool assert_clock_maintain_size()
{
    if (clock_maxed_flag)
    {
        return mem_clock.size() + 1 == MAX_PHYSICAL_MEM_SIZE 
            and ppage_in_clock.size() + 1 == MAX_PHYSICAL_MEM_SIZE;
    }
    return true;
}

//Copy on write
bool assert_correct_write_flag()
{
    for (const auto& arena : pid_to_arena)
    {
        for (const auto virtual_page : arena.second->virtual_pages)
        {
            if (virtual_page.second->in_clock)
            {
                if (ppage_in_clock.find(virtual_page.second->entry->ppage) == end(ppage_in_clock))
                {
                    return false;
                }
                if (ppage_to_owners.find(virtual_page.second->entry->ppage) == end(ppage_to_owners))
                {
                    return false;
                }
                if (virtual_page.second->entry->read_enable == 1 and
                    ppage_in_clock[virtual_page.second->entry->ppage]->dirty == 1 and 
                    virtual_page.second->entry->write_enable == 0 and
                    ppage_to_owners[virtual_page.second->entry->ppage].size() == 1)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool assert_filenames_exist()
{
    /*
        if a virutal page has a filename that is file backed then it better 
        have a corresponding filename and it also better be an owner of that file
        in the filename_to_owners structure
    */

   for (const auto& arena : pid_to_arena)
   {
       for (const auto & virtual_page : arena.second->virtual_pages)
       {
           // if a virtual page has a filename, that filename better be in the filename to owners 
           if (virtual_page.second->filename != DEFAULT_FILENAME 
           and filename_to_owners.find(virtual_page.second->filename) == end(filename_to_owners))
           {
               return false;
           }

           // if there is a filename that matches the virtual page filename, it better be an owner of this filename
           if (virtual_page.second->filename != DEFAULT_FILENAME 
           and filename_to_owners.find(virtual_page.second->filename) != end(filename_to_owners)
           and filename_to_owners[virtual_page.second->filename].find(virtual_page.second) 
                == end(filename_to_owners[virtual_page.second->filename]))
           {
               return false;
           }
       }
   }
   return true;
}

bool assert_flag_correctness()
{
    for (const auto& arena : pid_to_arena)
    {
        for (const auto& virtual_page : arena.second->virtual_pages)
        {
            //If its file backed
            auto& entry = virtual_page.second->entry;
            if(virtual_page.second->filename != DEFAULT_FILENAME)
            {
                // just make sure that you are in the clock properly
                if (virtual_page.second->in_clock and ppage_to_owners.find(entry->ppage) == end(ppage_to_owners))
                {
                    return false;
                }
                // i better own the physical page that i say i do
                if (virtual_page.second->in_clock and 
                ppage_to_owners[entry->ppage].find(virtual_page.second) == 
                            end(ppage_to_owners[entry->ppage]))
                {
                    return false;
                }

                if (entry->read_enable == 1 and entry->write_enable != ppage_in_clock[entry->ppage]->dirty)
                {
                    return false;
                }
            }   

            //for both
            if( virtual_page.second->entry->read_enable == 0
                and virtual_page.second->entry->write_enable == 1){
                return false;
            }

        }
    }
    return true;
}

bool assert_fileback_not_in_swap(){
    
    for(const auto& arena : pid_to_arena){
        for (const auto& pair : arena.second->virtual_pages)
        {
            auto virtual_page = pair.second;
            //File backed
            if(virtual_page->filename != DEFAULT_FILENAME){
                if(virtual_page->in_swap == true){ return false;}
                //Make sure i cant find you in any swapbacked maps
                if(swap_block_to_owners.find(virtual_page->block) != end(swap_block_to_owners)
                and swap_block_to_owners[virtual_page->block].find(virtual_page) != end(swap_block_to_owners[virtual_page->block]))
                {
                    return false;
                }
            }
            //Swap backed
            if(virtual_page->in_swap == true){
                if(virtual_page->filename != DEFAULT_FILENAME) {return false;}
                if(filename_to_owners.find(virtual_page->filename) != end(filename_to_owners)
                and filename_to_owners[virtual_page->filename].find(virtual_page) != end(filename_to_owners[virtual_page->filename]))
                {
                    return false;
                }
            }
            
        }
    }
    return true;
}

bool assert_out_of_clock_flags()
{
    /*
        make sure that that if a virtual page says it is not in the clock
        that its flags are R:0 W:0 
    
    */

   for (const auto & arena : pid_to_arena)
   {
       for (const auto& virtual_page : arena.second->virtual_pages)
       {
           if (!virtual_page.second->in_clock and virtual_page.second->entry->ppage != 0)
           {
                if (!(virtual_page.second->entry->read_enable == 0 
                and virtual_page.second->entry->write_enable == 0))
                {
                    return false;
                }
           }
       }
   }

   return true;
}

bool assert_ppage_owner_type_correcness()
{
    /*
        go through every single ppage, make sure that all of its owners
        are either swap backed or file backed and never both
    */

   bool file_backed = false;
   bool swap_backed = false;

   for (const auto & owners : ppage_to_owners)
   {
       for (const auto & vpage : owners.second)
       {
           swap_backed = swap_backed or vpage->filename == DEFAULT_FILENAME;
           file_backed = file_backed or vpage->filename != DEFAULT_FILENAME;

           if (swap_backed and file_backed)
           {
               return false;
           }
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

    assert(assert_clock_maintain_size());
    // if we have hit the clock limit (flag) make sure
    // that clock_size == max_limit

    assert(assert_correct_write_flag());
    
    //Assert if filename is DEFAULT_FILENAME you can only be r:1w:1 or r:0w:0
    //or if swap back you are not r:0w:1
    assert(assert_flag_correctness());

    // assert that you are not in_swap
    // assert that if you are file backed that your file exsists in the filename_to_owners (full regression)
    assert(assert_filenames_exist());

    assert(assert_out_of_clock_flags());

    assert(assert_fileback_not_in_swap());
}
#endif /* _VM_PAGER_H_ */
