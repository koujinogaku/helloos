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
#define SYSCALL_FN_PGM_GETEXITCODE 49
#define SYSCALL_FN_PGM_GETARGS     50
#define SYSCALL_FN_SHM_SETNAME     51
#define SYSCALL_FN_SHM_LOOKUP      52
#define SYSCALL_FN_PGM_ALLOCATE    53
#define SYSCALL_FN_PGM_LOADIMAGE   54
#define SYSCALL_FN_SHM_GETPHYSICAL 55
#define SYSCALL_FN_SHM_PULL        56
#define SYSCALL_FN_DMA_SETMODE     57
#define SYSCALL_FN_DMA_ENABLE      58
#define SYSCALL_FN_DMA_ALLOCBUFFER 59
#define SYSCALL_FN_DMA_FREEBUFFER  60
#define SYSCALL_FN_DMA_SETBUFFER   61
#define SYSCALL_FN_DMA_PUSHBUFFER  62
#define SYSCALL_FN_DMA_PULLBUFFER  63

void syscall_init(void);

int syscall_tst3(int c1,int c2, int c3);
int syscall_putc(int c);
int syscall_puts(char *s);
int syscall_getcursor(void);
int syscall_setcursor(int pos);
int syscall_puts(char *s);
int syscall_exit(int c);
int syscall_wait(int c);

#define SYSCALL_SHM_OPT_KERNEL 0x01
int syscall_shm_create(unsigned int shmname, unsigned long size, unsigned int options);
int syscall_shm_setname(int shmid, unsigned int shmname);
int syscall_shm_lookup(unsigned int shmname);
int syscall_shm_delete(int shmid);
int syscall_shm_getsize(int shmid, unsigned long *sizep);
int syscall_shm_map(int shmid, void *vmem);
int syscall_shm_unmap(int shmid, void *vmem);
int syscall_shm_getphysical(int shmid, unsigned int pagenum, unsigned long *addr);
int syscall_shm_pull(int shmid, unsigned int pagenum, void *addr);

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
int syscall_pgm_allocate(char *name, int type, unsigned long size);
int syscall_pgm_loadimage(int taskid, void *image, unsigned long size);
int syscall_pgm_setargs(int taskid, char *args, int argsize);
int syscall_pgm_getargs(int taskid, char *args, int argsize);
int syscall_pgm_start(int taskid, int exitque);
int syscall_pgm_delete(int taskid);
int syscall_pgm_settaskq(int queid);
int syscall_pgm_gettaskq(int taskid);
int syscall_pgm_getexitcode(int taskid);
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

#define SYSCALL_DMA_MODE_VERIFY   0x00
#define SYSCALL_DMA_MODE_WRITE    0x04
#define SYSCALL_DMA_MODE_READ     0x08
#define SYSCALL_DMA_MODE_AUTOINIT 0x10
#define SYSCALL_DMA_MODE_INCL     0x00
#define SYSCALL_DMA_MODE_DECL     0x20
#define SYSCALL_DMA_MODE_DEMAND   0xc0
#define SYSCALL_DMA_MODE_SINGLE   0x40
#define SYSCALL_DMA_MODE_BLOCK    0x80
#define SYSCALL_DMA_MODE_CASCADE  0xc0
#define SYSCALL_DMA_CHANNEL_FDD     0x02
#define SYSCALL_DMA_CHANNEL_XTDISK  0x03
#define SYSCALL_DMA_CHANNEL_CASCADE 0x04
#define SYSCALL_DMA_ENABLE        1
#define SYSCALL_DMA_DISABLE       0
int syscall_dma_setmode( unsigned int channel, unsigned int mode );
int syscall_dma_enable( unsigned int channel, unsigned int sw );
int syscall_dma_allocbuffer(unsigned int channel, unsigned long size);
int syscall_dma_freebuffer(unsigned int channel);
int syscall_dma_setbuffer(unsigned int channel);
int syscall_dma_pushbuffer(unsigned int channel, void* buf);
int syscall_dma_pullbuffer(unsigned int channel, void* buf);


int syscall_intr_regist(int irq, int queid);

int syscall_dbg_locputs(int x, int y, char *s);

#endif /* SYSCALL_H */
