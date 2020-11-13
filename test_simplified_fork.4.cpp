#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <vector>

using std::cout;

int main()
{

    /*
        this will max out the swap file and cause a seg fault or whatever
        happens
    */

    // Parent has 1 virtual page 0x00060000
    // Parent current_arena_bound is 0x0006001000
    // ppage1 is being created

    pid_t pid;
    std::vector<char*> ppage = std::vector<char*>();
    for (int i = 0; i < 1; i++)
    {
        char *page = (char *) vm_map(nullptr, 0);
        strcpy(page, "c.txt");
        if (i<2)
        {
            ppage.push_back(page);
        }
    }
    for(auto page :ppage){
        std::cout<< page<<'\n';
    }
    cout << "parent forks\n";
    pid = fork();

    if(pid)
    {       
        //do nothing and die
        vm_yield();
        vm_yield();
        vm_yield();
    }
    else
    {
        cout << "child 1 forks\n";
        pid_t pid2;
        pid2 = fork();
        if(pid2)
        {       
            //do nothing and die
            vm_yield();
            vm_yield();
        }
        else
        {
            cout << "child 2 forks\n";
            pid_t pid3;
            pid3 = fork();
            if(pid3)
            {       
                //do nothing and die
                vm_yield();
            }
            else
            {
                cout << "kill the swap \n";
                char *page = (char *) vm_map(nullptr, 0);
                strcpy(page, "c.txt");
            }   
            
        }
    }
}
// parent 
// child1
// child2 


