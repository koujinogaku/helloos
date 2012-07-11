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

int syscall_shm_create(int shmid, int size)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_CREATE, r, shmid, size);
  return r;
}
int syscall_shm_delete(int shmid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_SHM_DELETE, r, shmid);
  return r;
}
int syscall_shm_getsize(int shmid, int sizep)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_GETSIZE, r, shmid, sizep);
  return r;
}
int syscall_shm_map(int shmid, int vmem)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_MAP, r, shmid, vmem);
  return r;
}
int syscall_shm_unmap(int shmid, int vmem)
{
  int r=0;
  SYSCALL_2(SYSCALL_FN_SHM_UNMAP, r, shmid, vmem);
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
int syscall_pgm_setargs(int taskid, char *args, int argsize)
{
  int r=0;
  r=*((int*)args); // check memory fault
  SYSCALL_3(SYSCALL_FN_PGM_SETARGS, r, taskid, args, argsize);
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
int syscall_kernel_get_bios_info(char *binfo)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_KERNEL_GET_BIOS_INFO, r, binfo);
  return r;
}
int syscall_alarm_set(unsigned int alarmtime, int queid, int arg)
{
  int r=0;
  SYSCALL_3(SYSCALL_FN_ALARM_SET, r, alarmtime, queid, arg);
  return r;
}
int syscall_alarm_unset(int alarmid)
{
  int r=0;
  SYSCALL_1(SYSCALL_FN_ALARM_UNSET, r, alarmid);
  return r;
}
