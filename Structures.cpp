#include "globals.h"


// NOTE ONLY WRITEBACK WHEN NEVER IN SWAP I.E. virtual_to_swap returns null OR dirty
std::deque<Physical_Page*> mem_clock;
//Not required for correctness
std::unordered_map<unsigned int, Physical_Page*> ppage_in_clock; 

pid_t current_running_pid = 0;
Arena* current_arena = nullptr;
std::unordered_map<pid_t, Arena*> pid_to_arena;
std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> swap_block_to_owners;
std::unordered_map<unsigned int , std::unordered_set<Virtual_Page *>> ppage_to_owners;

unsigned int available_swap_blocks;
unsigned int current_available_block = 0;
unsigned int MAX_SWAP_CAPACITY;
unsigned int MAX_PHYSICAL_MEM_SIZE;