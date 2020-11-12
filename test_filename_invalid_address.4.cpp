#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"
#include <string>

using std::cout;

// 1 parent 2 children regression? // Make sure children are sharing after parent dies
// (1 parent forks twice)

int main()
{
    pid_t pid;

    /* Allocate swap-backed page from the arena */
    for (int i = 0; i < 4095; ++i)
    {
        char *page = (char *) vm_map("nullptr", 0);
    }

    char * extralong = (char *) vm_map(nullptr, 0);
    std::string str(4097, 'z');
    strcpy(extralong, str.data());
    cout << extralong;

}
