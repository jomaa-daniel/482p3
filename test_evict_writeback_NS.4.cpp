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
        This will test writing a dirty page to a swap block that is already been written to/ exists
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
    // bring from swap file
    //Perform a write
    //becomes dirty
    strcpy(page1, "1test.txt");
    

    //evict page1 again
    for(int i = 0; i < 3; ++i)
    {
        /* Allocate swap-backed page from the arena */
        char *page2 = (char *) vm_map(nullptr, 0);
        strcpy(page2, "2.txt");
    }
    //make sure page1 doesn;t write back to swap

    // cout << page1 << "\n";
    strcpy(page1, "1test.txt");

}
// parent 
// child1
// child2 