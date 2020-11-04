#include <deque>
#pragma once
#include <unordered_map>
#include <vector>
#include "vm_pager.h"
#include "util.h"
#include <cassert>



struct Virtual_Page
{
    Virtual_Page(page_table_entry_t* entry_, unsigned int block_, bool has_block_)
        : entry(entry_), block(block_), has_block(has_block_) 
        {
            //iterate valid_arena_bound ????
            
            if (current_arena->valid_arena_bound + VM_PAGESIZE > VM_ARENA_SIZE ||
             current_arena->valid_arena_bound < uintptr_t(VM_ARENA_BASEADDR))
            { 
                throw NotEnoughVirtualMemory();
            }
            //Iterate arena bound 
            current_arena->valid_arena_bound = 
                current_arena->valid_arena_bound + VM_PAGESIZE - 1;

            //Eager Swap reservation (Make sure virtual page has enough space in swap file)
            if(available_swap_blocks > 0){
                //Iterate swap space
                --available_swap_blocks;
            }
            else{
                throw NotEnoughSwapBlocks();
            }
            
        }     
        
    page_table_entry_t* entry;
    //Never the case where shared virtual pages have conflicting blocks
    unsigned int block;
    bool has_block; // for evictions (for the first appearance), if dirty=0 and has_block
};

struct Physical_Page {
    R:0 W:0
    unsigned int ppage_location = 1;
    bool dirty = 0;
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
                pair.second->block, pair.second->has_block); 
            virtual_pages[pair.first] = new_page;

            // make sure that the virtual page that we are copying actually exists in the 
            // swap file
            assert(pair.second->has_block and 
            swap_to_owners.find(pair.first) != end(swap_to_owners));
            


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
std::deque<Physical_Page> clock;

pid_t current_running_pid = 0;
Arena* current_arena = nullptr;
std::unordered_map<pid_t, Arena*> pid_to_arena;
std::unordered_map<unsigned int , std::vector<Virtual_Page *>> swap_to_owners;
std::unordered_map<unsigned int , std::vector<Virtual_Page *>> ppage_to_owners;
unordered_set<unsigned int> pages_in_phys; 

unsigned int available_swap_blocks;
unsigned int MAX_PHYSICAL_MEM_SIZE;


