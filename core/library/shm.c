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
static unsigned long shm_mapping_address = CFG_MEM_SHMSTART;

int shm_create(unsigned int shmname, unsigned long size)
{
  return syscall_shm_create(shmname, size, 0);
}

int shm_setname(int shmid, unsigned int shmname)
{
  return syscall_shm_setname(shmid, shmname);
}

int shm_lookup(unsigned int shmname)
{
  return syscall_shm_lookup(shmname);
}

int shm_delete(int shmid)
{
  return syscall_shm_delete(shmid);
}

int shm_getsize(int shmid,unsigned long *sizep)
{
  return syscall_shm_getsize(shmid, sizep);
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

  return syscall_shm_map(shmid,vmem);
}

int shm_unmap(int shmid)
{
  struct SHM_TBL *shm;
  void *vmem;

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
  vmem = (void*)shm->vmem;
  list_del(shm);
  mfree(shm);

  return syscall_shm_unmap(shmid,vmem);
}


int shm_bind(int shmid, unsigned long *addr)
{
  int rc;
  unsigned long size;

  rc = shm_getsize(shmid,&size);
  if(rc<0) {
    return rc;
  }

  rc = shm_map(shmid,(void*)shm_mapping_address);
  if(rc<0) {
    return rc;
  }

  *addr=shm_mapping_address;
  shm_mapping_address += size;

  return 0;
}

int shm_open(int shmname, unsigned long size, int *init)
{
  int shmid;

  if(shmname==0)
      return ERRNO_NOTEXIST;

  shmid=shm_create(shmname, size); // same name to map address
  if(shmid<0) {
    if(shmid!=ERRNO_INUSE)
      return shmid;
    shmid=shm_lookup(shmname);
    if(shmid<0)
      return shmid;
    *init=0;
  }
  else {
    *init=1;
  }

  return shmid;
}
