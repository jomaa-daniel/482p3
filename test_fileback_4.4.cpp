/*1.) Swap back not shared
Possible states
                                            1        points valid; R;1,W;0 in_swap;false in_clock;true block; valid # Filename:Valid name Dirty; false
                                            2        (ON WRITE ONLY) points valid; R;1,W;1 in_swap;false in_clock;true block; valid # Filename:Valid name Dirty; true
    on clock R;0 W;0 Dirty: false (ON READ/WRITE GOES BACK TO TOP WRITE=DIRTYorWRITE_FLAG) two cases                    on clock R;0 W;0 Dirty: true (ON READ/WRITE GOES BACK TO TOP WRITE=DIRTYorWRITE_FLAG) two cases
                                            4        Evicted R;0 W;0 in_swap;false in_clock;false block; valid # Filename:Valid name (MUST WRITE IF DIRTY/ MUST NOT WRITE IF NOT DIRTY) 
5    on read R;1 W;0 in_swap;true block;valid # Dirty; false                                    6 on write R;1 W;1 in_swap;true block;valid # Dirty; true
7    on clock R;0 W;0 in_swap;true block;valid # Dirty false (ON WRITE GOES BACK TO TOP)        8 on clock R;0 W;0 in_swap;true block;valid # Dirty true (ON WRITE GOES BACK TO TOP)
9    Evicted R;0 W;0 in_swap;true block;valid # (WE DO NOT NEED TO PERFORM A WRITE BACK)         10 Evicted R;0 W;0 in_swap;true block;valid # (WE NEED TO PERFORM A WRITE BACK)

11: on non shared destroy must write back to the file
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

    
    //Points to zero
    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename,"data1.bin");
    // at stage 2 in memory
    char *file = (char *) vm_map(filename, 1);
    strcpy(filename,"fakename.txt");

    for (int i = 0; i < 10; i++)
    {
       char *e = (char *) vm_map(nullptr, 0);
        strcpy(e,"data1.bin");
    } 
    pid = fork();
    if(pid)
    {
        vm_yield();
        //bring back and read
        for(int i= 0; i < 4096; ++i)
        {
            cout << file[i];
        }
        strcpy(filename,"data1.bin");
    }
    else{
        //bring back and read
        for(int i= 0; i < 4096; ++i)
        {
            cout << file[i];
        }
        strcpy(filename,"data1.bin");
    }
}