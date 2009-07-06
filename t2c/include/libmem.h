#ifndef LIBMEM_H
#define LIBMEM_H

#ifdef __cplusplus
extern "C"
{
#endif 

extern void* alloc_mem (void* ptr, int num, int size);
extern char* alloc_mem_for_string (char* ptr, int new_length);

#ifdef	__cplusplus
}
#endif
#endif /* LIBMEM_H */
