#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{
    pid_t pid;

    /* 
       Evict a dirty page with no corresponding swap block
        
    
     */
    //Shared physical page
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "solo.txt");
    //evict
    for(int i = 0 ; i<3;++i) // we are looping 4 times to check to see if the zero page is 
                            // being evicted
    {
        /* Allocate swap-backed page from the arena */
        char *page2 = (char *) vm_map(nullptr, 0);
        strcpy(page2, "2.txt");
    }
    //overwrite
    strcpy(page, "write2.txt");
    
    //share it
    pid = fork();

    if(pid)
    {
        // evict a dirty page to a corresponding swap block
        for (int i = 0; i < 3; ++i)
        {
            char *page2 = (char *) vm_map(nullptr, 0);
            strcpy(page2, "2.txt");
        }
        vm_yield();
    }
    else
    {
        //Child code
       cout << page << '\n';

       exit(0);

    }

}
