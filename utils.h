#ifndef _utils_H_
#define _utils_H_

#include "vm_pager.h"
#include "vm_arena.h"
#include "globals.h"
#include <cassert>

extern unsigned int current_available_block;


struct NotEnoughVirtualMemory : public std::exception
{
    const char * what() const throw ()
    {
        return "Not enough virtual memory";
    }
};

struct NotEnoughSwapBlocks : public std::exception
{
    const char * what() const throw ()
    {
        return "Eager Swap reservation doesn't have enough swap blockss";
    }
};

void handle_last_owner(Physical_Page& old_phys_page);

void zero_out_ppage(unsigned int ppage);

unsigned int get_virtual_page_num(const void* addr);

unsigned int find_open_block();

unsigned int find_ppage();

bool check_ppage_owners_age(Physical_Page *phys_page);

// keeps pushing and popping till it finds something to evict
// returns the pointer to a victim
// should we delete here and return a physical page on the stack?
Physical_Page find_victim();

// allocates and adds new physical page to the clock
// make sure to keep parallel with the map
Physical_Page* add_new_page_to_clock(unsigned int ppage_location, bool write_flag);

// TAKE NOTE THAT THERE IS A POSSIBILITY THAT THE PHYSICAL PAGE HAS NO OWNERS
//  they might have been dealloc while still having v_pages pointing to phys pages
// in the clock
void evict_phys_page(Physical_Page& page);


void propigate_referenced(bool write_flag, Virtual_Page* page);

bool not_valid_addr(uintptr_t addr);

void handle_fault_clock(uintptr_t addr, bool write_flag);

void handle_fault_swap(bool write_flag, Virtual_Page* page);

void handle_last_owner(Physical_Page* old_phys_page);

void handle_fault_clock(bool write_flag, Virtual_Page* page);

void handle_fault_zero(bool write_flag, Virtual_Page* page);

void write_to_swap(Virtual_Page* page);

Physical_Page* create_phys_page(bool write_flag);

/////////////////// ASSERT FUNCTIONS //////////////////

// assert variables:

extern bool clock_maxed_flag;

void assert_all();

// make sure this accounts for the zero page
bool assert_clock_and_pages_sync();

bool assert_swap_and_pages_sync();

bool assert_ppage_owners_sync();

bool assert_swap_block_owners_sync();

bool assert_clock_map_match();

bool assert_no_write_on_share();

bool assert_same_ppage_same_block();

bool assert_no_dirty_pages(Physical_Page* page);

bool assert_ppage_map_correctness();

bool assert_swap_map_correctness();

bool assert_zero_page_correctness();

bool assert_valid_swap_owners();

bool assert_arenas_sync(Arena* a1, Arena* a2);

bool assert_clock_maintain_size();

bool assert_correct_write_flag();

bool assert_filenames_exist();

bool assert_flag_correctness();


#endif /* _VM_PAGER_H_ */
