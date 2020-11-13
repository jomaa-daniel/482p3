/*1.) Swap back not shared
Possible states
                                            1        points to zero; R;1,W;1 in_swap;false block; invald
                                            2        On write R;1, W;1, ppage :Dirty true in_swap;false block;invalid
                                            3        on clock R;0 W;0 Dirty true (ON READ/WRITE GOES BACK TO TOP) two cases
                                            4        Evicted R;0 W;0 in_swap;true block;valid #
5    on read R;1 W;0 in_swap;true block;valid # Dirty; false                                    6 on write R;1 W;1 in_swap;true block;valid # Dirty; true
7    on clock R;0 W;0 in_swap;true block;valid # Dirty false (ON READ/WRITE GOES BACK TO TOP)        8 on clock R;0 W;0 in_swap;true block;valid # Dirty true (ON WRITE GOES BACK TO TOP)
9    Evicted R;0 W;0 in_swap;true block;valid # (WE DO NOT NEED TO PERFORM A WRITE BACK)         10 Evicted R;0 W;0 in_swap;true block;valid # (WE NEED TO PERFORM A WRITE BACK)

3.) Swap backed shared
*/



#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{

    
    
    //vm_yield();

    //Points to zero
    char *page9read = (char *) vm_map(nullptr, 0);
    // at stage 2 in memory
    strcpy(page9read, "randomtext.txt");
    
    //eviction
    for(int i = 0; i < 3; ++i)
    {
        char *evict = (char *) vm_map(nullptr, 0);
        // at stage 2 in memory
        strcpy(evict, "evict");
    }
    
    
    {
        char *evict = (char *) vm_map(nullptr, 0);
            // at stage 2 in memory
        strcpy(evict, "evict");
    }
    //Read an evicted file bring him to 5

    
    //Make old to 7
    strcpy(page9read, "something new");

    for(int i = 0; i < 3; ++i)
    {
        char *evict = (char *) vm_map(nullptr, 0);
        strcpy(evict, "evict");
    }
    //Write on an old out of a swap block back to 6  
    cout << page9read;
}
    // pid_t pid;

    // pid = fork();

    // if (pid)
    // {

    // }
    // else
    // {
        
    // }
    
    // page9 is now: R:0,W:0 not in memory in clock is false and he is in swap 
    //ON WRITE ON EVICTION