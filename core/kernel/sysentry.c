/*
** syscall.c --- system call
*/
#include "config.h"
#include "intr.h"
#include "syscall.h"
#include "cpu.h"
#include "desc.h"
#include "string.h"
#include "console.h"
#include "task.h"
#include "alarm.h"
#include "page.h"
#include "shrmem.h"
#include "queue.h"
#include "kprogram.h"
#include "fat.h"
#include "kernel.h"
#include "kmemory.h"
#include "mutex.h"
#include "kmem.h"
#include "timer.h"
#include "errno.h"
#include "dma.h"

extern int syscall_entry_intr(void);

asm (\
".text				\n"
"syscall_entry_intr:            \n"
"       push %eax               \n"
"       pushal                  \n"
"       push %ds                \n"
"       push %es                \n"
"       push $0x08              \n" /*DESC_SELECTOR(DESC_KERNEL_DATA)*/
"       pop %es                 \n"
"       push $0x08              \n" /*DESC_SELECTOR(DESC_KERNEL_DATA)*/
"       pop %ds                 \n"
"       push %edx               \n"
"       push %ecx               \n"
"       push %ebx               \n"
"       push %eax               \n"
"       call syscall_entry      \n"
"       addl $16, %esp          \n"
"       movl %eax,40(%esp)      \n" /* set return code on stack*/
"       pop %es                 \n"
"       pop %ds                 \n"
"       popal                   \n"
"       pop %eax                \n"
"       iretl                   \n"
);

// static char s[32];

void syscall_init(void)
{
  intr_define(SYSCALL_INTRNO, (int)syscall_entry_intr,INTR_DPL_USER);
}

int syscall_entry(int eax,int ebx,int ecx,int edx)
{
/*
  console_puts("[a=");
  int2str(eax,s);
  console_puts(s);
  console_puts(",b=");
  int2str(ebx,s);
  console_puts(s);
  console_puts(",c=");
  int2str(ecx,s);
  console_puts(s);
  console_puts(",d=");
  int2str(edx,s);
  console_puts(s);
  console_puts("]");
*/
  cpu_unlock();

  task_set_lastfunc(eax);

  switch(eax) {
  case SYSCALL_FN_PUTC:
    eax = syscall_putc(ebx);
    break;
  case SYSCALL_FN_PUTS:
    eax = syscall_puts((char*)ebx);
    break;
  case SYSCALL_FN_GETCURSOR:
    eax = syscall_getcursor();
    break;
  case SYSCALL_FN_SETCURSOR:
    eax = syscall_setcursor(ebx);
    break;
  case SYSCALL_FN_EXIT:
    eax = syscall_exit(ebx);
    break;
  case SYSCALL_FN_WAIT:
    eax = syscall_wait(ebx);
    break;
  case SYSCALL_FN_SHM_CREATE:
    eax = syscall_shm_create((unsigned int)ebx, (unsigned long)ecx, (unsigned int)edx);
    break;
  case SYSCALL_FN_SHM_SETNAME:
    eax = syscall_shm_setname(ebx, (unsigned int)ecx);
    break;
  case SYSCALL_FN_SHM_LOOKUP:
    eax = syscall_shm_lookup((unsigned int)ebx);
    break;
  case SYSCALL_FN_SHM_DELETE:
    eax = syscall_shm_delete(ebx);
    break;
  case SYSCALL_FN_SHM_GETSIZE:
    eax = syscall_shm_getsize(ebx, (unsigned long *)ecx);
    break;
  case SYSCALL_FN_SHM_MAP:
    eax = syscall_shm_map(ebx, (void *)ecx);
    break;
  case SYSCALL_FN_SHM_UNMAP:
    eax = syscall_shm_unmap(ebx, (void *)ecx);
    break;
  case SYSCALL_FN_SHM_GETPHYSICAL:
    eax = syscall_shm_getphysical(ebx, (unsigned int)ecx, (unsigned long *)edx);
    break;
  case SYSCALL_FN_SHM_PULL:
    eax = syscall_shm_pull(ebx, (unsigned int)ecx, (void *)edx);
    break;
  case SYSCALL_FN_QUE_CREATE:
    eax = syscall_que_create((unsigned int)ebx);
    break;
  case SYSCALL_FN_QUE_SETNAME:
    eax = syscall_que_setname(ebx, (unsigned int)ecx);
    break;
  case SYSCALL_FN_QUE_DELETE:
    eax = syscall_que_delete(ebx);
    break;
  case SYSCALL_FN_QUE_PUT:
    eax = syscall_que_put(ebx, (void *)ecx);
    break;
  case SYSCALL_FN_QUE_GET:
    eax = syscall_que_get(ebx, (void *)ecx);
    break;
  case SYSCALL_FN_QUE_TRYGET:
    eax = syscall_que_tryget(ebx, (void *)ecx);
    break;
  case SYSCALL_FN_QUE_PEEKSIZE:
    eax = syscall_que_peeksize(ebx);
    break;
  case SYSCALL_FN_QUE_TRYPEEKSIZE:
    eax = syscall_que_trypeeksize(ebx);
    break;
  case SYSCALL_FN_QUE_LOOKUP:
    eax = syscall_que_lookup((unsigned int)ebx);
    break;
  case SYSCALL_FN_QUE_LIST:
    eax = syscall_que_list(ebx, ecx, (void*)edx);
    break;
  case SYSCALL_FN_INTR_REGIST:
    eax = syscall_intr_regist(ebx, ecx);
    break;
  case SYSCALL_FN_DBG_LOCPUTS:
    eax = syscall_dbg_locputs(ebx, ecx, (char *)edx);
    break;
  case SYSCALL_FN_PGM_LOAD:
    eax = syscall_pgm_load((char *)ebx, ecx);
    break;
  case SYSCALL_FN_PGM_ALLOCATE:
    eax = syscall_pgm_allocate((char *)ebx, ecx, (unsigned long)edx);
    break;
  case SYSCALL_FN_PGM_LOADIMAGE:
    eax = syscall_pgm_loadimage(ebx, (void *)ecx, (unsigned long)edx);
    break;
  case SYSCALL_FN_PGM_SETARGS:
    eax = syscall_pgm_setargs(ebx, (char *)ecx, edx);
    break;
  case SYSCALL_FN_PGM_GETARGS:
    eax = syscall_pgm_getargs(ebx, (char *)ecx, edx);
    break;
  case SYSCALL_FN_PGM_START:
    eax = syscall_pgm_start(ebx, ecx);
    break;
  case SYSCALL_FN_PGM_DELETE:
    eax = syscall_pgm_delete(ebx);
    break;
  case SYSCALL_FN_PGM_SETTASKQ:
    eax = syscall_pgm_settaskq(ebx);
    break;
  case SYSCALL_FN_PGM_GETTASKQ:
    eax = syscall_pgm_gettaskq(ebx);
    break;
  case SYSCALL_FN_PGM_GETEXITCODE:
    eax = syscall_pgm_getexitcode(ebx);
    break;
  case SYSCALL_FN_PGM_LIST:
    eax = syscall_pgm_list(ebx, ecx, (void*)edx);
    break;
  case SYSCALL_FN_FILE_OPEN:
    eax = syscall_file_open((char *)ebx, ecx);
    break;
  case SYSCALL_FN_FILE_CLOSE:
    eax = syscall_file_close(ebx);
    break;
  case SYSCALL_FN_FILE_READ:
    eax = syscall_file_read(ebx, (void*)ecx, edx);
    break;
  case SYSCALL_FN_FILE_SIZE:
    eax = syscall_file_size(ebx, (unsigned int *)ecx);
    break;
  case SYSCALL_FN_FILE_DIROPEN:
    eax = syscall_file_opendir((unsigned long *)ebx, (char *)ecx);
    break;
  case SYSCALL_FN_FILE_DIRREAD:
    eax = syscall_file_readdir((unsigned long *)ebx, (char *)ecx);
    break;
  case SYSCALL_FN_FILE_DIRCLOSE:
    eax = syscall_file_closedir((unsigned long *)ebx);
    break;
  case SYSCALL_FN_KRN_GET_BIOS_INFO:
    eax = syscall_krn_get_bios_info((char *)ebx);
    break;
  case SYSCALL_FN_KRN_MEMORY_STATUS:
    eax = syscall_krn_memory_status((unsigned long *)ebx);
    break;
  case SYSCALL_FN_KRN_GET_SYSTIME:
    eax = syscall_krn_get_systime((void *)ebx);
    break;
  case SYSCALL_FN_ALARM_SET:
    eax = syscall_alarm_set((unsigned int)ebx, ecx, edx);
    break;
  case SYSCALL_FN_ALARM_UNSET:
    eax = syscall_alarm_unset(ebx, ecx);
    break;
  case SYSCALL_FN_MTX_LOCK:
    eax = syscall_mtx_lock((int *)ebx);
    break;
  case SYSCALL_FN_MTX_TRYLOCK:
    eax = syscall_mtx_trylock((int *)ebx);
    break;
  case SYSCALL_FN_MTX_UNLOCK:
    eax = syscall_mtx_unlock((int *)ebx);
    break;
  case SYSCALL_FN_DMA_SETMODE:
    eax = syscall_dma_setmode( (unsigned int)ebx, (unsigned int)ecx );
    break;
  case SYSCALL_FN_DMA_ENABLE:
    eax = syscall_dma_enable( (unsigned int)ebx, (unsigned int)ecx );
    break;
  case SYSCALL_FN_DMA_ALLOCBUFFER:
    eax = syscall_dma_allocbuffer( (unsigned int)ebx, (unsigned long)ecx );
    break;
  case SYSCALL_FN_DMA_FREEBUFFER:
    eax = syscall_dma_freebuffer( (unsigned int)ebx );
    break;
  case SYSCALL_FN_DMA_SETBUFFER:
    eax = syscall_dma_setbuffer( (unsigned int)ebx );
    break;
  case SYSCALL_FN_DMA_PUSHBUFFER:
    eax = syscall_dma_pushbuffer( (unsigned int)ebx, (void*)ecx );
    break;
  case SYSCALL_FN_DMA_PULLBUFFER:
    eax = syscall_dma_pullbuffer( (unsigned int)ebx, (void*)ecx );
    break;
  default:
    eax = -1;
  }

  return eax;
}



int syscall_putc(int c)
{
  console_putc(c);
  return 0;
}
int syscall_puts(char *s)
{
  console_puts(s);
  return 0;
}
int syscall_getcursor(void)
{
  return console_getpos();
}
int syscall_setcursor(int pos)
{
  console_putpos(pos);
  return 0;
}
int syscall_exit(int c)
{
  task_exit(0x7fff & c);
  return 0;
}
int syscall_wait(int c)
{
  return alarm_wait(c);
}

int syscall_shm_create(unsigned int shmname, unsigned long size, unsigned int options)
{
  return shm_create(shmname, size, options);
}
int syscall_shm_setname(int shmid, unsigned int shmname)
{
  return shm_setname(shmid, shmname);
}
int syscall_shm_lookup(unsigned int shmname)
{
  return shm_lookup(shmname);
}
int syscall_shm_delete(int shmid)
{
  return shm_delete(shmid);
}
int syscall_shm_getsize(int shmid, unsigned long *sizep)
{
  if((unsigned long)sizep < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return shm_get_size(shmid, sizep);
}
int syscall_shm_map(int shmid, void *vmem)
{
  if((unsigned long)vmem < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return shm_map(shmid, page_get_current_pgd(), vmem,(PAGE_TYPE_USER|PAGE_TYPE_RDWR));
}
int syscall_shm_unmap(int shmid, void *vmem)
{
  if((unsigned long)vmem < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return shm_unmap(shmid, page_get_current_pgd(), vmem);
}
int syscall_shm_getphysical(int shmid, unsigned int pagenum, unsigned long *addr)
{
  if((unsigned long)addr < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return shm_get_physical(shmid, pagenum, addr);
}
int syscall_shm_pull(int shmid, unsigned int pagenum, void *addr)
{
  if((unsigned long)addr < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return shm_pull(shmid, pagenum, addr);
}

int syscall_que_create(unsigned int quename)
{
  return queue_create(quename);
}
int syscall_que_setname(int queid, unsigned int quename)
{
  return queue_setname(queid, quename);
}
int syscall_que_delete(int queid)
{
  return queue_destroy(queid);
}
int syscall_que_put(int queid, void* msg)
{
  if((unsigned long)msg < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return queue_put(queid, msg);
}
int syscall_que_get(int queid, void *msg)
{
  if((unsigned long)msg < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return queue_get(queid, msg);
}
int syscall_que_tryget(int queid, void *msg)
{
  if((unsigned long)msg < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return queue_tryget(queid, msg);
}
int syscall_que_peeksize(int queid)
{
  return queue_peek_nextsize(queid);
}
int syscall_que_trypeeksize(int queid)
{
  return queue_trypeek_nextsize(queid);
}
int syscall_que_lookup(unsigned int quename)
{
  return queue_lookup(quename);
}
int syscall_que_list(int start, int count, void *qlist)
{
  return queue_list(start, count, qlist);
}

int syscall_intr_regist(int irq, int queid)
{
  return intr_regist_receiver(irq, queid);
}

int syscall_dbg_locputs(int x, int y, char *s)
{
  int pos;
  pos = console_getpos();
  console_locatepos(x,y);
  console_puts(s);
  console_putpos(pos);
  return 0;
}

int syscall_pgm_load(char *filename, int type)
{
  return program_load(filename, type);
}
int syscall_pgm_allocate(char *name, int type, unsigned long size)
{
  return program_allocate(name, type, size);
}
int syscall_pgm_loadimage(int taskid, void *image, unsigned long size)
{
  if((unsigned long)image < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return program_loadimage(taskid, image, size);
}
int syscall_pgm_setargs(int taskid, char *args, int argsize)
{
  if((unsigned long)args < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return program_set_args(taskid, args, argsize);
}
int syscall_pgm_getargs(int taskid, char *args, int argsize)
{
  if((unsigned long)args < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return program_get_args(taskid, args, argsize);
}
int syscall_pgm_start(int taskid, int exitque)
{
  return program_start(taskid, exitque);
}
int syscall_pgm_delete(int taskid)
{
  return program_delete(taskid);
}
int syscall_pgm_settaskq(int queid)
{
  return program_set_taskque(queid);
}
int syscall_pgm_gettaskq(int taskid)
{
  return program_get_taskque(taskid);
}
int syscall_pgm_getexitcode(int taskid)
{
  return program_get_exitcode(taskid);
}

int syscall_pgm_list(int start, int count, void *plist)
{
  if((unsigned long)plist < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return program_list(start, count, plist);
}

int syscall_file_open(char *filename, int mode)
{
  return fat_open_file(filename, mode);
}
int syscall_file_close(int fp)
{
  return fat_close_file(fp);
}
int syscall_file_size(int fp, unsigned int *size)
{
  return fat_get_filesize(fp, size);
}
int syscall_file_read(int fp, void* buf, unsigned int size)
{
  return fat_read_file(fp, buf, size);
}
int syscall_file_opendir(unsigned long *vdirdesc, char *dirname)
{
  return fat_open_dir(vdirdesc, dirname);
}
int syscall_file_readdir(unsigned long *vdirdesc, char *dirent)
{
  return fat_read_dir(vdirdesc, dirent);
}
int syscall_file_closedir(unsigned long *vdirdesc)
{
  return fat_close_dir(vdirdesc);
}

int syscall_krn_get_bios_info(char *binfo)
{
  if((unsigned long)binfo < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return kernel_get_bios_info(binfo);
}

int syscall_krn_memory_status(unsigned long *status)
{
  if((unsigned long)status < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  if(status) {
    *status++ = mem_get_totalsize();
    *status++ = page_get_totalfree()*PAGE_PAGESIZE;
    *status++ = mem_get_kernelsize();
    *status++ = mem_get_kernelfree();
  }
  return 0;
}

int syscall_krn_get_systime(void *systime)
{
  if((unsigned long)systime < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  timer_get_systime((void *)systime);
  return 0;
}

int syscall_alarm_set(unsigned int alarmtime, int queid, int arg)
{
  return alarm_set(alarmtime, queid, arg);
}
int syscall_alarm_unset(int alarmid, int queid)
{
  return alarm_unset(alarmid, queid);
}

int syscall_mtx_lock(int *mutex)
{
  if((unsigned long)mutex < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  mutex_lock(mutex);
  return 0;
}

int syscall_mtx_trylock(int *mutex)
{
  if((unsigned long)mutex < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  return mutex_trylock(mutex);
}

int syscall_mtx_unlock(int *mutex)
{
  if((unsigned long)mutex < CFG_MEM_KERNELMAX)
    return ERRNO_NOTEXIST;
  mutex_unlock(mutex);
  return 0;
}

int syscall_dma_setmode( unsigned int channel, unsigned int mode )
{
  return dma_set_mode( channel, mode );
}
int syscall_dma_enable( unsigned int channel, unsigned int sw )
{
  return dma_enable( channel, sw );
}
int syscall_dma_allocbuffer(unsigned int channel, unsigned long size)
{
  return dma_alloc_buffer(channel, size);
}
int syscall_dma_freebuffer(unsigned int channel)
{
  return dma_free_buffer(channel);
}
int syscall_dma_setbuffer(unsigned int channel)
{
  return dma_set_buffer(channel);
}
int syscall_dma_pushbuffer(unsigned int channel, void* buf)
{
  return dma_push_buffer(channel, buf);
}
int syscall_dma_pullbuffer(unsigned int channel, void* buf)
{
  return dma_pull_buffer(channel, buf);
}
