#include "Virtual_Page.h"
#include "globals.h"
#include "utils.h"

Virtual_Page::Virtual_Page() : block(0), in_swap(false), in_clock(false) 
{
    //iterate valid_arena_bound // TODO this does not look right, 
    if (current_arena->valid_arena_bound + VM_PAGESIZE > 
        uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE ||
        current_arena->valid_arena_bound < uintptr_t(VM_ARENA_BASEADDR))
    { 
        throw NotEnoughVirtualMemory();
    }
    //Eager Swap reservation (Make sure virtual page has enough space in swap file)
    if(available_swap_blocks > 0)
    {
        //Iterate swap space
        --available_swap_blocks;
    }
    else{
        throw NotEnoughSwapBlocks();
    }
    uintptr_t vp_num = get_virtual_page_num((void *) current_arena->valid_arena_bound);
        //uintptr_t vp_num =  current_arena->valid_arena_bound - VM_ARENA_BASEADDR;

    entry = &current_arena->page_table.ptes[vp_num];
    
    //Make sure to set entry to be appropriate for ppage 0.
    entry->ppage = 0;
    entry->read_enable = 1;
    entry->write_enable = 0;

    //Store in arenas virtual page map
    current_arena->virtual_pages[vp_num] = this;

    //Update ppage_to_owner map/ so this virtual page owns 0 page
    if(ppage_to_owners.find(0) == ppage_to_owners.end())
    {
        ppage_to_owners[0] = std::unordered_set<Virtual_Page*>();
    }

    ppage_to_owners[0].insert(this);

    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
        current_arena->valid_arena_bound + VM_PAGESIZE;
}

Virtual_Page::Virtual_Page(unsigned int block_, bool in_swap_, bool in_clock_, 
unsigned int vp_num, Arena *arena)
: block(block_), in_swap(in_swap_), in_clock(in_clock_) 
{
    //iterate valid_arena_bound ????
    
    if (current_arena->valid_arena_bound + VM_PAGESIZE > 
        uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE ||
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
    //Check vp_num matches the copied pages vp_num ASSERT
    entry = &arena->page_table.ptes[vp_num];

    //Store in arenas virtual page map
    arena->virtual_pages[vp_num] = this; 
    
    //Update ppage_to_owner map/ so this virtual page owns the same ppage 
    //assert(ppage_to_owners.find(ppage_entry) == ppage_to_owners.end())
    if(in_clock || this->entry->ppage == 0){
        //Update ownership of ppage
        ppage_to_owners[this->entry->ppage].insert(this);
    }
    //Update swap_to_owner map/ so this virtual page owns the same swap 
    if(in_swap)
    {               
        //This swap block must have 1 owner namely the source virtual page
        swap_block_to_owners[block].insert(this);
    }
        
    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
        current_arena->valid_arena_bound + VM_PAGESIZE;
    
} 
bool Virtual_Page::operator==(const Virtual_Page &rhs){

    bool equal = (
    this->in_swap == rhs.in_swap
    and this->in_clock == rhs.in_clock 
    and this->entry->ppage == rhs.entry->ppage
    and this->entry->read_enable == rhs.entry->read_enable
    and this->entry->write_enable == rhs.entry->write_enable);

    if(this->in_swap and rhs.in_swap and this->block != rhs.block){
        return false;
    }
    return equal;
}


Virtual_Page::~Virtual_Page()
{
    //Since we allocate swap blocks for every virtual page, we need to reclaim 
    // on the way out
    ++available_swap_blocks;
    
    if(this->in_swap){
            //Check this block is owned by someone
            assert(swap_block_to_owners.find(this->block) != swap_block_to_owners.end());
            //Check this block is owned by this virtual page
            assert(swap_block_to_owners[this->block].find(this) != 
                swap_block_to_owners[this->block].end());   

            // remove this virtual page as an owner of this swap block
            swap_block_to_owners[this->block].erase(this);
           
            //Give space back to swap file
            if(swap_block_to_owners[this->block].empty()){
                swap_block_to_owners.erase(this->block);
            }
        }
        
        
    if(this->in_clock or this->entry->ppage == 0)
    {
        // this == 0x7ffffffeda20
        // this->entry == 0x8447008 -> R1 w0 block0 pp0 false false
        // [0] : "0x8447008"
            //Check if this ppage is owned by someone
            assert(ppage_to_owners.find(this->entry->ppage) != ppage_to_owners.end());
            assert(ppage_to_owners[this->entry->ppage].find(this) != end(ppage_to_owners[this->entry->ppage]));
            auto vp =  ppage_to_owners[this->entry->ppage].find(this);
            //Check this ppage is owned by this virtual page
            assert(*(ppage_to_owners[this->entry->ppage].find(this)) == this);


            //Dont evict a page just replace this one on next vm_fault
            if(ppage_to_owners[this->entry->ppage].size() == 1 
            and this->entry->ppage != 0)
            {
                for(auto pp = mem_clock.begin(); pp != mem_clock.end(); ++pp){
                    if((*pp)->ppage_location == this->entry->ppage){
                        mem_clock.erase(pp);
                        ppage_in_clock.erase(this->entry->ppage);
                        break;
                    }              
                } 
            }
            //Remove this virtual page as an owner of this swap block
            ppage_to_owners[this->entry->ppage].erase(this);

            // if there is no more owners, then remove the empty set
            if (ppage_to_owners[this->entry->ppage].empty())
            {
                ppage_to_owners.erase(this->entry->ppage);
            }
        }


}