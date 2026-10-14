
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
    char *zero = (char *) vm_map(nullptr, 0);
    
    //Prints original.txt
    //Prints nothing
    //No faults
    cout << page << '\n';
    cout << zero << '\n';

    pid = fork();

    if(pid)
    {
         //Parent
         //Fork again
         //No Faults
         pid_t pid2 = fork();
         if(pid2){
            // no faults
            cout <<"parent is running\n";
            cout << page<< '\n';
            cout << zero<< '\n';
            //Should not fault
            vm_yield();
            vm_yield();
            strcpy(page, "Test copy on write.txt");
            cout << "Parent runs final time\n";
            cout << page<< '\n';
            return 0;
         }
         else{
            //Child 2
            //Prints shared parent and child2
            //Prints zero shared
             //No faults
             cout <<"child2 is running\n";
             cout << page<< '\n';
             cout << zero<< '\n';
             return 0;            
         }
    }
    else
    {
        //Child 1
        //Prints Original
        //Prints nothing
        //No Faults
        cout <<"child1 is running\n";
        cout << page << "\n";
        cout << zero << "\n";
        return 0;   
    }

}
