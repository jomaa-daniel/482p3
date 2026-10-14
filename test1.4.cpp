#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

int main()
{
    char *p0 = (char *) vm_map(nullptr, 0);
    char *p1 = (char *) vm_map(nullptr, 0);
    char *p2 = (char *) vm_map(nullptr, 0);
    char *p3 = (char *) vm_map(nullptr, 0);

    // read from p0, p1, p2
    std::cout << (int) p0[0] << std::endl;
    std::cout << (int) p1[0] << std::endl;
    std::cout << (int) p2[0] << std::endl;

    // write to p2, p3
    p2[0] = 'a';
    p3[0] = 'b';

    // ask OS to call the pager's vm_accessed function
    vm_accessed();

    // read from p1, p2
    std::cout << (int) p1[0] << std::endl;
    std::cout << (int) p2[0] << std::endl;

    // write to p0 and p1
    p0[0] = 'c';
    p1[0] = 'd';

    // ask OS to call the pager's vm_accessed function
    vm_accessed();
}