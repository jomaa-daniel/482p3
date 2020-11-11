#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

/* 
Create a chain of forks with work NO done in between // Make sure child2 is sharing with parent and child1
    (Parent forks child forks)
    -- note: keep one thing shared accross all forks, copy on write at the end
    -- have the last owner write - test handle last owner
*/

int main()
{
    pid_t pid;

    char *page = (char *) vm_map(nullptr, 0);
    strcpy(page, "copy_on_write_all together");
    char * zero = (char* )vm_map(nullptr, 0);

    pid = fork();

    if(pid)
    {
         vm_yield();         
         vm_yield();
         cout << "Parent After Yield\n";
         cout << page << "\n";
         cout << zero << "\n";
    }
    else
    {
        int pid2 = fork();
        
        if (pid2)
        {
            vm_yield();
            cout << "child 1 after yield\n";
            cout << page << "\n";
            cout << zero << "\n";
        }
        else
        {
            strcpy(page, "copy_on_write_last_child");
            strcpy(zero, "copy_on_zero_page");
            cout << "Last Child\n";
            cout << page << "\n";
            cout << zero << "\n";
        }
        
        
    }

}
