#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/libmem.h"

void*
alloc_mem (void* ptr, int num, int size)
{
    if (!ptr)
    {
        ptr = calloc (num, size);
    } else {
        ptr = realloc (ptr, num * size);
    }

    if (!ptr)
    {
        printf ("libmem: alloc_mem() can't allocate memory\n");
        exit (1);
    }
    return ptr;
}

char*
alloc_mem_for_string (char* ptr, int new_length)
{
    int old_length = 0;

    if (ptr)
    {
        old_length = strlen (ptr);
    }
    
    if (new_length == 0)
    {
        new_length = 1;
    }

    ptr = (char*) alloc_mem (ptr, sizeof (char), new_length + 1);
    
    if(old_length < new_length)
    {
        memset(ptr + old_length, '\0',
               sizeof (char) * (new_length - old_length + 1));
    } else {
        *(ptr + new_length) = '\0';
    }

    return ptr;
}
