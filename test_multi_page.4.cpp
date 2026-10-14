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
    
    char* a = (char *) vm_map(nullptr, 0);
    char* b = (char *) vm_map(nullptr, 0);
    std::string big(4097, 'z');
    strcpy(a,big.data());
    strcpy(b,"SOME RANDOM TEXT THAT ENDS");
    cout << a;
}
