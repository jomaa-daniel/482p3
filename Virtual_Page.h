#pragma once
#include "vm_pager.h"

class Arena;

class Virtual_Page
{
    public:
    //Creating fresh page for vm_create
    Virtual_Page();
    Virtual_Page(unsigned int block_, bool in_swap_, bool in_clock_, 
    unsigned int ppage_entry, bool read_enable, 
    bool write_enable, Arena *arena);    
    bool operator==(const Virtual_Page& rhs);
    ~Virtual_Page();
    page_table_entry_t* entry;
    //Never the case where shared virtual pages have conflicting blocks
    unsigned int block;
    bool in_swap; // for evictions (for the first appearance), if dirty=0 and has_blo
    bool in_clock;
};