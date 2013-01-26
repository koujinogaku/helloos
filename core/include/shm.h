#ifndef SHM_H
#define SHM_H

int shm_create(unsigned int shmname, unsigned long size);
int shm_setname(int shmid, unsigned int shmname);
int shm_lookup(unsigned int shmname);
int shm_delete(int shmid);
int shm_getsize(int shmid,unsigned long *sizep);
int shm_map(int shmid,void *vmem);
int shm_unmap(int shmid);
int shm_bind(int shmid, unsigned long *addr);
int shm_open(int shmname, unsigned long size, int *init);

#endif
