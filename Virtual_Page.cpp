#include "Virtual_Page.h"
#include "globals.h"
#include "utils.h"



void check_virtual_page_bound(Arena* arena, std::string& filename)
{
    
    if (arena->valid_arena_bound + VM_PAGESIZE > 
        uintptr_t(VM_ARENA_BASEADDR) + VM_ARENA_SIZE ||
        arena->valid_arena_bound < uintptr_t(VM_ARENA_BASEADDR))
    { 
        throw NotEnoughVirtualMemory();
    }

    if (filename == DEFAULT_FILENAME)
    {
        if(available_swap_blocks > 0){
            //Iterate swap space
            --available_swap_blocks;
        }
        else{
            throw NotEnoughSwapBlocks();
        } 
    }
    //Eager Swap reservation (Make sure virtual page has enough space in swap file)
}

//Default constructor for a 0 page.
Virtual_Page::Virtual_Page() :  filename(DEFAULT_FILENAME), block(0), in_swap(false), in_clock(false) 
{
    check_virtual_page_bound(current_arena, filename);

    uintptr_t vp_num = get_virtual_page_num((void *) current_arena->valid_arena_bound);
        

    entry = &current_arena->page_table.ptes[vp_num];

 
    //else:
    //Make sure to set entry to be appropriate for ppage 0.
    entry->ppage = 0;
    entry->read_enable = 1;
    entry->write_enable = 0;

    //Store in arenas virtual page map 
    //Protection against overrides
    assert(current_arena->virtual_pages.find(vp_num) == end(current_arena->virtual_pages));
    current_arena->virtual_pages[vp_num] = this; // i think the issue is right here

    //Update ppage_to_owner map/ so this virtual page owns 0 page
    if(ppage_to_owners.find(entry->ppage) == ppage_to_owners.end())
    {
        ppage_to_owners[entry->ppage] = std::unordered_set<Virtual_Page*>();
    }

    ppage_to_owners[entry->ppage].insert(this);

    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
        current_arena->valid_arena_bound + VM_PAGESIZE;
}

Virtual_Page::Virtual_Page(char *filename_, unsigned int file_block_) : filename(filename_), block(file_block_) 
{

    check_virtual_page_bound(current_arena, filename);

    uintptr_t vp_num = get_virtual_page_num((void *) current_arena->valid_arena_bound);

    entry = &current_arena->page_table.ptes[vp_num];

    // create a new physical page
    Physical_Page * new_page = create_phys_page(true);
    
    //Make sure to set entry to be appropriate for ppage 0.
    entry->ppage = new_page->ppage_location;
    entry->read_enable = 1;
    entry->write_enable = 1;
    
    this->in_clock = true;

    assert(current_arena->virtual_pages.find(vp_num) == end(current_arena->virtual_pages));
    current_arena->virtual_pages[vp_num] = this;

    //Update ppage_to_owner map/ so this virtual page owns 0 page
    if(ppage_to_owners.find(entry->ppage) == ppage_to_owners.end())
    {
        ppage_to_owners[entry->ppage] = std::unordered_set<Virtual_Page*>();
    }

    ppage_to_owners[entry->ppage].insert(this);

    // update the file to owner such that this virutal page is a new owner of the file
    if (filename_to_owners.find(filename) == end(filename_to_owners))
    {
        filename_to_owners[filename] = std::unordered_set<Virtual_Page*>();
    }
    
    filename_to_owners[filename].insert(this);

    //Finally preform read so that memory actually exists in vm_phys_mem
    file_read(filename.data(), block, (void *) ( uintptr_t(vm_physmem) + (VM_PAGESIZE * entry->ppage)));


    //Iterate arena bound for next page
    current_arena->valid_arena_bound = 
        current_arena->valid_arena_bound + VM_PAGESIZE;

}

Virtual_Page::Virtual_Page(unsigned int block_, bool in_swap_, bool in_clock_, std::string& otherfilename,
unsigned int vp_num, Arena *arena )
: filename (otherfilename) , block(block_), in_swap(in_swap_), in_clock(in_clock_)
{
    check_virtual_page_bound(arena, filename);

    //Check vp_num matches the copied pages vp_num ASSERT
    entry = &arena->page_table.ptes[vp_num];

    //Store in arenas virtual page map
    //Protection against overrides
    assert(arena->virtual_pages.find(vp_num) == end(arena->virtual_pages));
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
    if(filename != DEFAULT_FILENAME){
        filename_to_owners[filename].insert(this);
    }
        
    //Iterate arena bound for next page
    arena->valid_arena_bound = arena->valid_arena_bound + VM_PAGESIZE;
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
    

    // TODO NOTE: FILEBACKED PAGES
    // IF I AM THE LAST OWNER OF THE FILENAME, // WRITE TO THE FILE IF DIRTY AND THEN ERASE THE FILENAME FROM THE MAP

    // assert filename is in owner???
    if (this->filename != DEFAULT_FILENAME)
    {
        assert(filename_to_owners.find(this->filename) != end(filename_to_owners));
    }

    /*
        if i am file backed i need to:
            remove myself as owner of a filename
            remove the filename 
    */
    if (!this->filename.empty() and filename_to_owners.find(this->filename) != end(filename_to_owners)) // im filebacked
    {
        
        filename_to_owners[this->filename].erase(this);

        if (filename_to_owners[this->filename].empty())
        {
            filename_to_owners.erase(this->filename);
            
            //If you own physical memory and you are dirty you must write it back
            if(this->in_clock and ppage_in_clock[this->entry->ppage]->dirty == true)
            {
                file_write(this->filename.data(), this->block, (void *) ( uintptr_t(vm_physmem) + (VM_PAGESIZE * this->entry->ppage)));
            }
        }
    }
    else
    {
        //Since we allocate swap blocks for every virtual page, we need to reclaim 
        // on the way out
        ++available_swap_blocks;
    }

    if(this->in_swap)
    {
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
            //Check this ppage is owned by this virtual page
            assert(*(ppage_to_owners[this->entry->ppage].find(this)) == this);


            
            //Dont evict a page just replace this one on next vm_fault
            //Only run if you are the last person pointing to this physical page
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
            
            //Remove this virtual page as an owner of this ppage
            assert(ppage_to_owners[this->entry->ppage].find(this) != 
                end(ppage_to_owners[this->entry->ppage]));

            ppage_to_owners[this->entry->ppage].erase(this);
            
            //Only runs if there is 1 vp after you delete yourself
            if(ppage_to_owners[this->entry->ppage].size() == 1 
            and this->entry->ppage != 0)
            {
                assert(ppage_in_clock.find(this->entry->ppage) != end(ppage_in_clock));
                
                //Checks if there is 1 more owner and gives him write permission
                handle_last_owner(*ppage_in_clock[this->entry->ppage]);
            }

            // if there is no more owners, then remove the empty set
            if (ppage_to_owners[this->entry->ppage].empty() and 
                this->entry->ppage != 0)
            {
                ppage_to_owners.erase(this->entry->ppage);
            }
        }


}