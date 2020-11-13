/*1.) Swap back not shared
Possible states
                                            1        points to zero; R;1,W;1 in_swap;false block; invald
                                            2        On write R;1, W;1, ppage :Dirty true in_swap;false block;invalid
                                            3        on clock R;0 W;0 Dirty true (ON READ/WRITE GOES BACK TO TOP) two cases
                                            4        Evicted R;0 W;0 in_swap;true block;valid #
5    on read R;1 W;0 in_swap;true block;valid # Dirty; false                                    6 on write R;1 W;1 in_swap;true block;valid # Dirty; true
7    on clock R;0 W;0 in_swap;true block;valid # Dirty false (ON WRITE GOES BACK TO TOP)        8 on clock R;0 W;0 in_swap;true block;valid # Dirty true (ON WRITE GOES BACK TO TOP)
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

    pid_t pid;

    pid = fork();
    
    if (pid)
    {
        vm_yield();
        {
            char *v = (char *) vm_map(nullptr, 0);
            // at stage 2 in memory
            strcpy(v, "randomtext.txt");
        }
    
        //Points to zero
        char *page9read = (char *) vm_map(nullptr, 0);
        // at stage 2 in memory
        strcpy(page9read, "randomtext.txt");
        
        //eviction
        for(int i = 0; i < 2; ++i)
        {
            char *evict = (char *) vm_map(nullptr, 0);
            // at stage 2 in memory
            strcpy(evict, "evict");
        }
        
        // page9 is now: R:0,W:0 not referenced 
        //ON WRITE
        strcpy(page9read,"write on old");
    }
    else
    {
        {
        char *v = (char *) vm_map(nullptr, 0);
        // at stage 2 in memory
        strcpy(v, "randomtext.txt"); 
        }
        
        //Points to zero
        char *page9read = (char *) vm_map(nullptr, 0);
        // at stage 2 in memory
        strcpy(page9read, "randomtext.txt");
        
        //eviction
        for(int i = 0; i < 2; ++i)
        {
            char *evict = (char *) vm_map(nullptr, 0);
            // at stage 2 in memory
            strcpy(evict, "evict");
        }
        // page9 is now: R:0,W:0 not referenced 
        //ON READ
        cout << page9read;
    }
    
}