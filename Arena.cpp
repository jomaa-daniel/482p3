#pragma once
#include "globals.h"


Arena::Arena()
{
    for (int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; ++i)
    {
        page_table_entry_t &e = page_table.ptes[i];
        e.ppage = e.read_enable = e.write_enable = 0;
    }        
}

Arena::Arena(Arena& other)
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

            Virtual_Page * new_page = new Virtual_Page
            (
                pair.second->block,
                pair.second->in_swap,
                pair.second->in_clock,
                pair.second->entry->ppage,
                pair.second->entry->read_enable,
                pair.second->entry->write_enable,
                this
            ); 

            // make sure that the virtual page that we are copying actually exists in the 
            // swap file
            
            // assign this new page as an owner of the corresponding swap block
            if(pair.second->in_swap)
            {
                
                //Iterate and check that the swap block is owned by the source virtual page
                assert([](auto& p)->bool
                {
                   for(auto vp : swap_block_to_owners[p.second->block])
                   {
                       if (p.second == vp) {return true;}
                   }
                   return false;     
                }(pair));
                
                //This swap block must have 1 owner namely the source virtual page
                swap_block_to_owners[pair.second->block].insert(new_page);
            }

            // assign this new page as an owner of this physical page
            // get the physical address of this virtual page by
            // pointer artithmetic 

            if(pair.second->in_clock || pair.second->entry->ppage == 0)
            {
                //Doesn't work since we are making and remaking physical pages
                // assert([&](auto& p)->bool
                // {
                //    for(auto* pp : mem_clock)
                //    {
                //        if (p.second->entry->ppage == pp->ppage_location)
                //         {return true;}
                //    }
                //    return false;     
                // }(pair));

                ppage_to_owners[pair.second->entry->ppage].insert(new_page);
            }
        }
    }


Arena::~Arena()
{
    for (auto& pair: virtual_pages)
    {
        if(pair.second->in_swap){
            //Check this block is owned by someone
            assert(swap_block_to_owners.find(pair.second->block) != end(swap_block_to_owners));
            //Check this block is owned by this virtual page
            assert(*(swap_block_to_owners[pair.second->block].find(pair.second)) == pair.second);

            if(swap_block_to_owners[pair.second->block].size() == 1){
                ++available_swap_blocks;
            }
            // remove this virtual page as an owner of this swap block
            swap_block_to_owners[pair.second->block].erase(pair.second);

        }
        //Make sure clock isnt checking for owners
        if(pair.second->in_clock || pair.second->entry->ppage == 0)
        {
            //Check if this ppage is owned by someone
            assert(ppage_to_owners.find(pair.second->entry->ppage) != end(ppage_to_owners));
            //Check this ppage is owned by this virtual page
            assert(*(ppage_to_owners[pair.second->entry->ppage].find(pair.second)) == pair.second);
            ppage_to_owners[pair.second->entry->ppage].erase(pair.second);
        }
        
        delete pair.second;
    }
}