#ifndef BUCKET_H
#define BUCKET_H

#define BUCKET_SELECT_DATA 1
#define BUCKET_SELECT_MSG  2
#define BUCKET_ALARM_COMMAND 0x8002

typedef struct _fd_set { unsigned long fds[4]; } fd_set;

#define FD_SET(fd,rfds)    ((rfds)->fds[(fd)/32] |= (1<<((fd)%32)))
#define FD_ZERO(rfds)      { int i; for(i=0;i<4;i++) { (rfds)->fds[i] = 0; }}
#define FD_CLR(fd,rfds)     ((rfds)->fds[(fd)/32] &= ((~1)<<((fd)%32)))
#define FD_ISSET(fd, rfds) ((rfds)->fds[(fd)/32] & (1<<((fd)%32)))

int bucket_open(void);
int bucket_bind(int dsc, int que_name, int service);
int bucket_connect(int dsc, int que_name, int service);
int bucket_accept(int dsc);
int bucket_find_dsc(int service, int queid);
int bucket_has_block(int dsc);
int bucket_select(int fdsetsize, fd_set *rfds, unsigned long  timeout);
void *bucket_selected_msg(void);
int bucket_send(int dsc, void *buffer, int size);
int bucket_recv(int dsc, void *buffer, int size);
int bucket_shutdown(int dsc);
int bucket_close(int dsc);

#endif
