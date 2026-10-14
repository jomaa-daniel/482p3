#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{


    //parent code
    /* Allocate swap-backed page from the arena */
    char *page1 = (char *) vm_map(nullptr, 0);
    strcpy(page1, "1.txt");
    /* Allocate swap-backed page from the arena */
    char *page2 = (char *) vm_map(nullptr, 0);
    strcpy(page2, "2.txt");
    /* Allocate swap-backed page from the arena */
    char *page3 = (char *) vm_map(nullptr, 0);
    strcpy(page3, "3.txt");

    

}
// parent 
// child1
// child2 