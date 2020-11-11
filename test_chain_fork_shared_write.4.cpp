#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

// 1 parent 2 children regression? // Make sure children are sharing after parent dies
// (1 parent forks twice)

int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "Original.txt");
    char *zero = (char *) vm_map(nullptr, 0);

    pid = fork();

    if(pid)
    {
         //Parent
         //Fork again
         pid_t pid2 = fork();
         if(pid2){
             
            vm_yield();
            //FAULT HERE for both zero and page
            strcpy(page, "Parent changed original.txt");
            strcpy(zero, "Parent changed zero.txt");
            cout <<"Parent is running\n";
            cout << page<< '\n';
            cout << zero<< '\n';
            vm_yield();
         }
         else{
            //Child 2
            // Only fault for 0
             cout <<"Child 2 is running\n";
            strcpy(page, "Child2 changed original.txt");
            strcpy(zero, "Child 2 changed zero.txt");
             cout << page<< '\n';
             cout << zero<< '\n';
             return 0;
            }
         vm_yield();
    }
    else
    {
        //Child 1
        //Prints Original
        //Prints nothing
        cout <<"child1 is running\n";
        cout << page << "\n";
        cout << zero << "\n";
        //Dies
        return 0;
    }

}
