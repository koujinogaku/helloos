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
#include "string.h"

#define SHM_STAT_NOTUSE 0
#define SHM_STAT_USED   1

struct SHM {
  struct SHM *next;
  struct SHM *prev;
  int id;
  unsigned int name;
  unsigned int status;
  int *pages;
  unsigned int pagecnt;
  unsigned int mappedcnt;
  unsigned int options;
};

#define SHM_TBLSZ 128
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

int shm_create(unsigned int shmname, unsigned long size, unsigned int options)
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

  if(shmname!=0) {
    list_for_each(shmtbl,shm) {
      if(shm->name==shmname) {
        mutex_unlock(&shm_tbl_mutex);
        return ERRNO_INUSE;
      }
    }
  }

  list_alloc(shmtbl,SHM_TBLSZ,shm_idx)
  if(shm_idx==0) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_RESOURCE;
  }
  shm=&shmtbl[shm_idx];
  shm->status=SHM_STAT_USED;
  shm->mappedcnt=0;
  shm->id=shm_idx;
  shm->name=shmname;
  shm->options=options;

//console_puts("[c]");

  if(options&SHM_OPT_KERNEL) {
    // alloc on kernel
    shm->pagecnt=size;
    shm->pages = mem_alloc(size);
    if(shm->pages==0) {
      shm->status=SHM_STAT_NOTUSE;
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_RESOURCE;
    }
    *(shm->pages)=1;
    list_add(shmtbl,shm);
    mutex_unlock(&shm_tbl_mutex);
    return shm_idx;
  }

  // alloc on page pool
  shm->pagecnt=pagecnt;
  shm->pages = mem_alloc(pagecnt*sizeof(unsigned int));
  if(shm->pages==0) {
    shm->status=SHM_STAT_NOTUSE;
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_RESOURCE;
  }
  for(i=0;i<pagecnt;i++) {
    shm->pages[i]=(int)page_alloc();
/*
long2hex((int)shm->pages[i],s);
console_puts(s);
console_puts(" ");
*/
    if(shm->pages[i]==0) {
      for(i--;i>=0;i--)
        page_free((void *)shm->pages[i]);
      mem_free(shm->pages,pagecnt*sizeof(unsigned int));
      shm->status=SHM_STAT_NOTUSE;
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_RESOURCE;
    }
  }

  list_add(shmtbl,shm);

  mutex_unlock(&shm_tbl_mutex);
  return shm_idx;
}

static int shm_not_exist(int shmid)
{
  if(shmid<1 || shmid>=SHM_TBLSZ) {
    return ERRNO_NOTEXIST;
  }

  if(shmtbl[shmid].status == SHM_STAT_NOTUSE) {
    return ERRNO_NOTEXIST;
  }
  return 0;
}

int shm_setname(int shmid, unsigned int shmname)
{
  struct SHM *shm;
  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  list_for_each(shmtbl,shm) {
    if(shm->name==shmname) {
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_INUSE;
    }
  }

  shmtbl[shmid].name = shmname;

  mutex_unlock(&shm_tbl_mutex);
  return shmid;
}

int shm_lookup(unsigned int shmname)
{
  struct SHM *shm;
  mutex_lock(&shm_tbl_mutex);

  list_for_each(shmtbl,shm) {
    if(shm->status==SHM_STAT_USED && shm->name==shmname ) {
      mutex_unlock(&shm_tbl_mutex);
      return shm->id;
    }
  }
  mutex_unlock(&shm_tbl_mutex);
  return ERRNO_NOTEXIST;
}

int shm_delete(int shmid)
{
  struct SHM *shm;
  int i;

  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->mappedcnt != 0) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_INUSE;
  }

  list_del(shm);

  if(shm->options&SHM_OPT_KERNEL) {
    mem_free(shm->pages,shm->pagecnt);
    shm->status=SHM_STAT_NOTUSE;
    mutex_unlock(&shm_tbl_mutex);
    return 0;
  }

  for(i=0;i<shm->pagecnt;i++) {
    page_free((void*)shm->pages[i]);
  }
  mem_free(shm->pages,shm->pagecnt*sizeof(unsigned int));
  shm->status=SHM_STAT_NOTUSE;

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}

int shm_get_size(int shmid, unsigned long *size)
{
  struct SHM *shm;

  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->options&SHM_OPT_KERNEL)
    *size = shm->pagecnt;
  else
    *size = shm->pagecnt*PAGE_PAGESIZE;

  mutex_unlock(&shm_tbl_mutex);
  return 0;
}

int shm_map(int shmid,void *pgd, void *vmem, int type)
{
  struct SHM *shm;
  unsigned int vpage;
  int i,r;

  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->options&SHM_OPT_KERNEL) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_MODE;
  }

  for(vpage=(unsigned int)vmem,i=0; i<shm->pagecnt; i++,vpage+=PAGE_PAGESIZE)
  {
    r=page_map_vpage(pgd, (void*)vpage, (void*)shm->pages[i], type);
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
int shm_unmap(int shmid,void *pgd, void *vmem)
{
  struct SHM *shm;
  unsigned int vpage;
  int i;
  void *ppage;

  mutex_lock(&shm_tbl_mutex);
  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->options&SHM_OPT_KERNEL) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_MODE;
  }

  for(vpage=(unsigned int)vmem,i=0; i<shm->pagecnt; i++,vpage+=PAGE_PAGESIZE)
  {
    ppage=page_get_ppage(pgd, (void*)vpage);
    if(ppage==0) {
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_CTRLBLOCK;
    }
    if(ppage!=(void*)shm->pages[i]) {
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
int shm_get_physical(int shmid, unsigned int pagenum, unsigned long *addr)
{
  struct SHM *shm;

  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->options&SHM_OPT_KERNEL) {
    *addr = (unsigned long)(shm->pages);
  }
  else {
    if(pagenum<shm->pagecnt) {
      *addr = (unsigned long)(shm->pages[pagenum]);
    }
    else {
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_OVER;
    }
  }
  mutex_unlock(&shm_tbl_mutex);
  return 0;
}
int shm_pull(int shmid, unsigned int pagenum, void *addr)
{
  struct SHM *shm;

  mutex_lock(&shm_tbl_mutex);

  if(shm_not_exist(shmid)) {
    mutex_unlock(&shm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }
  shm=&(shmtbl[shmid]);

  if(shm->options&SHM_OPT_KERNEL) {
    memcpy(addr,shm->pages,shm->pagecnt);
  }
  else {
    if(pagenum<shm->pagecnt) {
      memcpy(addr,(void*)(shm->pages[pagenum]),PAGE_PAGESIZE);
    }
    else {
      mutex_unlock(&shm_tbl_mutex);
      return ERRNO_OVER;
    }
  }
  mutex_unlock(&shm_tbl_mutex);
  return 0;
}
