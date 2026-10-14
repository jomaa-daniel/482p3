#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <string>

using std::string;

using std::cout;

int main()
{

    /*
        This will test a clean page that has a swap
        block and does not need to write back to it
        upon eviction
    */
   
    //parent code
    /* Allocate swap-backed page from the arena */
    char *page1 = (char *) vm_map(nullptr, 0);
    strcpy(page1, "1.txt");

    for(int i = 0; i < 3; ++i)
    {
        /* Allocate swap-backed page from the arena */
        char *page2 = (char *) vm_map(nullptr, 0);
        strcpy(page2, "2.txt");
    }
    //Perform a read
    //still clean
    cout << page1 << "\n";

    //evict page1 again
    for(int i = 0; i < 3; ++i)
    {
        /* Allocate swap-backed page from the arena */
        char *page2 = (char *) vm_map(nullptr, 0);
        strcpy(page2, "2.txt");
    }
    //make sure page1 doesn;t write back to swap

    cout << page1 << "\n";

}
// parent 
// child1
// child2 