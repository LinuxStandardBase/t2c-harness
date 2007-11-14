#ifndef LIBMEM_H
#define LIBMEM_H

extern void* alloc_mem (void* ptr, int num, int size);
extern char* alloc_mem_for_string (char* ptr, int new_length);


#endif /* LIBMEM_H */
