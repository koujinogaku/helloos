/*
** syscall.c --- system call
*/
#include "syscall.h"

#define SYSCALL_0(syscall_number, result)                                         \
    asm volatile("movl $%c1, %%eax \n"                                            \
                 "int  $0x40       \n"                                            \
                 "movl %%eax, %0   \n"                                            \
                 :"=m"(result)                                                    \
                 :"g"(syscall_number)                                             \
                 :"ebx"                                                           \
                 );                                                               \

#define SYSCALL_1(syscall_number, result, arg1)                                   \
    asm volatile("movl $%c1, %%eax \n"                                            \
                 "movl %2  , %%ebx \n"                                            \
                 "int  $0x40       \n"                                            \
                 "movl %%eax, %0   \n"                                            \
                 :"=m"(result)                                                    \
                 :"g"(syscall_number), "m"(arg1)                                  \
                 );                                                               \

#define SYSCALL_2(syscall_number, result, arg1, arg2)                             \
    asm volatile("movl $%c1, %%eax \n"                                            \
                 "movl %2  , %%ebx \n"                                            \
                 "movl %3  , %%ecx \n"                                            \
                 "int  $0x40       \n"                                            \
                 "movl %%eax, %0   \n"                                            \
                 :"=m"(result)                                                    \
                 :"g"(syscall_number), "m"(arg1), "m"(arg2)                       \
                 );                                                               \

#define SYSCALL_3(syscall_number, result, arg1, arg2, arg3)                       \
    asm volatile("movl $%c1, %%eax \n"                                            \
                 "movl %2  , %%ebx \n"                                            \
                 "movl %3  , %%ecx \n"                                            \
                 "movl %4  , %%edx \n"                                            \
                 "int  $0x40       \n"                                            \
                 "movl %%eax, %0   \n"                                            \
                 :"=m"(result)                                                    \
                 :"g"(syscall_number), "m"(arg1), "m"(arg2), "m"(arg3)            \
                 );                                                               \

int syscall_tst3(int c1,int c2, int c3)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_TST3, r, c1, c2, c3);
  return r;
}
int syscall_putc(int c)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_PUTC, r, c);
  return r;
}
int syscall_puts(char *s)
{
  int r=0;
  r=*((int*)s); // check memory fault
  SYSCALL_1(SYSCALL_FN_PUTS, r, s);
  return r;
}
int syscall_getcursor(void)
{
  int r=0;
  SYSCALL_0(SYSCALL_FN_GETCURSOR, r);
  return r;
}
int syscall_setcursor(int pos)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_SETCURSOR, r, pos);
  return r;
}
int syscall_exit(int c)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_EXIT, r, c);
  return r;
}
int syscall_wait(int c)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_WAIT, r, c);
  return r;
}

int syscall_shm_create(unsigned int shmname, unsigned long size, unsigned int options)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_SHM_CREATE, r, shmname, size, options);
  return r;
}
int syscall_shm_setname(int shmid, unsigned int shmname)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_SETNAME, r, shmid, shmname);
  return r;
}
int syscall_shm_lookup(unsigned int shmname)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_SHM_LOOKUP, r, shmname);
  return r;
}
int syscall_shm_delete(int shmid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_SHM_DELETE, r, shmid);
  return r;
}
int syscall_shm_getsize(int shmid, unsigned long *sizep)
{
  int r=0;
  int sizepv = (int)sizep;
  SYSCALL_2(SYSCALL_FN_SHM_GETSIZE, r, shmid, sizepv);
  return r;
}
int syscall_shm_map(int shmid, void *vmem)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_MAP, r, shmid, vmem);
  return r;
}
int syscall_shm_unmap(int shmid, void *vmem)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_UNMAP, r, shmid, vmem);
  return r;
}
int syscall_shm_getphysical(int shmid, unsigned int pagenum, unsigned long *addr)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_SHM_GETPHYSICAL, r, shmid, pagenum, addr);
  return r;
}
int syscall_shm_pull(int shmid, unsigned int pagenum, void *addr)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_SHM_PULL, r, shmid, pagenum, addr);
  return r;
}

int syscall_que_create(unsigned int quename)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_QUE_CREATE, r, quename);
  return r;
}
int syscall_que_setname(int queid, unsigned int quename)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_QUE_SETNAME, r, queid, quename);
  return r;
}
int syscall_que_delete(int queid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_QUE_DELETE, r, queid);
  return r;
}
int syscall_que_put(int queid, void *msg)
{
  int r;
  r=*((int*)msg); // check memory fault
  SYSCALL_2(SYSCALL_FN_QUE_PUT, r, queid, msg);
  return r;
}
int syscall_que_get(int queid, void *msg)
{
  int r=0;
  r=*((int*)msg); // check memory fault
  SYSCALL_2(SYSCALL_FN_QUE_GET, r, queid, msg);
  return r;
}
int syscall_que_tryget(int queid, void *msg)
{
  int r=0;
  r=*((int*)msg); // check memory fault
  SYSCALL_2(SYSCALL_FN_QUE_TRYGET, r, queid, msg);
  return r;
}
int syscall_que_peeksize(int queid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_QUE_PEEKSIZE, r, queid);
  return r;
}
int syscall_que_trypeeksize(int queid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_QUE_TRYPEEKSIZE, r, queid);
  return r;
}
int syscall_que_lookup(unsigned int quename)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_QUE_LOOKUP, r, quename);
  return r;
}
int syscall_que_list(int start, int count, void *qlist)
{
  int r=0;
  r=*((int*)qlist); // check memory fault
  SYSCALL_3(SYSCALL_FN_QUE_LIST, r, start, count, qlist);
  return r;
}

int syscall_intr_regist(int irq, int queid)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_INTR_REGIST, r, irq, queid);
  return r;
}

int syscall_dbg_locputs(int x, int y, char *s)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_DBG_LOCPUTS, r, x, y, s);
  return r;
}

int syscall_pgm_load(char *filename, int type)
{
  int r=0;
  r=*((int*)filename); // check memory fault
  SYSCALL_2(SYSCALL_FN_PGM_LOAD, r, filename, type);
  return r;
}
int syscall_pgm_allocate(char *name, int type, unsigned long size)
{
  int r=0;
  r=*((int*)name); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_ALLOCATE, r, name, type, size);
  return r;
}
int syscall_pgm_loadimage(int taskid, void *image, unsigned long size)
{
  int r=0;
  r=*((int*)image); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_LOADIMAGE, r, taskid, image, size);
  return r;
}
int syscall_pgm_setargs(int taskid, char *args, int argsize)
{
  int r=0;
  r=*((int*)args); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_SETARGS, r, taskid, args, argsize);
  return r;
}
int syscall_pgm_getargs(int taskid, char *args, int argsize)
{
  int r=0;
  r=*((int*)args); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_GETARGS, r, taskid, args, argsize);
  return r;
}
int syscall_pgm_start(int taskid, int exitque)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_PGM_START, r, taskid, exitque);
  return r;
}
int syscall_pgm_delete(int taskid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_PGM_DELETE, r, taskid);
  return r;
}
int syscall_pgm_settaskq(int queid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_PGM_SETTASKQ, r, queid);
  return r;
}
int syscall_pgm_gettaskq(int taskid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_PGM_GETTASKQ, r, taskid);
  return r;
}
int syscall_pgm_getexitcode(int taskid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_PGM_GETEXITCODE, r, taskid);
  return r;
}
int syscall_pgm_list(int start, int count, void *plist)
{
  int r=0;
  r=*((int*)plist); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_LIST, r, start, count, plist);
  return r;
}

int syscall_file_open(char *filename, int mode)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_FILE_OPEN, r, filename, mode);
  return r;
}
int syscall_file_close(int fp)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_FILE_CLOSE, r, fp);
  return r;
}
int syscall_file_size(int fp, unsigned int *size)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_FILE_SIZE, r, fp, size);
  return r;
}
int syscall_file_read(int fp, void* buf, unsigned int size)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_FILE_READ, r, fp, buf, size);
  return r;
}
int syscall_file_opendir(unsigned long *vdirdesc, char *dirname)
{
  int r=0;
  r=*((int*)vdirdesc); // check memory fault
  r+=*((int*)dirname); // check memory fault
  SYSCALL_2(SYSCALL_FN_FILE_DIROPEN, r, vdirdesc, dirname);
  return r;
}
int syscall_file_readdir(unsigned long *vdirdesc, char *dirent)
{
  int r=0;
  r=*((int*)vdirdesc); // check memory fault
  r+=*((int*)dirent);  // check memory fault
  SYSCALL_2(SYSCALL_FN_FILE_DIRREAD, r, vdirdesc, dirent);
  return r;
}
int syscall_file_closedir(unsigned long *vdirdesc)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_FILE_DIRCLOSE, r, vdirdesc);
  return r;
}
int syscall_krn_get_bios_info(char *binfo)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_KRN_GET_BIOS_INFO, r, binfo);
  return r;
}
int syscall_krn_memory_status(unsigned long *status)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_KRN_MEMORY_STATUS, r, status);
  return r;
}
int syscall_krn_get_systime(void *systime)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_KRN_GET_SYSTIME, r, systime);
  return r;
}
int syscall_alarm_set(unsigned int alarmtime, int queid, int arg)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_ALARM_SET, r, alarmtime, queid, arg);
  return r;
}
int syscall_alarm_unset(int alarmid, int queid)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_ALARM_UNSET, r, alarmid, queid);
  return r;
}
int syscall_mtx_lock(int *mutex)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_MTX_LOCK, r, mutex);
  return r;
}
int syscall_mtx_trylock(int *mutex)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_MTX_TRYLOCK, r, mutex);
  return r;
}
int syscall_mtx_unlock(int *mutex)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_MTX_UNLOCK, r, mutex);
  return r;
}
int syscall_dma_setmode( unsigned int channel, unsigned int mode )
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_DMA_SETMODE, r, channel, mode);
  return r;
}
int syscall_dma_enable( unsigned int channel, unsigned int sw )
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_DMA_ENABLE, r, channel, sw);
  return r;
}
int syscall_dma_allocbuffer(unsigned int channel, unsigned long size)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_DMA_ALLOCBUFFER, r, channel, size);
  return r;
}
int syscall_dma_freebuffer(unsigned int channel)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_DMA_FREEBUFFER, r, channel);
  return r;
}
int syscall_dma_setbuffer(unsigned int channel)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_DMA_SETBUFFER, r, channel);
  return r;
}
int syscall_dma_pushbuffer(unsigned int channel, void* buf)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_DMA_PUSHBUFFER, r, channel, buf);
  return r;
}
int syscall_dma_pullbuffer(unsigned int channel, void* buf)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_DMA_PULLBUFFER, r, channel, buf);
  return r;
}
