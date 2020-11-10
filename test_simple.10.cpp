#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;

int main()
{
    /* Allocate swap-backed page from the arena */
    char *filename = (char *) vm_map(nullptr, 0);
    /* Print the first speech from the file */
    for (char * c = filename; c < filename + 4096; ++c)
    {
	    cout << *c;
    }
    cout << "--------------------\n";
    strcpy(filename, "shakespeare.txt");
    /* Print the first speech from the file */
    for (char * c = filename; c < filename + 4096; ++c)
    {
	    cout << *c;
    }
}