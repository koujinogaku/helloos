#ifndef SHM_H
#define SHM_H

int shm_create(int shmid, int size);
int shm_delete(int shmid);
int shm_getsize(int shmid,int *sizep);
int shm_map(int shmid,void *vmem);
int shm_unmap(int shmid);

#endif
