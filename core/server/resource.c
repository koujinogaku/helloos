/* program manager */
#include "config.h"

#define PGM_STAT_NOTUSE 0
#define PGM_STAT_USED   1

#define PGM_TASKIDSZ 8
#define PGM_SHMIDSZ  8
#define PGM_TITLESZ  32

struct PGM {
  struct PGM *next;
  struct PGM *prev;
  unsigned short id;
  unsigned char  status;
  unsigned char  ioenable;
  unsigned short exitcode;
  void *pgd;
  void *progaddr;
  void *vram;
  unsigned long vramsize;
  int taskid[PGM_TASKIDSZ];
  int shmid[PGM_SHMIDSZ];
  char title[PGM_TITLESZ];
};

#define PGM_TBLSZ 32
static struct PGM *progtbl=0;
static int pgm_tbl_mutex=0;
static struct list_head pgm_free_head;
static struct list_head pgm_list_head;
static struct list_head *pgmfree=&pgm_free_head;
static struct list_head *pgmlist=&pgm_list_head;

//static char s[10];

int program_init(void)
{
  struct PGM *pgm;
  pgmtbl = mem_alloc(PGM_TBLSZ * sizeof(struct PGM));
  if(pgmtbl==0)
    return ERRNO_RESOURCE;
  memset(pgmtbl,0,PGM_TBLSZ * sizeof(struct PGM));

  list_init(pgmlist);
  list_init(pgmfree);

  for(i=0;i<PGM_TBLSZ;i++) {
    pgm=&pgmtbl[i];
    pgm->id=i+1;
    list_add_tail(pgmfree,pgm);
  }

  return 0;
}

static struct PGM *pgm_alloc(void)
{
  struct PGM *pgm;

  if(list_empty(pgmfree)) {
    return 0;
  }
  pgm=(struct PGM*)pgmfree->next;
  list_del(pgm);
  memset(pgm,0,sizeof(struct PGM));
  pgm->id = ((unsigned int)pgm - (unsigned int)pgmtbl) / sizeof(struct PGM) + 1;
  list_add(pgmlist,pgm);

  return pgm;
}

static struct PGM *pgm_find(int pgmid)
{
  struct PGM pgm;
  list_for_each(pgmlist,pgm)
    if(pgm->id == pgmid)
      return pgm;
  return NULL;
}

int program_load(char filename)
{
  int fp,taskid;
  unsigned int filesize;
  unsigned int pagesize;
  void *loadbuf,*pgd,*progaddr;
  int r;

  mutex_lock(&pgm_tbl_mutex);
  
  pgm=pgm_alloc();
  if(pgm==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_RESOURCE;
  }

  fp = fat_open_file(filename, FAT_O_RDONLY);
  if(fp<0) {
    console_puts("file open error=");
    console_puts(filename);
    console_puts("\n");
    return fp;
  }

  r=fat_get_filesize(fp,&filesize);
  if(r<0) {
    console_puts("file size error=");
    console_puts(filename);
    console_puts("\n");
    return r;
  }
/*
  console_puts("FileSize=");
  int2dec(memsz,s);
  console_puts(s);
  console_puts("\n");
*/
  pagesize=page_frame_size(filesize);
  pgd=page_create_pgd();
  if(pgd==NULL){
    console_puts("pgd alloc error\n");
    return ERRNO_RESOURCE;
  }
/*
console_puts("pgd=");
long2hex((int)tstpgd,s);
console_puts(s);
console_puts("\n");
*/
//page_dump_pgd(tstpgd);

  progaddr=(void*)CFG_MEM_USER;
  r=page_alloc_vmem(pgd,progaddr,pagesize,(PAGE_TYPE_USER|PAGE_TYPE_RDWR));
  if(r<0){
    console_puts("vmem alloc error\n");
    return ERRNO_RESOURCE;
  }

  loadbuf=mem_alloc(MEM_PAGESIZE);
  if(loadbuf==0) {
    console_puts("mem alloc error\n");
    return ERRNO_RESOURCE;
  }

//mem_dumpfree();

  for(n=0;n<filesize;n+=512) {

    r=fat_read_file(fp,loadbuf,512)
    if(r<0) {
      console_puts("file read error\n");
      return r;
    }
    page_memcpy(pgd,(void*)((unsigned int)progaddr+n),loadbuf, 512);

  
  r=fat_close_file(fp);
  if(r<0) {
    console_puts("file close error\n");
    return r;
  }
  mem_free(loadbuf,MEM_PAGESIZE);

//mem_dumpfree();

//page_dump_pgd(tstpgd);

  taskid = task_create_process(pgd,progaddr,pagesize);
  if(taskid<0) {
    console_puts("task create error\n");
    return taskid;
  }
  pgm->status=PGM_STAT_LOADED;
  pgm->pgd=pgd;
  pgm->progaddr=progaddr;
  pgm->taskid[0]=taskid;

  mutex_unlock(&pgm_tbl_mutex);
  return pgm->id;
}


int program_delete(int pgmid)
{
  struct PGM *pgm;
  int i;

  mutex_lock(&pgm_tbl_mutex);

  pgm=pgm_find(pgmid);
  if(pgm=NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  for(i=0;i<PGM_TASKIDSZ;i++)
    if(pgm->taskid[i]!=0) {
      void *fpu;
      fpu=task_get_fpu(taskid);
      if(fpu)
        fpu_free(fpu);
      exitcode=task_delete(pgm->taskid[i]);
    }
  for(i=0;i<PGM_SHMIDSZ;i++)
    if(pgm->shmid[i]!=0)
      exitcode=shm_delete(pgm->taskid[i]);

  page_delete_pgd(pgm->pgd);

  mutex_unlock(&shm_tbl_mutex);
  return exitcode;
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
