#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

// Create a chain of forks WITH work done in between // Make sure p
//    (do work Parent forks do work in child child forks)
int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "Original.txt");
    char *zero = (char *) vm_map(nullptr, 0);
    
    //Prints original.txt
    //Prints nothing
    cout << page << '\n';
    cout << zero << '\n';

    pid = fork();

    if(pid)
    {
         //Parent
         //Fork again
         pid_t pid2 = fork();
         if(pid2){
             
            strcpy(page, "Shared parent and child 2.txt");
            strcpy(zero, "Zero Shared parent and child 2.txt");
            cout <<"child2 is running\n";
            cout << page<< '\n';
            cout << zero<< '\n';
            vm_yield();
         }
         else{
            //Child 2
            //Prints shared parent and child2
            //Prints zero shared
             //Prints shared parent and child2
             //Prints zero shared
             cout <<"parent is running\n";
             cout << page<< '\n';
             cout << zero<< '\n';
             vm_yield();
            
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
        vm_yield();
    }

}
