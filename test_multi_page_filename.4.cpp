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
    //char* b = (char *) vm_map(nullptr, 0);
    //std::string big(100, 'z');

    strcpy(a,"shakespeare.txt");
    char *file1 = (char *) vm_map(a, 0);
    //CAN WE JUST DO THIS
    //cout << *file1;wdfgfgdfg 
    /* Print the first speech from the file */
    for (unsigned int i=0; i<4096; i++) {
	    cout << file1[i];
    }
    //char *file1 = (char *) vm_map(big.data(), 0);
}

