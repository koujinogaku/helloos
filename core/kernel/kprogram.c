/* program manager */
#include "config.h"
#include "kprogram.h"
#include "mutex.h"
#include "fat.h"
#include "page.h"
#include "kmemory.h"
#include "errno.h"
#include "console.h"
#include "task.h"
#include "list.h"
#include "string.h"
#include "kmessage.h"
#include "queue.h"
#include "cpu.h"
#include "kmem.h"
#include "fpu.h"

extern int kernel_queid;

#define PGM_STAT_NOTUSE 0
#define PGM_STAT_USED   1

struct PGM {
  struct PGM *next;
  struct PGM *prev;
  unsigned short id;
  unsigned short status;
  unsigned short taskid;
  unsigned short exitque;
  void *pgd;
  unsigned short taskque;
  char pgmname[8];
};

static int pgm_tbl_mutex=0;
static struct PGM *pgmtbl=0;
#define PGM_TBLSZ 64

static char s[10];

struct PGM *pgm_alloc(void)
{
  struct PGM *pgm;
  int pgm_idx;

  list_alloc(pgmtbl,PGM_TBLSZ,pgm_idx)
  if(pgm_idx==0) {
    return NULL;
  }
  pgm=&pgmtbl[pgm_idx];
  pgm->status=PGM_STAT_USED;
  pgm->id=pgm_idx;
  pgm->taskid=0;
  pgm->exitque=0;
  pgm->taskque=0;
  pgm->pgd=0;

  list_add_tail(pgmtbl,pgm)

  return pgm;
}

void pgm_free(struct PGM *pgm)
{
  pgm->status=PGM_STAT_NOTUSE;
  list_del(pgm);
}

static struct PGM *pgm_find(int taskid)
{
  struct PGM *pgm;

  list_for_each(pgmtbl,pgm)
  {
    if(pgm->taskid==taskid)
      break;
  }
  if(pgmtbl==pgm) {
    return NULL;
  }
  return pgm;
}

int pgm_delete(int taskid)
{
  struct PGM *pgm;

  if(pgmtbl==NULL)
    return ERRNO_NOTINIT;

  pgm=pgm_find(taskid);
  if(pgm==NULL)
    return ERRNO_NOTEXIST;

  pgm_free(pgm);

  return 0;
}

int program_init(void)
{
  struct PGM *pgm;

  pgmtbl = mem_alloc(PGM_TBLSZ * sizeof(struct PGM));
  if(pgmtbl==0)
    return ERRNO_RESOURCE;
/*
{char s[10];
console_puts("pgmtbl size=");
int2dec(PGM_TBLSZ * sizeof(struct PGM),s);
console_puts(s);
console_puts("\n");
}
*/
  memset(pgmtbl,0,PGM_TBLSZ * sizeof(struct PGM));
  list_init(pgmtbl);

  pgm=pgm_alloc();
  pgm->taskid=1;
  pgm->pgd=task_get_pgd(pgm->taskid);
  pgm->taskque=kernel_queid;
  strncpy(pgm->pgmname,"[Kernel]",8);
  pgm=pgm_alloc();
  pgm->taskid=2;
  pgm->pgd=task_get_pgd(pgm->taskid);
  strncpy(pgm->pgmname,"[Idle]",8);
  return 0;
}

int program_load(char *filename, int type)
{
  struct PGM *pgm;
  int fp,taskid;
  unsigned int filesize;
  unsigned int pagesize;
  void *loadbuf,*progaddr;
  int r;
  int n;
  char userargs[CFG_MEM_USERARGUMENTSZ];

  mutex_lock(&pgm_tbl_mutex);
  
  if(pgmtbl==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTINIT;
  }
  pgm=pgm_alloc();
  if(pgm==NULL) {
    console_puts("pgm alloc error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_RESOURCE;
  }

  fp = fat_open_file(filename, FAT_O_RDONLY);
  if(fp<0) {
    pgm_free(pgm);
/*
    console_puts("file open error=");
    console_puts(filename);
    console_puts("\n");
*/
    mutex_unlock(&pgm_tbl_mutex);
    return fp;
  }

  r=fat_get_filesize(fp,&filesize);
  if(r<0) {
    pgm_free(pgm);
    console_puts("file size error=");
    console_puts(filename);
    console_puts("\n");
    mutex_unlock(&pgm_tbl_mutex);
    return r;
  }
/*
  console_puts("FileSize=");
  int2dec(memsz,s);
  console_puts(s);
  console_puts("\n");
*/
  pagesize=page_frame_size(filesize);
  pgm->pgd=page_create_pgd();
  if(pgm->pgd==NULL){
    pgm_free(pgm);
    console_puts("pgd alloc error\n");
    mutex_unlock(&pgm_tbl_mutex);
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
  r=page_alloc_vmem(pgm->pgd,progaddr,pagesize,(PAGE_TYPE_USER|PAGE_TYPE_RDWR));
  if(r<0){
    pgm_free(pgm);
    console_puts("vmem alloc error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_RESOURCE;
  }

  loadbuf=mem_alloc(MEM_PAGESIZE);
  if(loadbuf==0) {
    pgm_free(pgm);
    console_puts("mem alloc error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_RESOURCE;
  }

//mem_dumpfree();

  for(n=0;n<filesize;n+=512) {

    r=fat_read_file(fp,loadbuf,512);
    if(r<0) {
      pgm_free(pgm);
      console_puts("file read error\n");
      mutex_unlock(&pgm_tbl_mutex);
      return r;
    }
    page_memcpy(pgm->pgd,(void*)((unsigned int)progaddr+n),loadbuf, 512);
  }
  
  r=fat_close_file(fp);
  if(r<0) {
    pgm_free(pgm);
    console_puts("file close error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return r;
  }
  mem_free(loadbuf,MEM_PAGESIZE);

//mem_dumpfree();

//page_dump_pgd(tstpgd);

  r = page_alloc_vmem(pgm->pgd,(void*)(CFG_MEM_USERDATAMAX-PAGE_PAGESIZE),PAGE_PAGESIZE,(PAGE_TYPE_USER|PAGE_TYPE_RDWR));
  if(r<0){
    pgm_free(pgm);
    console_puts("vmem alloc error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_RESOURCE;
  }

  memset(userargs,0,CFG_MEM_USERARGUMENTSZ);
  page_memcpy(pgm->pgd,(void*)(CFG_MEM_USERARGUMENT),userargs, CFG_MEM_USERARGUMENTSZ);

  taskid = task_create_process(pgm->pgd,progaddr,CFG_MEM_USERSTACKTOP);
  if(taskid<0) {
    pgm_free(pgm);
    console_puts("task create error\n");
    mutex_unlock(&pgm_tbl_mutex);
    return taskid;
  }

  if(type&PGM_TYPE_IO)
    task_enable_io(taskid);

  if(type&PGM_TYPE_VGA || 1) {// debug
    if(pgm->pgd!=0) {
      r=page_map_vga(pgm->pgd);
      if(r<0) {
        pgm_free(pgm);
        console_puts("vga map error\n");
        mutex_unlock(&pgm_tbl_mutex);
        return r;
      }
    }
  }

  pgm->taskid = taskid;

  strncpy(pgm->pgmname,filename,sizeof(pgm->pgmname));
  mutex_unlock(&pgm_tbl_mutex);

  return taskid;
}

int program_set_args(int taskid, char *args, int argsize)
{
  struct PGM *pgm;
  char userargs[CFG_MEM_USERARGUMENTSZ];
  int userargssz=CFG_MEM_USERARGUMENTSZ;

  mutex_lock(&pgm_tbl_mutex);

  if(pgmtbl==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTINIT;
  }
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  memset(userargs,0,CFG_MEM_USERARGUMENTSZ);
  if(userargssz>argsize)
    userargssz=argsize;
  memcpy(userargs,args,userargssz);
  page_memcpy(pgm->pgd,(void*)(CFG_MEM_USERARGUMENT),userargs, CFG_MEM_USERARGUMENTSZ);

  mutex_unlock(&pgm_tbl_mutex);
  return 0;
}

int program_start(int taskid, int exitque)
{
  struct PGM *pgm;

  mutex_lock(&pgm_tbl_mutex);

  if(pgmtbl==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTINIT;
  }
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  pgm->exitque=exitque;

  task_start(taskid);

  mutex_unlock(&pgm_tbl_mutex);
  return 0;
}

int program_get_exitque(int taskid)
{
  struct PGM *pgm;

  mutex_lock(&pgm_tbl_mutex);

  if(pgmtbl==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTINIT;
  }
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  mutex_unlock(&pgm_tbl_mutex);
  return pgm->exitque;
}

int program_delete(int taskid)
{
  struct PGM *pgm;
  int exitcode;
  void *fpu;

  mutex_lock(&pgm_tbl_mutex);

  if(pgmtbl==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTINIT;
  }
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    mutex_unlock(&pgm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  fpu=task_get_fpu(taskid);
  if(fpu)
    fpu_free(fpu);
  exitcode=task_delete(taskid);

  if(pgm->pgd!=NULL)
    page_delete_pgd(pgm->pgd);

  pgm_delete(taskid);

  mutex_unlock(&pgm_tbl_mutex);

  return exitcode;
}

int program_exitevent(struct msg_head *msg)
{
  struct PGM *pgm;
  int taskid;
  int exitcode;
  int exitque;

  taskid=msg->arg;
  if(pgmtbl==NULL) {
    return ERRNO_NOTINIT;
  }
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    return ERRNO_NOTEXIST;
  }
  if(task_get_status(taskid)==TASK_STAT_ZONBIE) {
    exitque=program_get_exitque(taskid);
    if(exitque==0) {
      exitcode=program_delete(taskid);
      return exitcode;
    }
    else { // exitque!=0
      queue_put(exitque,msg);
    }
  }
  else {
    console_puts("exitevent not zonbie taskid=");
    int2dec(taskid,s);
    console_puts(s);
    console_puts("\n");
  }

  return -1;
}

int program_set_taskque(int queid)
{
  struct PGM *pgm;
  int taskid;

  taskid = task_get_currentid();
  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    return ERRNO_NOTEXIST;
  }
  pgm->taskque=queid;
  return 0;
}

int program_get_taskque(int taskid)
{
  struct PGM *pgm;

  pgm=pgm_find(taskid);
  if(pgm==NULL) {
    return ERRNO_NOTEXIST;
  }
  return pgm->taskque;
}

int program_list(int start, int count, struct kmem_program *plist)
{
  struct PGM *pgm;
  int i=0;
  int n=0;

  list_for_each(pgmtbl,pgm)
  {
    if(i>=start) {
      plist->id = pgm->id;
      plist->taskid = pgm->taskid;
      plist->status = task_get_status(pgm->taskid);
      plist->exitque = pgm->exitque;
      plist->taskque = pgm->taskque;
      plist->pgd = pgm->pgd;
      strncpy(plist->pgmname,pgm->pgmname,sizeof(plist->pgmname));
      plist++;
      n++;
      if(n>=count)
        break;
    }
  }
  return n;
}
