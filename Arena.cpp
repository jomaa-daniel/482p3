#include "vm_arena.h"
#include "Arena.h"

Arena::Arena()
{
    for (unsigned int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; ++i)
    {
        page_table_entry_t &e = page_table.ptes[i];
        e.ppage = e.read_enable = e.write_enable = 0;
    }        
}

Arena::Arena(Arena& other)
{ 

    for (unsigned int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; ++i)
    {
        page_table_entry_t & p = other.page_table.ptes[i];
        page_table_entry_t & c = page_table.ptes[i];
        c.read_enable = p.read_enable;
        //Child must always copy on write so it must be write disabled
        c.write_enable = 0;
        //Parent must always copy on write so it must be write disabled
        p.write_enable = 0;

        c.ppage = p.ppage;
    }

    for (auto & pair : other.virtual_pages)
    {

        // Allocate a new page and increments the address bound in its arena
        // Updates ownership maps based on source virtual page
        // Updates which block and ppage it now shares
        new Virtual_Page
        (
            pair.second->block,
            pair.second->in_swap,
            pair.second->in_clock,
            pair.second->entry->ppage,
            pair.second->entry->read_enable,
            pair.second->entry->write_enable,
            this
        ); 
    }

}


Arena::~Arena()
{
    for (auto& pair: virtual_pages)
    {   
        delete pair.second;
    }
}