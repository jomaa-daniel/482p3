#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

// write on a last owner, make sure that there is no fault

int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "Parent.txt");


    pid = fork();

    if(pid)
    {
         vm_yield();
         // this should not create a vm_fault
         strcpy(page, "copy_on_write_last_owner");
         cout << page << "\n";
    }
    else
    {
        strcpy(page, "child.txt");
        cout << page << "\n";
    }

}
