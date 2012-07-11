#ifndef SHRMEM_H
#define SHRMEM_H

#include "config.h"
#define SHM_MAXPAGES 16
#define SHM_MAXSIZE (CFG_MEM_PAGESIZE*SHM_MAXPAGES)

int shm_init(void);
int shm_create(unsigned int shmid, unsigned int size);
int shm_delete(unsigned int shmid);
int shm_get_size(unsigned int shmid, unsigned int *size);
int shm_map(unsigned int shmid,void *pgd, void *vmem, int type);
int shm_unmap(unsigned int shmid,void *pgd, void *vmem);

#endif
