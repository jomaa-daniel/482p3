#ifndef _arena_H_
#define _arena_H_

#include "vm_pager.h"
#include <unordered_map>
#include "Virtual_Page.h"

class Arena
{
    public:
    // virtual page base address index maps to the virtual page struct
    std::unordered_map<unsigned int, Virtual_Page* > virtual_pages;
    page_table_t page_table;
    bool unique = false;
    uintptr_t valid_arena_bound = 0x600000000;
    Arena();
    ~Arena();
    Arena(Arena& other);
    
};

#endif /* _VM_PAGER_H_ */
