#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{
    pid_t pid;


    /* 
        Shared phyiscal memory  on line 19. then it is written to on line 32 causing a copy on write
        now page has a virtual pointer to zero and a virtual pointer to real mem

        there is a shared zero page on line 26, copy on write on line 41

        solo belongs to the child. 
        parent owns its own 'page' when it creates shared.txt. 
        page1 is created and owned by parent
        child's solo.txt is evicted with dirty bit to swap on line 50
        parents page "shared.txt" is evicted by parent's page3 variable
        
    
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

        //copy on write on a shared physical page
        strcpy(page, "shared.txt");
        
        
        //copy on write on a shared zero page
        strcpy(page1, "1.txt");
        
        /* Allocate swap-backed page from the arena */
        char *page2 = (char *) vm_map(nullptr, 0);
        strcpy(page2, "2.txt");

        //----------EVICTION MAKE SURE CHILD *page GETS EVICTED----------

        /* Allocate swap-backed page from the arena */
        char *page3 = (char *) vm_map(nullptr, 0);
        strcpy(page3, "3.txt");
        

        //----------EVICTION MAKE SURE CHILD *page1 GETS EVICTED----------



    }
    else
    {
       cout << page << '\n';
       cout << page1 << '\n';
    }

}
// parent 
// child1
// child2 