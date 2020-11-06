#include <deque>
#pragma once
#include <unordered_map>
#include <vector>
#include "vm_pager.h"
#include "util.h"
#include <iostream>
#include <cassert>

const int DEBUG = 1;

class Virtual_Page
{
    public:
    //Creating fresh page for vm_create
    Virtual_Page()
        : block(0), in_swap(false), in_clock(false) 
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
            if(ppage_to_owners.find(0) == end(ppage_to_owners))
            {
                ppage_to_owners[0] = std::vector<Virtual_Page*>();
            }
    
            ppage_to_owners[0].push_back(this);

            //Iterate arena bound for next page
            current_arena->valid_arena_bound = 
            current_arena->valid_arena_bound + VM_PAGESIZE - 1;
        }

        Virtual_Page(unsigned int block_, bool has_block_, unsigned int ppage_entry, bool read_enable, bool write_enable, Arena *arena)
        : block(block_), in_swap(has_block_) 
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
            if(ppage_to_owners.find(ppage_entry) == end(ppage_to_owners))
            {
                ppage_to_owners[ppage_entry] = std::vector<Virtual_Page*>();
            }
    
               
            ppage_to_owners[ppage_entry].push_back(this);

            //Iterate arena bound for next page
            current_arena->valid_arena_bound = 
                current_arena->valid_arena_bound + VM_PAGESIZE;
            
        }       
        
    page_table_entry_t* entry;
    //Never the case where shared virtual pages have conflicting blocks
    unsigned int block;
    bool in_swap; // for evictions (for the first appearance), if dirty=0 and has_blo
    bool in_clock;
};

struct Physical_Page {
    unsigned int ppage_location;
    bool dirty;

    Physical_Page(unsigned int ppage_location_, bool dirty_) : ppage_location(ppage_location_),
        dirty(dirty_) {}
    
    Physical_Page(Physical_Page& other) : ppage_location(other.ppage_location),
        dirty(other.dirty){}
    
    Physical_Page() : ppage_location(0), dirty(false) {}
};



class Arena
{
    public:
    // virtual page base address index maps to the virtual page struct
    std::unordered_map<unsigned int, Virtual_Page* > virtual_pages;
    page_table_t page_table;
    bool unique = false;
    uintptr_t valid_arena_bound = 0x600000000;
    Arena(Arena& other)
    { 

        for (int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; ++i)
        {
            page_table_entry_t & p = other.page_table.ptes[i];
            page_table_entry_t & c = page_table.ptes[i];
            c.read_enable = p.read_enable;
            c.write_enable = p.write_enable;
            c.ppage = p.ppage;
        }

        for (auto & pair : other.virtual_pages)
        {
            // alloc new virt page;
            //Index into this page_table and 
            // set eq to pair.second.block

            // this needs to allocate a new page and increment the address
            Virtual_Page * new_page = new Virtual_Page(&(page_table.ptes[pair.first]),
                pair.second->block, pair.second->in_swap); 
            virtual_pages[pair.first] = new_page;

            // make sure that the virtual page that we are copying actually exists in the 
            // swap file
            assert(pair.second->in_swap and 
            swap_block_to_owners.find(pair.first) != end(swap_block_to_owners));
            
            // assign this new page as an owner of the corresponding swap block
            if(pair.second->in_swap){
                swap_block_to_owners[pair.second->block].push_back(new_page);
            }

            // assign this new page as an owner of this physical page
            // get the physical address of this virtual page by
            // pointer artithmetic 
            if(pair.second->in_clock)
            {
                ppage_to_owners[pair.second->entry->ppage].push_back(new_page);
            }
        }
    }

    Arena()
    {
        for (int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; ++i)
        {
            page_table_entry_t &e = page_table.ptes[i];
            e.ppage = e.read_enable = e.write_enable = 0;
        }        
    }

    ~Arena()
    {
        for (auto& page: virtual_pages)
        {
            delete page.second;
        }
    }
};

// NOTE ONLY WRITEBACK WHEN NEVER IN SWAP I.E. virtual_to_swap returns null OR dirty
std::deque<Physical_Page*> mem_clock;
//Not required for correctness
std::unordered_map<unsigned int, Physical_Page*> ppage_in_clock; 

pid_t current_running_pid = 0;
Arena* current_arena = nullptr;
std::unordered_map<pid_t, Arena*> pid_to_arena;
std::unordered_map<unsigned int , std::vector<Virtual_Page *>> swap_block_to_owners;
std::unordered_map<unsigned int , std::vector<Virtual_Page *>> ppage_to_owners;


unsigned int available_swap_blocks;
unsigned int MAX_SWAP_CAPACITY;
unsigned int MAX_PHYSICAL_MEM_SIZE;


