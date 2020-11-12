
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

//Create a chain of forks with work NO done in between // Make sure child2 is sharing with parent and child1
//    (Parent forks child forks)
//    -- note: keep one thing shared accross all forks, copy on write at the end
//    -- have the last owner write - test handle last owner

int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "Original.txt");
    
    //full mem 
    for(int i =0; i< 2;++i){
        char *page1 = (char *) vm_map(nullptr, 0);
        strcpy(page1, "Original.txt");
    }


    pid = fork();

    if(pid)
    {
         vm_yield();
         //Should be a fault
        cout <<"parent is running\n";
        cout << page << '\n';
    }
    else
    {
        //Child 1
        //Prints Original
        //Prints nothing
        //No Faults
        cout <<"child is running\n";
        *(page+1)='N';
        cout << page << '\n';
    }
}