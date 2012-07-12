#include "config.h"
#include "list.h"
#include "syscall.h"
#include "memory.h"
#include "errno.h"
#include "shm.h"
#include "string.h"

struct SHM_TBL {
  struct SHM_TBL *next;
  struct SHM_TBL *prev;
  unsigned int id;
  int vmem;
  unsigned int size;
};

static struct SHM_TBL *shmtbl=0;

int shm_create(int shmid, int size)
{
  return syscall_shm_create(shmid, size);
}

int shm_delete(int shmid)
{
  return syscall_shm_delete(shmid);
}

int shm_getsize(int shmid,int *sizep)
{
  return syscall_shm_getsize(shmid, (int)sizep);
}

int shm_map(int shmid,void *vmem)
{
  struct SHM_TBL *shm;

  if(shmtbl==0) {
    shmtbl=malloc(sizeof(struct SHM_TBL));
    memset(shmtbl,0,sizeof(struct SHM_TBL));
    list_init(shmtbl);
  }
  if((unsigned int)vmem >= CFG_MEM_USER)
    return ERRNO_SEEK;

  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      return ERRNO_INUSE;
  }

  shm=malloc(sizeof(struct SHM_TBL));
  if(shm==0) {
    return ERRNO_RESOURCE;
  }
  shm->id=shmid;
  shm->vmem=(int)vmem;
  list_add(shmtbl,shm);

  return syscall_shm_map(shmid,(int)vmem);
}

int shm_unmap(int shmid)
{
  struct SHM_TBL *shm;
  int vmem;

  if(shmtbl==0) {
    return ERRNO_NOTEXIST;
  }

  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl==shm) {
    return ERRNO_NOTEXIST;
  }
  vmem = (int)shm->vmem;
  list_del(shm);
  mfree(shm);

  return syscall_shm_unmap(shmid,vmem);
}
