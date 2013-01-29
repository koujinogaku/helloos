#ifndef SHRMEM_H
#define SHRMEM_H

#include "page.h"
#define SHM_MAXPAGES (PAGE_PAGESIZE/sizeof(unsigned int))
#define SHM_MAXSIZE (PAGE_PAGESIZE*SHM_MAXPAGES)

#define SHM_OPT_KERNEL 0x01

int shm_init(void);
int shm_create(unsigned int shmname, unsigned long size, unsigned int options);
int shm_setname(int shmid, unsigned int shmname);
int shm_lookup(unsigned int shmname);
int shm_delete(int shmid);
int shm_get_size(int shmid, unsigned long *size);
int shm_map(int shmid,void *pgd, void *vmem, int type);
int shm_unmap(int shmid,void *pgd, void *vmem);
int shm_get_physical(int shmid, unsigned int pagenum, unsigned long *addr);
int shm_pull(int shmid, unsigned int pagenum, void *addr);

#endif
