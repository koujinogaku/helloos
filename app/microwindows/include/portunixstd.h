#ifndef PORTUNIXSTD_H
#define PORTUNIXSTD_H

#define HELLOOS (1)

#include "syscall.h"
#include "display.h"
#include "config.h"
#include "assert.h"

//#include "dbgmemory.h"
//#define malloc(m) dbg_malloc(m)
#include "stdlib.h"

#define sprintf   print_sformat
#define vsprintf  print_vsformat
#define printf    print_format
#define free(m)   mfree(m)
#define getpid()  environment_getqueid()
#define strcpy(dest,src)  strncpy(dest,src,1024)
#define setbuf(fp,bf)  {}
#define memory_chkheapaddr(m)  ( CFG_MEM_USERHEAP <= (unsigned int)(m) && (unsigned int)(m) < CFG_MEM_USERHEAPMAX )

typedef unsigned int size_t;
#define offsetof(type, member) ((size_t)&((type*)0)->member)

#ifndef O_BINARY
#define O_BINARY 	0
#endif
#ifndef O_RDONLY
#define O_RDONLY 0x0001
#endif
#ifndef O_WRONLY
#define O_WRONLY 0x0002
#endif
#ifndef O_RDWR
#define O_RDWR   0x0003
#endif

#ifndef EOF
#define EOF (-1)
#endif
// ===== Socket =====
#define AF_HELLO
#define NX_QNM_BUCKET MSGQUENAMES_WINDOW
#define NX_SRV_BUCKET MSGQUENAMES_WINDOW


//#define SOCK_STREAM     1

struct sockaddr_un {
};

#endif
