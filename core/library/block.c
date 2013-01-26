#include "syscall.h"
#include "shm.h"
#include "block.h"
#include "display.h"
#include "stdlib.h"

struct block_ctrl {
  unsigned long next;
};
struct block_header {
  int shm_mtx;
  unsigned int shmname;
  unsigned long next;
};


unsigned long block_init(unsigned int shmname)
{
  unsigned long block_shm_addr;
  struct block_header *block_header;

  int rc;
  struct block_ctrl *freeblock;
  int initflg;
  int shmid;
  unsigned long count;
  int i;

  shmid = shm_open(shmname,BLOCK_MAXSIZE,&initflg);
  if(shmid<0)
    return 0;

  rc = shm_bind(shmid,&block_shm_addr);
  if(rc<0)
    return 0;

  block_header=(struct block_header *)(block_shm_addr);

  if(!initflg)
    return block_shm_addr;

  block_header->next=BLOCK_SIZE;
  block_header->shmname=shmname;
  count=BLOCK_MAXSIZE/BLOCK_SIZE;
  for(i=1;i<count-1;i++) {
    freeblock = (void*)(block_shm_addr+(BLOCK_SIZE*i));
    freeblock->next = (i+1)*BLOCK_SIZE;
  }
  freeblock->next=0;

  syscall_mtx_unlock(&block_header->shm_mtx);

  return block_shm_addr;
}

unsigned long block_alloc(unsigned long block_shm_addr)
{
  struct block_header *block_header=(struct block_header *)(block_shm_addr);
  struct block_ctrl *addr;
  unsigned long offset;

  if(block_shm_addr==0)
    return 0;

  syscall_mtx_lock(&block_header->shm_mtx);

  if(block_header->next==0) {
    syscall_mtx_unlock(&block_header->shm_mtx);
    return 0;
  }

  offset=block_header->next;
  addr=block_addr(block_shm_addr,offset);
  block_header->next=addr->next;

  syscall_mtx_unlock(&block_header->shm_mtx);

  return offset;
}

void block_free(unsigned long block_shm_addr, unsigned long offset)
{
  struct block_ctrl *addr=block_addr(block_shm_addr,offset);
  struct block_header *block_header=(struct block_header *)(block_shm_addr);
  if(block_shm_addr==0)
    return ;

  if(offset==0) {
    return ;
  }

  syscall_mtx_lock(&block_header->shm_mtx);

  addr->next = block_header->next;
  block_header->next = offset;

  syscall_mtx_unlock(&block_header->shm_mtx);

  return ;
}

