/* memory page manager */
#include "config.h"
#include "page.h"
#include "kmemory.h"
#include "shrmem.h"
#include "string.h"
#include "list.h"
#include "console.h"
#include "errno.h"
#include "shrmem.h"
#include "mutex.h"

#define SHM_STAT_NOTUSE 0
#define SHM_STAT_USED   1

struct SHM {
  struct SHM *next;
  struct SHM *prev;
  unsigned int id;
  unsigned int status;
  void *pages[SHM_MAXPAGES];
  unsigned int pagecnt;
  unsigned int mappedcnt;
};

#define SHM_TBLSZ 32
static struct SHM *shmtbl=0;
static int shm_tbl_mutex=0;

//static char s[10];

int shm_init(void)
{
  shmtbl = mem_alloc(SHM_TBLSZ * sizeof(struct SHM));
  if(shmtbl==0)
    return ERRNO_RESOURCE;
/*
{char s[10];
console_puts("shmtbl size=");
int2dec(SHM_TBLSZ * sizeof(struct SHM),s);
console_puts(s);
console_puts("\n");
}
*/
  memset(shmtbl,0,SHM_TBLSZ * sizeof(struct SHM));
  list_init(shmtbl);
  return 0;
}

int shm_create(unsigned int shmid, unsigned int size)
{
  struct SHM *shm;
  int shm_idx,i;
  unsigned int pagecnt;

  mutex_lock(&shm_tbl_mutex);

  pagecnt=page_frame_size(size)/PAGE_PAGESIZE;
  if(pagecnt>SHM_MAXPAGES) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_OVER;
  }

  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl!=shm){
    return ERRNO_INUSE;
    mutex_unlock(&shm_tbl_mutex);
  }
  list_alloc(shmtbl,SHM_TBLSZ,shm_idx)
  if(shm_idx==0) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_RESOURCE;
  }
  shm=&shmtbl[shm_idx];
  shm->status=SHM_STAT_USED;

//console_puts("[c]");

  for(i=0;i<pagecnt;i++) {
    shm->pages[i]=page_alloc();
/*
long2hex((int)shm->pages[i],s);
console_puts(s);
console_puts(" ");
*/
    if(shm->pages[i]==0) {
      for(i--;i>=0;i--)
        page_free(shm->pages[i]);
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_RESOURCE;
    }
  }
  shm->pagecnt=pagecnt;
  shm->mappedcnt=0;
  shm->id=shmid;

  list_add(shmtbl,shm);

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}

int shm_delete(unsigned int shmid)
{
  struct SHM *shm;
  int i;

  mutex_lock(&shm_tbl_mutex);

  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl==shm) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  if(shm->mappedcnt != 0) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_INUSE;
  }
//console_puts("[d]");

  for(i=0;i<shm->pagecnt;i++) {
    page_free(shm->pages[i]);
/*
long2hex((int)shm->pages[i],s);
console_puts(s);
console_puts(" ");
*/
  }

  list_del(shm);
  shm->status=SHM_STAT_NOTUSE;

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}

int shm_get_size(unsigned int shmid, unsigned int *size)
{
  struct SHM *shm;

  mutex_lock(&shm_tbl_mutex);
  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl==shm) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  *size = shm->pagecnt*PAGE_PAGESIZE;

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}

int shm_map(unsigned int shmid,void *pgd, void *vmem, int type)
{
  struct SHM *shm;
  unsigned int vpage;
  int i,r;

  mutex_lock(&shm_tbl_mutex);

  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl==shm) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  for(vpage=(unsigned int)vmem,i=0; i<shm->pagecnt; i++,vpage+=PAGE_PAGESIZE)
  {
    r=page_map_vpage(pgd, (void*)vpage, shm->pages[i], type);
    if(r<0) {
      for(i--,vpage+=PAGE_PAGESIZE; i>=0; i--,vpage+=PAGE_PAGESIZE)
        page_unmap_vpage(pgd, (void*)vpage);
      mutex_unlock(&shm_tbl_mutex);
      return r;
    }
  }
  shm->mappedcnt++;
  mutex_unlock(&shm_tbl_mutex);
  return 0;
}
int shm_unmap(unsigned int shmid,void *pgd, void *vmem)
{
  struct SHM *shm;
  unsigned int vpage;
  int i;
  void *ppage;

  mutex_lock(&shm_tbl_mutex);
  list_for_each(shmtbl,shm)
  {
    if(shm->id==shmid)
      break;
  }
  if(shmtbl==shm) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  for(vpage=(unsigned int)vmem,i=0; i<shm->pagecnt; i++,vpage+=PAGE_PAGESIZE)
  {
    ppage=page_get_ppage(pgd, (void*)vpage);
    if(ppage==0) {
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_CTRLBLOCK;
    }
    if(ppage!=shm->pages[i]) {
/*
console_puts("[");
long2hex((int)vpage,s);
console_puts(s);
console_puts(",");
long2hex((int)ppage,s);
console_puts(s);
console_puts("]");
*/
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_CTRLBLOCK;
    }
    ppage=page_unmap_vpage(pgd, (void*)vpage);
  }
  shm->mappedcnt--;

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}
