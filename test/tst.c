#include "syscall.h"
#include "stdlib.h"
#include "string.h"
#include "keyboard.h"
#include "display.h"
#include "memory.h"
#include "errno.h"
#include "config.h"
#include "mouse.h"
#include "message.h"
#include "shm.h"
#include "bucket.h"
#include "environm.h"
//#include "window.h"
#include "math.h"
#include "print.h"

void puts(char *str)
{
  for(;*str != 0;++str)
    display_putc(*str);
}

static char s[10];

void tst_console(void)
{
//  int a;
//  int i;
//  int j;
//  int size;

//  a = display_putc('H');
//  a = display_putc('E');
//  a = display_putc('L');
//  display_putc(a);
//syscall_tst3(12,34,56);

  display_puts("hello\n");

//  exit(123);
/*
  for(i=0;i<5;i++) {
    for(j=0;j<i;j++)
      puts(".");
    puts("Test2\n");
  }
*/
/*
  for(i=0;i<10;i++) {
    puts("2");
    syscall_wait(80);
  }
*/
//  cpu_halt();
}

#define SHRMEMNAME 1
#define SHRMEMNAME2 2
#define SHRMEMADDR ((void *)0x7000000)
#define SHRMEMHEAP ((void *)0x7000000)

void tst_shm(void)
{
  int r;
/*
  r=syscall_shm_create(SHRMEMNAME,64*1024+1);
  display_puts("shm create=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");
*/
  r=syscall_shm_create(SHRMEMNAME,4);
  display_puts("shm create=");
  long2hex(r,s);
  display_puts(s);
//  display_puts("\n");
/*
  r=syscall_shm_create(SHRMEMNAME2,64*1024);
  display_puts("shm2 create=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");

  r=syscall_shm_getsize(SHRMEMNAME,(int)&size);
  display_puts("shm getsize=");
  long2hex(r,s);
  display_puts(s);
  display_puts(",");
  long2hex(size,s);
  display_puts(s);
  display_puts("\n");

  r=syscall_shm_getsize(SHRMEMNAME2,(int)&size);
  display_puts("shm2 getsize=");
  long2hex(r,s);
  display_puts(s);
  display_puts(",");
  long2hex(size,s);
  display_puts(s);
  display_puts("\n");

  r=syscall_shm_create(SHRMEMNAME,4);
  display_puts("shm create=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");

  r=syscall_shm_getsize(100,(int)&size);
  display_puts("shm getsize=");
  long2hex(r,s);
  display_puts(s);
  display_puts(",");
  long2hex(size,s);
  display_puts(s);
  display_puts("\n");
*/

  r=syscall_shm_map(SHRMEMNAME,(unsigned int)SHRMEMADDR);
  display_puts(" shm map=");
  long2hex(r,s);
  display_puts(s);
//  display_puts("\n");

  memcpy((void*)SHRMEMADDR,"ABCD",4);
  syscall_wait(1000);

  r=syscall_shm_unmap(SHRMEMNAME,(unsigned int)SHRMEMADDR);
  display_puts(" shm unmap=");
  long2hex(r,s);
  display_puts(s);
//  display_puts("\n");
/*
  r=syscall_shm_create(SHRMEMNAME2,4*4096);
  display_puts("shm2 create=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");
*/
  r=syscall_shm_delete(SHRMEMNAME);
  display_puts(" shm2 delete=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");
/*
  r=syscall_shm_create(SHRMEMNAME,4);
  display_puts("shm create=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");
*/
}
void tst_shm2(void)
{
  int r;

  r=shm_create(SHRMEMNAME,4);
  display_puts("shm create=");
  sint2dec(r,s);
  display_puts(s);

  r=shm_map(SHRMEMNAME,(char*)SHRMEMADDR);
  display_puts(" shm map=");
  sint2dec(r,s);
  display_puts(s);

  memcpy((void*)SHRMEMADDR,"ABCD",4);
  syscall_wait(1000);

  r=shm_unmap(SHRMEMNAME);
  display_puts(" shm unmap=");
  sint2dec(r,s);
  display_puts(s);

  r=shm_delete(SHRMEMNAME);
  display_puts(" shm2 delete=");
  sint2dec(r,s);
  display_puts(s);
  display_puts("\n");
}


#define QUENAME  1
#define QUENAME2 2

void tst_que(void)
{
  int r;
  int q;
  unsigned char c;
  struct msg_head msg;

  q = syscall_que_create(QUENAME);
  if(q<0) {
    display_puts("que create=");
    long2hex(-q,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  for(;;) {
    msg.size=msg_sizeof(msg);
    r = syscall_que_get(q, &msg);
    if(r<0) {
      display_puts("que get=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      break;
    }
    c=msg.arg;
    display_putc(c);
    if(c=='\n')
      break;
  }
  r = syscall_que_delete(q);
  if(r<0) {
    display_puts("que delete=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }

}

void tst_key(void)
{
  int r;
/*
  r=keyboard_init();
  if(r<0) {
    display_puts("kbd init=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
*/
  for(;;)
  {
    r=keyboard_getcode();
    if(r<0) {
      display_puts("kbd getcode=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return;
    }
    display_putc(r);

/*    display_puts("key=");
    byte2hex(r,s);
    display_puts(s);
*/
/*    s[0]=r;
    s[1]='2';
    s[2]=0;
    display_puts(s);
*/
/*    int2dec(r,s);
    display_puts(s);
    display_puts("2");
*/
  }

}

char datasegmentaddr[10];

void tst_data(void)
{
  display_puts("p=");
  long2hex((long)datasegmentaddr,s);
  display_puts(s);
  display_puts("\n");
}

void tst_pagefault(void)
{
  //int *c=(int *)(0xA1000000);
  int *c=(int *)(0x71000000);

//  display_puts((char*)c);

  //*c=0x12345678;

  display_puts("****");

  long2hex(*c,s);
  display_puts(s);

  display_puts("****");

  c=(int *)(0x90000000);

//  *c=0x87654321;
  display_puts((void*)c);

  display_puts("****");

  long2hex(*c,s);
  display_puts(s);

  display_puts("****");


}

void tst_pgm(void)
{
  int taskid,r;
  r=keyboard_init();
  if(r<0) {
    display_puts("keyinit error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }

  taskid=r=syscall_pgm_load("tst2.out", 0);
  if(r<0) {
    display_puts("load error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  r=syscall_pgm_start(taskid,0);
  if(r<0) {
    display_puts("start error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  display_puts("program start by syscall tst2=");
  int2dec(taskid,s);
  display_puts(s);
  display_puts("\n");
  for(;;)
  {
    r=keyboard_getcode();
    if(r<0) {
      display_puts("kbd getcode=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return;
    }
    if(r=='\n')
      break;
  }
  display_puts("trying delete\n");
  r=syscall_pgm_delete(taskid);
  if(r<0) {
    display_puts("delete error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  display_puts("program delete by syscall tst2\n");

}

void tst_heap(void)
{
  char *m1,*m2,*m3;
  long n;

  n=memory_get_heapfree(0);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

  memory_dumpfree(0);
  m1=malloc(10);
  m2=malloc(15);
  m3=malloc(20);
  memory_dumpfree(0);
  n=memory_get_heapfree(0);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

  mfree(m1);
  memory_dumpfree(0);
  mfree(m2);
  memory_dumpfree(0);
  mfree(m3);
  memory_dumpfree(0);

  n=memory_get_heapfree(0);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

}

void tst_heap2(void)
{
  char *m1,*m2,*m3;
  long n;
  int r;

  r=shm_create(SHRMEMNAME,8*1024);
  display_puts("shm create=");
  sint2dec(r,s);
  display_puts(s);

  r=shm_map(SHRMEMNAME,SHRMEMHEAP);
  display_puts(" shm map=");
  sint2dec(r,s);
  display_puts(s);
  display_puts("\n");

  memory_init(SHRMEMHEAP,8*1024);

  n=memory_get_heapfree(SHRMEMHEAP);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

  memory_dumpfree(SHRMEMHEAP);
  m1=memory_alloc(10,SHRMEMHEAP);
  m2=memory_alloc(15,SHRMEMHEAP);
  m3=memory_alloc(20,SHRMEMHEAP);
  memory_dumpfree(SHRMEMHEAP);
  n=memory_get_heapfree(SHRMEMHEAP);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

  memory_free(m1,SHRMEMHEAP);
  memory_dumpfree(SHRMEMHEAP);
  memory_free(m2,SHRMEMHEAP);
  memory_dumpfree(SHRMEMHEAP);
  memory_free(m3,SHRMEMHEAP);
  memory_dumpfree(SHRMEMHEAP);

  n=memory_get_heapfree(SHRMEMHEAP);
  display_puts("user heap free=");
  int2dec(n,s);
  display_puts(s);
  display_puts("\n");

  r=shm_unmap(SHRMEMNAME);
  display_puts(" shm unmap=");
  sint2dec(r,s);
  display_puts(s);

  r=shm_delete(SHRMEMNAME);
  display_puts(" shm2 delete=");
  sint2dec(r,s);
  display_puts(s);
  display_puts("\n");
}

void tst_mutex(void)
{
  int r,r1;
  char *ctl;
  int *mutex;
  char *m1;

  r1=shm_create(SHRMEMNAME,8*1024);
  display_puts("shm create=");
  sint2dec(r1,s);
  display_puts(s);

  r=shm_map(SHRMEMNAME,SHRMEMHEAP);
  display_puts(" shm map=");
  sint2dec(r,s);
  display_puts(s);
  display_puts("\n");

  mutex = SHRMEMHEAP;
  ctl = (void*)((unsigned long)SHRMEMHEAP+sizeof(int));
  if(r1==0) {
    *mutex=0;
    syscall_mtx_lock(mutex);
    memory_init(ctl,8*1024-sizeof(int));
    display_puts(" init heap\n");
    syscall_mtx_unlock(mutex);
  }

  syscall_mtx_lock(mutex);
  display_puts("lock tst\n");
  m1 = memory_alloc(10,ctl);
  display_puts("alloc tst\n");
  display_puts("sleeping tst\n");
  syscall_wait(1000);
  syscall_mtx_unlock(mutex);
  display_puts("unlock tst\n");

  memcpy(m1,"abcd",5);
  display_puts("memcpy tst\n");
  syscall_wait(1000);
  display_puts("sleeping tst\n");
  display_puts("dump tst=");
  display_puts(m1);
  display_puts("\n");

}

void tst_dir(void)
{
  unsigned long dp;
  int r;
  char dirent[12];

  r = syscall_file_opendir(&dp,"");
  if(r<0) {
    display_puts("opendir error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  for(;;) {
    r = syscall_file_readdir(&dp,dirent);
//for(;;);
    if(r==ERRNO_OVER)
      break;
    if(r<0) {
      display_puts("readdir error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return;
    }
    display_puts("dir=");
    display_puts(dirent);
    display_puts("\n");
  }
  r = syscall_file_closedir(&dp);
  if(r<0) {
    display_puts("closedir error=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }

}

void tst_dsp(void)
{
  int r;
/*
  r=display_init();
  if(r<0) {
    display_puts("dsp init=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
*/
/*
  r=display_putc('A');
  if(r<0) {
    display_puts("dsp putc=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
*/
  r=display_puts("WXYZ\b\b\bAB\tTAB\vVER\rCR\fH\nO\nM\nE\n");
  if(r<0) {
    display_puts("dsp puts=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }

  display_puts("BS=");
  int2dec('\b',s);
  display_puts(s);
  display_puts("\n");
}

int tst_alarm(void)
{
  int i,rc;
  int alarm;
  struct msg_head selected_msg;

  for(i=0;i<10;i++) {
    display_puts("*");
    syscall_wait(50);
  }
  display_puts("\n");

  for(i=0;i<10;i++) {
    display_puts("*");
    alarm = syscall_alarm_set(50,environment_getqueid(),0x00010000);
    selected_msg.size=sizeof(selected_msg);
    rc=message_poll(MESSAGE_MODE_WAIT, 0, 0, &selected_msg);
    rc=message_receive(MESSAGE_MODE_TRY, 0, 0, &selected_msg);
  }
  display_puts("\n");


  return 0;
}

int tst_args(int argc, char *argv[])
{
  int i;
  char *userargs=(char*)CFG_MEM_USERARGUMENT;

  for(i=0;i<CFG_MEM_USERARGUMENTSZ;i++) {
    if(userargs[i]==0)
      display_putc('.');
    else
      display_putc(userargs[i]);
  }
  display_puts("\n");


  for(i=0;i<argc;i++){
    display_puts("argv[");
    int2dec(i,s);
    display_puts(s);
    display_puts("]=[");
    display_puts(argv[i]);
    display_puts("]\n");
  }
  return 0;
}

int tst_mouse(void)
{
  int button,dx,dy;
  int r;

  r=mouse_init();
  if(r<0)
    return 255;

  for(;;) {
    r=mouse_getcode(&button, &dx, &dy);
    if(r<0) {
      display_puts("dsp puts=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      return -1;
    }
/*
    display_puts("[");
    byte2hex(button,s);
    display_puts(s);
    display_puts(",");
    sint2dec(dx,s);
    display_puts(s);
    display_puts(",");
    sint2dec(dy,s);
    display_puts(s);
    display_puts("]");
*/
    display_puts("[");
    byte2hex(button,s);
    display_puts(s);
    display_puts(",");
    byte2hex(dx,s);
    display_puts(s);
    display_puts(",");
    byte2hex(dy,s);
    display_puts(s);
    display_puts("]");


    if(button & 0x1)
      display_puts("L");
    else
      display_puts(".");
    if(button & 0x2)
      display_puts("R");
    else
      display_puts(".");

    display_puts("\n");

  }
}

int tst_usermsg(void)
{
  struct msg_list *head;
  struct msg_head msg;
  int r;

  head=message_create_userqueue();

  if(head==NULL) {
    display_puts("error: create userqueue\n");
    return -1;
  }


  msg.size=sizeof(struct msg_head);
  msg.service=1;
  msg.command=2;
  msg.arg=3;
  r=message_put_userqueue(head,&msg);
  if(r<0) {
    display_puts("error: put userqueue\n");
    return -1;
  }

  msg.size=sizeof(struct msg_head);
  msg.service=4;
  msg.command=5;
  msg.arg=6;
  r=message_put_userqueue(head,&msg);
  if(r<0) {
    display_puts("error: put userqueue\n");
    return -1;
  }
  msg.size=sizeof(struct msg_head);
  r=message_get_userqueue(head,&msg);
  if(r<0) {
    display_puts("error: get userqueue\n");
    return -1;
  }
  display_puts("msg=");
  int2dec(msg.service,s);
  display_puts(s);
  display_puts(".");
  int2dec(msg.command,s);
  display_puts(s);
  display_puts(".");
  int2dec(msg.arg,s);
  display_puts(s);
  display_puts("\n");
  msg.size=sizeof(struct msg_head);

  r=message_get_userqueue(head,&msg);
  if(r<0) {
    display_puts("error: get userqueue\n");
    return -1;
  }
  display_puts("msg=");
  int2dec(msg.service,s);
  display_puts(s);
  display_puts(".");
  int2dec(msg.command,s);
  display_puts(s);
  display_puts(".");
  int2dec(msg.arg,s);
  display_puts(s);
  display_puts("\n");

  r=message_get_userqueue(head,&msg);
  if(r<0) {
    display_puts("empty userqueue\n");
    return -1;
  }

  return 0;
}

#define BKT_QNM_BUCKET     MSGQUENAMES_WINMGR
#define BKT_SRV_BUCKET     MSGQUENAMES_WINMGR

int tst_bucket()
{
  int fd,cli;
  char s[16];
  int rc;
  char *buffer;
  int j;
  fd_set fdset;
  int fdsize=0;

  rc=fd=bucket_open();
  if(rc<0) {
    display_puts("open error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }

  rc=bucket_bind(fd, BKT_QNM_BUCKET, BKT_SRV_BUCKET);
  if(rc<0) {
    display_puts("bind error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }

  
  fdsize=max(fdsize,fd);
  FD_ZERO(&fdset);
  FD_SET(fd,&fdset);
  display_puts("wait accepting in select...");
  rc=bucket_select(fdsize+1,&fdset, 10000);
  if(rc<0) {
    display_puts("select error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }
  if(rc==0) {
    display_puts("timeout waiting be connected\n");
    bucket_close(fd);
    return 0;
  }

  display_puts("selected type=");
  int2dec(rc,s);
  display_puts(s);
  display_puts("\n");

  rc=cli=bucket_accept(fd);
  if(rc<0) {
    display_puts("accept error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }
  display_puts("\n");


  buffer=malloc(4096);
  j=0;

    display_puts("receiving(server)...");
    fdsize=max(fdsize,fd);
    FD_ZERO(&fdset);
    FD_SET(cli,&fdset);
    rc=bucket_select(fdsize+1,&fdset, 1000);
    if(rc<0) {
      display_puts("select error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 1;
    }
    display_puts("\n");

  if(rc==0) {
    display_puts("timeout waiting receive from client\n");
    bucket_shutdown(cli);
    bucket_close(cli);
    bucket_close(fd);
    return 0;
  }

  while(j<5) {
    rc=bucket_recv(cli, buffer, 2);
    if(rc<0) {
      display_puts("recv error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 1;
    }
    display_puts("received(server) bucket:");
    int2dec(rc,s);
    display_puts(s);
    display_puts("byte.(");
    buffer[rc]=0;
    display_puts(buffer);
    display_puts(")\n");

    if(rc==0)
      break;

//syscall_wait(100);
    j++;
  }

  memcpy(buffer,"HELLO",6);
  rc=bucket_send(cli, buffer, 6);
  display_puts("send bucket:");
  int2dec(rc,s);
  display_puts(s);
  display_puts("byte.(");
  display_puts(buffer);
  display_puts(")\n");

  rc=bucket_shutdown(cli);

  rc=bucket_close(cli);
  if(rc<0) {
    display_puts("shutdown error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }

  display_puts("done(server)\n");

  return 0;
}

double tst_fpu(double x,int y)
{
  int z;

  z = (int)x;

  display_puts("9.0 ");
  if(z > 9.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("8.0 ");
  if(z > 8.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("7.0 ");
  if(z > 7.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("4.0 ");
  if(z > 3.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("3.0 ");
  if(z > 3.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("2.0 ");
  if(z > 2.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("1.0 ");
  if(z > 1.0)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.2 ");
  if(z > 0.2)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.1 ");
  if(z > 0.1)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.02 ");
  if(z > 0.02)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.01 ");
  if(z > 0.01)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.002 ");
  if(z > 0.002)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.001 ");
  if(z > 0.001)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.0002 ");
  if(z > 0.0002)
    display_puts("High\n");
  else
    display_puts("Low\n");

  display_puts("0.0001 ");
  if(z > 0.0001)
    display_puts("High\n");
  else
    display_puts("Low\n");

  return z;
}

int tst_int(void)
{
  int x;
  char s[32];

  x=-5;
  sint2dec(x,s);
  display_puts(s);
  display_puts("->");
  sint2dec((x&1),s);
  display_puts(s);
  display_puts("\n");
  x = x/2;
  sint2dec(x,s);
  display_puts(s);
  display_puts("->");
  sint2dec((x&1),s);
  display_puts(s);
  display_puts("\n");
  x = x/2;
  sint2dec(x,s);
  display_puts(s);
  display_puts("->");
  sint2dec((x&1),s);
  display_puts(s);
  display_puts("\n");
  x = x/2;
  sint2dec(x,s);
  display_puts(s);
  display_puts("->");
  sint2dec((x&1),s);
  display_puts(s);
  display_puts("\n");

  return 0;
}

int tst_printdouble(double val, int width)
{
	int minus=0,is_exp=0;
	int exp_digit,dot_pos;
	int digit;
	double top_base,exp_f;
	int i;

	if(val<0) {
		minus=1;
		val = fabs(val);
	}

	exp_f = log10(val);
	if(exp_f >= 0.0) {
		dot_pos = exp_digit = (int)exp_f;
		display_puts("exp=plus\n");
	}
	else {
		dot_pos = exp_digit = ((int)exp_f)-1;
		display_puts("exp=minus\n");
	}

	sint2dec(exp_digit,s);
	display_puts("val_width=");
	display_puts(s);
	display_putc('\n');

	if( exp_digit >= 0 ) {
		if( exp_digit >= width ) {
			is_exp = 1;
			dot_pos = 0;
		}
	}
	else {
		if(-exp_digit <= width) {
			exp_digit = -1;
			dot_pos = -1;
		}
		else {
			is_exp = 1;
			dot_pos = 0;
		}
	}
	top_base = bpow(10.0,exp_digit);

	if(minus)
		display_putc('-');
	dot_pos++;
	for(i=0; i<width; i++) {
		if(dot_pos==0)
			display_putc('.');
		digit = (int)(val / top_base);
		display_putc('0'+digit);
		val = val - (digit*top_base);
		top_base = top_base / 10;
		dot_pos--;
	}
	if(is_exp) {
		display_putc('e');
		if(exp_digit>0)
			display_putc('+');
		sint2dec(exp_digit,s);
		display_puts(s);
	}

	display_putc('\n');
	return 0;
}
int tst_printf(void)
{
  double x=0.0;
  char s[16];

  //print_format("double=%f\n",x);
  dbl2dec(x,s,9);
  display_puts("double=");
  display_puts(s);
  display_puts("\n");

  return 0;
}

float tst_float(float a, float b)
{
  float c;

  c = a*b;

  return c;
}
/*
int tst_window(void)
{
  int r;
  int winid;
  int win_x,win_y,win_w,win_h,drv_w,drv_h;
  int event,ev,mou_x,mou_y,button;
  int color;
  int fgcolor,bgcolor;
//  int winx,winy,winw,winh;
//  int orgx=0,orgy=0;
  int winact=0;
  int gc=0;

  window_init();

  drv_w=640;
  drv_h=480;

  win_x=250;
  win_y=130;
  win_w=300;
  win_h=150;

//  win_x = drv_w/2;
//  win_y = drv_h/2;
  color=1;
  winid =window_create(0, 0, win_w, win_h,"tst");
  if(winid<0)
    return winid;

  window_move(winid, win_x, win_y);
  window_raise(winid);
//  draw_window(winid,win_w,win_h,"Title");

//  keyboard_getcode();

  gc =window_gc_create();

  fgcolor=window_get_color(winid,WIN_COLOR_BLACK);
  bgcolor=window_get_color(winid,WIN_COLOR_WHITE);

//  window_add_event(winid,WIN_EVT_MOU_CMD_MOVEWIN,0,0,win_w,20);


  for(;;) {
    char s[16];

    r=window_get_event(winid, &event, &mou_x, &mou_y, &button);
    if(r<0)
      break;
    ev=event&0xff;

    window_set_color(gc,bgcolor);
    window_draw_boxfill(winid,gc,1,20,200,35);
    window_set_color(gc,fgcolor);

    sint2dec(mou_x,s);
    window_draw_text(winid,gc,1,20,s);
    sint2dec(mou_y,s);
    window_draw_text(winid,gc,50,20,s);
    byte2hex(event,s);
    window_draw_text(winid,gc,100,20,s);

    window_set_color(gc,color);
    if(mou_x>1 && mou_x<win_w-2 && mou_y>20 && mou_y<win_h-2)
    {
      window_draw_pixel(winid,gc,mou_x,mou_y);
    }

    switch(ev)
    {
      case WIN_EVT_MOU_LEFT_DOWN:
        color++;
        if(color>15) {
          color=1;
        }
        window_set_color(gc,color);
        window_raise(winid);
        break;

      case WIN_EVT_MOU_LEFT_UP:
        winact = 0;
        break;

      case WIN_EVT_MOU_RIGHT_DOWN:
        break;

    }

  }

  window_gc_delete(gc);

  return 0;
}
*/

int start(int argc, char *argv[])
{

  display_puts("hello tst\n");
//  tst_que();
//  tst_key();
//  tst_data();
//  tst_pgm();
  tst_pagefault();
//  tst_heap();
//  tst_heap2();
//  tst_dir();
//  tst_dsp();
//  tst_alarm();
//  tst_args(argc,argv);
//  tst_mouse();
//  tst_usermsg();
//  tst_window();
//  display_puts("end tst\n");
//  tst_shm();
//  tst_shm2();
//  tst_mutex();
//  tst_bucket();
//tst_fpu(2.9, 2);
//tst_int(10.9);
//tst_printdouble(100.0 , 8);
//tst_printf();
//tst_float(2.9, 3.9);

  return 456;
}

