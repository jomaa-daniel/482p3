#ifndef _globals_H_
#define _globals_H_
#define NDEBUG
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
#include <string>
#include <utility>

using namespace std;

const int DEBUG = 0;

extern std::string DEFAULT_FILENAME;

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
        //The only time we copy physical pages is copy on write which means the page we wrote to must be write_enable
        this->dirty = 1;
        int base_lhs = this->ppage_location * VM_PAGESIZE;
        int base_rhs = rhs.ppage_location * VM_PAGESIZE;
        for (unsigned int i = 0; i < VM_PAGESIZE; ++i)
        {
            ((char *)vm_physmem)[i + base_lhs] = ((char *)vm_physmem)[base_rhs + i];
        }

        return *this;
    }
};

struct Ghost_Page
{
    std::string filename;
    unsigned int block;
    unsigned int referenced;
    Ghost_Page() : block(0), referenced(0) {}
    Ghost_Page(string& in_file, unsigned int block_, unsigned int referenced_)
     : filename(in_file), block(block_), referenced(referenced_) {}
    
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
extern std::unordered_map<std::string, std::unordered_set<Virtual_Page*>> filename_to_owners;
extern std::set<uintptr_t> accessed;
extern std::unordered_map<std::string, unordered_map<unsigned int, Physical_Page>> filename_to_ppage;
extern std::unordered_map<unsigned int, Ghost_Page> ppage_to_ghostpage;

extern unsigned int available_swap_blocks;
extern unsigned int current_available_block;
extern unsigned int MAX_SWAP_CAPACITY;
extern unsigned int MAX_PHYSICAL_MEM_SIZE;



#endif /* _VM_PAGER_H_ */
