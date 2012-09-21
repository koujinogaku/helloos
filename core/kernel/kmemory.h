#ifndef KMEMORY_H
#define KMEMORY_H

#define MEM_PAGESIZE  4096

#ifndef NULL
#define NULL   (0)
#endif

#define KMEMORY_STAT_NOTINIT  0
#define KMEMORY_STAT_LOWADDR  1
#define KMEMORY_STAT_HIGHADDR 2

void mem_free(void *freearea, unsigned long areasize);
void *mem_alloc(unsigned long areasize);
unsigned long mem_get_totalsize(void);
unsigned long mem_get_kernelsize(void);
unsigned long mem_get_kernelfree(void);
void mem_init(void);
void mem_dumpfree(void);
int mem_get_status(void);

#endif
