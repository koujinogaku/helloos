#ifndef SHRMEM_H
#define SHRMEM_H

#include "page.h"
#define SHM_MAXPAGES (PAGE_PAGESIZE/sizeof(unsigned int))
#define SHM_MAXSIZE (PAGE_PAGESIZE*SHM_MAXPAGES)

int shm_init(void);
int shm_create(unsigned int shmid, unsigned int size);
int shm_delete(unsigned int shmid);
int shm_get_size(unsigned int shmid, unsigned int *size);
int shm_map(unsigned int shmid,void *pgd, void *vmem, int type);
int shm_unmap(unsigned int shmid,void *pgd, void *vmem);

#endif
