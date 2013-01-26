#ifndef BLOCK_H
#define BLOCK_H

#include "msgquenames.h"

#define BLOCK_SIZE     4096
#define BLOCK_MAXSIZE  (BLOCK_SIZE*16)
#define BLOCK_SHM_ID   MSGQUENAMES_BLOCK

#define block_addr(base,offset)  ((void*)(offset+base))
#define block_offset(base,addr)  ((unsigned long)addr-base)

typedef unsigned long block_t;

unsigned long block_init(unsigned int shmname);
unsigned long block_alloc(unsigned long block_shm_addr);
void block_free(unsigned long block_shm_addr, unsigned long offset);


#endif
