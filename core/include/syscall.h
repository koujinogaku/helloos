/*
** syscall.h --- system call
*/
#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_INTRNO 0x40

#define SYSCALL_FN_TST3          0 
#define SYSCALL_FN_EXIT          1 
#define SYSCALL_FN_PUTC          2 
#define SYSCALL_FN_PUTS          3 
#define SYSCALL_FN_GETCURSOR     4 
#define SYSCALL_FN_SETCURSOR     5 
#define SYSCALL_FN_WAIT          6 
#define SYSCALL_FN_SHM_CREATE    7 
#define SYSCALL_FN_SHM_DELETE    8 
#define SYSCALL_FN_SHM_GETSIZE   9 
#define SYSCALL_FN_SHM_MAP       10
#define SYSCALL_FN_SHM_UNMAP     11
#define SYSCALL_FN_QUE_CREATE    12
#define SYSCALL_FN_QUE_SETNAME   13
#define SYSCALL_FN_QUE_DELETE    14
#define SYSCALL_FN_QUE_PUT       15
#define SYSCALL_FN_QUE_GET       16
#define SYSCALL_FN_QUE_TRYGET    17
#define SYSCALL_FN_QUE_LOOKUP    18
#define SYSCALL_FN_QUE_PEEKSIZE  19
#define SYSCALL_FN_QUE_TRYPEEKSIZE 20
#define SYSCALL_FN_INTR_REGIST   21
#define SYSCALL_FN_DBG_LOCPUTS   22
#define SYSCALL_FN_PGM_LOAD      23
#define SYSCALL_FN_PGM_SETARGS   24
#define SYSCALL_FN_PGM_START     25
#define SYSCALL_FN_PGM_DELETE    26
#define SYSCALL_FN_FILE_OPEN     27
#define SYSCALL_FN_FILE_CLOSE    28
#define SYSCALL_FN_FILE_READ     29
//#define SYSCALL_FN_FILE_WRITE  30
//#define SYSCALL_FN_FILE_SEEK   31
#define SYSCALL_FN_FILE_SIZE     32
//#define SYSCALL_FN_FILE_ATTR   33
#define SYSCALL_FN_FILE_DIROPEN  34
#define SYSCALL_FN_FILE_DIRREAD  35
#define SYSCALL_FN_FILE_DIRCLOSE 36
#define SYSCALL_FN_KRN_GET_BIOS_INFO 37
#define SYSCALL_FN_ALARM_SET     38
#define SYSCALL_FN_ALARM_UNSET   39
#define SYSCALL_FN_KRN_MEMORY_STATUS 40
#define SYSCALL_FN_MTX_LOCK      41
#define SYSCALL_FN_MTX_TRYLOCK   42
#define SYSCALL_FN_MTX_UNLOCK    43
#define SYSCALL_FN_PGM_LIST      44
#define SYSCALL_FN_QUE_LIST      45
#define SYSCALL_FN_PGM_SETTASKQ  46
#define SYSCALL_FN_PGM_GETTASKQ  47
#define SYSCALL_FN_KRN_GET_SYSTIME 48

void syscall_init(void);

int syscall_tst3(int c1,int c2, int c3);
int syscall_putc(int c);
int syscall_puts(char *s);
int syscall_getcursor(void);
int syscall_setcursor(int pos);
int syscall_puts(char *s);
int syscall_exit(int c);
int syscall_wait(int c);

int syscall_shm_create(int shmid, int size);
int syscall_shm_delete(int shmid);
int syscall_shm_getsize(int shmid,int sizep);
int syscall_shm_map(int shmid,int vmem);
int syscall_shm_unmap(int shmid, int vmem);

int syscall_que_create(unsigned int quename);
int syscall_que_setname(int queid, unsigned int quename);
int syscall_que_delete(int queid);
int syscall_que_put(int queid, void *msg);
int syscall_que_get(int queid, void *msg);
int syscall_que_tryget(int queid, void *msg);
int syscall_que_lookup(unsigned int quename);
int syscall_que_peeksize(int queid);
int syscall_que_trypeeksize(int queid);
int syscall_que_list(int start, int count, void *qlist);

int syscall_mtx_lock(int *mutex);
int syscall_mtx_trylock(int *mutex);
int syscall_mtx_unlock(int *mutex);


#define SYSCALL_PGM_TYPE_IO  0x00000001
#define SYSCALL_PGM_TYPE_VGA 0x00000002
int syscall_pgm_load(char *filename, int type);
int syscall_pgm_setargs(int taskid, char *args, int argsize);
int syscall_pgm_start(int taskid, int exitque);
int syscall_pgm_delete(int taskid);
int syscall_pgm_settaskq(int queid);
int syscall_pgm_gettaskq(int taskid);
int syscall_pgm_list(int start, int count, void *plist);

#define SYSCALL_FILE_O_RDONLY 0x0001
#define SYSCALL_FILE_O_WRONLY 0x0002
#define SYSCALL_FILE_O_RDWR   0x0003
int syscall_file_open(char *filename, int mode);
int syscall_file_close(int fp);
int syscall_file_size(int fp, unsigned int *size);
int syscall_file_read(int fp, void* buf, unsigned int size);
int syscall_file_opendir(unsigned long *vdirdesc, char *dirname);
int syscall_file_readdir(unsigned long *vdirdesc, char *dirent);
int syscall_file_closedir(unsigned long *vdirdesc);
int syscall_krn_get_bios_info(char *binfo);
int syscall_krn_memory_status(unsigned long *status);
int syscall_krn_get_systime(void *systime);
int syscall_alarm_set(unsigned int alarmtime, int queid, int arg);
int syscall_alarm_unset(int alarmid, int queid);


int syscall_intr_regist(int irq, int queid);

int syscall_dbg_locputs(int x, int y, char *s);

#endif /* SYSCALL_H */
