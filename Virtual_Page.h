#ifndef _vp_H_
#define _vp_H_

#include "vm_pager.h"

class Arena;

class Virtual_Page
{
    public:
    //Creating fresh page for vm_create
    Virtual_Page();
    Virtual_Page(unsigned int block_, bool in_swap_, bool in_clock_, 
    unsigned int vp_num, Arena *arena);    
    bool operator==(const Virtual_Page& rhs);
    Virtual_Page& operator=(const Virtual_Page& rhs)
    {
        this->entry = rhs.entry;
        in_swap = rhs.in_swap;
        in_clock = rhs.in_clock;
        block = rhs.block;
        return *this;
    }
    ~Virtual_Page();
    page_table_entry_t* entry;
    //Never the case where shared virtual pages have conflicting blocks
    unsigned int block;
    bool in_swap; // for evictions (for the first appearance), if dirty=0 and has_blo
    bool in_clock;
};

#endif /* _VM_PAGER_H_ */
