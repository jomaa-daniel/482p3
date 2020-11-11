#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

// copy on write two owners that both point to the same swap block (NOT IN CLOCK)

int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "Parent.txt");

    for(int i = 0; i<3; ++i)
    {
        char *page1 = (char *) vm_map(nullptr, 0);
        strcpy(page1, "1.txt");
    }

    pid = fork();

    if(pid)
    {
         vm_yield();
         cout << page << "\n";
    
    }
    else
    {
        //child
        //code
        /* Allocate swap-backed page from the arena */
        strcpy(page, "child.txt");
        cout << page << "\n";
    }

}
// parent 
// child1
// child2 