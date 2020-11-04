#include "vm_pager.h"

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


void zero_out_ppage(unsigned int ppage)
{
    for(int i = ppage; i < VM_PAGESIZE; ++i) {
        ((char *)vm_physmem)[i] = 0;
    }
}

unsigned int get_virtual_page_num(void* addr)
{
    unsigned int address = (unsigned int) addr;
    unsigned int mask = 0xFFFFFFFFFFFFF000;

    return address & mask;
}