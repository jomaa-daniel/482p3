#ifndef _globals_H_
#define _globals_H_
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "vm_pager.h"
// #include "utils.h"
#include <iostream>
#include <cassert>
#include "Virtual_Page.h"
#include "Arena.h"

using namespace std;

const int DEBUG = 0;


struct Physical_Page 
{
    unsigned int ppage_location;
    bool dirty;

    Physical_Page(unsigned int ppage_location_, bool dirty_) : ppage_location(ppage_location_),
        dirty(dirty_) {}
    
    Physical_Page(const Physical_Page& other) : ppage_location(other.ppage_location),
        dirty(other.dirty){}
    
    Physical_Page() : ppage_location(0), dirty(false) {}

    Physical_Page& operator=(const Physical_Page& rhs)
    {
        this->dirty = 1;
        
        for (unsigned int i = 0; i < VM_PAGESIZE; ++i)
        {
            ((char *)vm_physmem)[this->ppage_location + i] 
            = ((char *)vm_physmem)[rhs.ppage_location + i];
        }

        return *this;
    }
};

// NOTE ONLY WRITEBACK WHEN NEVER IN SWAP I.E. virtual_to_swap returns null OR dirty
extern std::deque<Physical_Page*> mem_clock;
//Not required for correctness
extern std::unordered_map<unsigned int, Physical_Page*> ppage_in_clock; 

extern pid_t current_running_pid;
extern Arena* current_arena;
extern std::unordered_map<pid_t, Arena*> pid_to_arena;
extern std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> swap_block_to_owners;
extern std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> ppage_to_owners;

extern unsigned int available_swap_blocks;
extern unsigned int current_available_block;
extern unsigned int MAX_SWAP_CAPACITY;
extern unsigned int MAX_PHYSICAL_MEM_SIZE;



#endif /* _VM_PAGER_H_ */






