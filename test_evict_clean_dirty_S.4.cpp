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
        11 swap blocks
    
     */
    //Shared physical page
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "solo.txt");
    
    
    //Shared zero page
    char *page1 = (char *) vm_map(nullptr, 0);


    pid = fork();


    if(pid)
    {
        //parent code
        for(int i = 0 ; i<4;++i) // we are looping 4 times to check to see if the zero page is 
                                // being evicted
        {
            /* Allocate swap-backed page from the arena */
            char *page2 = (char *) vm_map(nullptr, 0);
            strcpy(page2, "2.txt");
        }
        
        cout << page << "\n"; 
        cout << page1 << '\n';
        // evict a clean page to a corresponding swap block
        for (int i = 0; i < 4; ++i)
        {
            char *page2 = (char *) vm_map(nullptr, 0);
            strcpy(page2, "2.txt");
        }
        vm_yield();
        //----------EVICTION MAKE SURE CHILD *page1 GETS EVICTED----------
    }
    else
    {
        //Child code
       cout << page << '\n';
       cout << page1 << '\n';
       
    //    strcpy(page, "solo.txt");
    //    strcpy(page1, "solo.txt");

       exit(0);

    }

}
// parent 
// child1
// child2 