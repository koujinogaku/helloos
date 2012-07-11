#ifndef MEMORY_H
#define MEMORY_H

#ifndef NULL
#define NULL   0
#endif

void mfree(void *freearea);
void *malloc(unsigned long areasize);
void *realloc(void *address,unsigned long areasize);
void *calloc(unsigned long count,unsigned long bufsize);

unsigned long mem_get_heapfree(void);
int mem_check_heapfree(void);
void mem_init(void);
void mem_dumpfree(void);
int mem_get_status(void);

#endif
