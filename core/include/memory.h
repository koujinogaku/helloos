#ifndef MEMORY_H
#define MEMORY_H

#ifndef NULL
#define NULL   0
#endif

#define mfree(freearea)  memory_free(freearea, 0)
#define malloc(areasize) memory_alloc(areasize, 0)
#define realloc(address, areasize) memory_realloc(address, areasize, 0)
#define calloc(count, bufsize) memory_calloc(count, bufsize, 0)

void memory_init(void *heap_start, unsigned long heap_size);
void memory_free(void *freearea, void* control);
void *memory_alloc(unsigned long areasize, void* control);
void *memory_realloc(void *address, unsigned long areasize, void* control);
void *memory_calloc(unsigned long count, unsigned long bufsize, void* control);

int memory_check_heapfree(void* control);
unsigned long memory_get_heapfree(void* control);
void memory_dumpfree(void* control);

#endif
