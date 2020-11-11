#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <vector>

using std::cout;

int main()
{

    /*
        This test will test the fact that the swap blocks need to be 
        free'ed once there is a VM destory. this spins up 1024 v pages
        and brings back 3 into phy mem. then is deallocated
        the child will do the same and should take the spots
        of the child virtual and phyiscal pages
    */
    pid_t pid;

    pid = fork();

    if(pid)
    {       

        std::vector<char*> ppage = std::vector<char*>();
        for (int i = 0; i < 1024; i++)
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
        vm_yield();
    }
    else
    {
        std::vector<char*> ppage = std::vector<char*>();
        for (int i = 0; i < 1024; i++)
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
    }
}
// parent 
// child1
// child2 