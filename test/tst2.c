#include "cpu.h"
#include "syscall.h"
#include "string.h"
#include "keyboard.h"
#include "display.h"
#include "shm.h"
#include "memory.h"
#include "bucket.h"
#include "environm.h"
//#include "window.h"

void puts(char *str)
{
  for(;*str != 0;++str)
    display_putc(*str);
}

char s[10];

void tst_console(void)
{
//  int a;
//  int i,j;
//  int size;


//  a = display_putc('H');
//  a = display_putc('E');
//  a = display_putc('L');
//  display_putc(a);
//syscall_tst3(12,34,56);

//  syscall_exit(123);
/*
  for(i=0;i<5;i++) {
    for(j=0;j<i;j++)
      puts(".");
    puts("Hello World\n");
  }
*/

  for(;;) {
      puts("1");
    syscall_wait(1000);
  }

//  cpu_halt();

}

#define SHRMEMNAME 1
#define SHRMEMNAME2 2
#define SHRMEMADDR ((void *)0x6000000)
#define SHRMEMHEAP ((void *)0x7000000)

void tst_shm(void)
{
  int r;

  syscall_wait(10);

  r=syscall_shm_map(SHRMEMNAME,(unsigned int)SHRMEMADDR);
  display_puts("shm map=");
  long2hex(r,s);
  display_puts(s);
//  display_puts("\n");

  syscall_wait(50);
  memcpy(s,(void*)SHRMEMADDR,4);
  s[4]=0;
  display_puts(" shrstr=");
  display_puts(s);
//  display_puts("\n");

  r=syscall_shm_unmap(SHRMEMNAME,(unsigned int)SHRMEMADDR);
  display_puts(" shm unmap=");
  long2hex(r,s);
  display_puts(s);
  display_puts("\n");
}

void tst_shm2(void)
{
  int r;

  syscall_wait(10);


  r=shm_map(SHRMEMNAME,(char*)SHRMEMADDR);
  display_puts("shm map=");
  sint2dec(r,s);
  display_puts(s);
//  display_puts("\n");
  if(r<0)
    return;

  syscall_wait(50);
  memcpy(s,(void*)SHRMEMADDR,4);
  s[4]=0;
  display_puts(" shrstr=");
  display_puts(s);
//  display_puts("\n");

  r=shm_unmap(SHRMEMNAME);
  //r=shm_unmap(0);
  display_puts(" shm unmap=");
  sint2dec(r,s);
  display_puts(s);
  display_puts("\n");
}


#define QUENAME  1
#define QUENAME2 2

void tst_que(void)
{
  int r,q;
  char str[]="ABCD\n";
  struct msg_head msg;
  char *c;

  syscall_wait(10);

  q = syscall_que_lookup(QUENAME);
  if(q<0) {
    display_puts("que lookup=");
    long2hex(-q,s);
    display_puts(s);
    display_puts("\n");
    return;
  }
  for(c=str;;c++) {
    msg.size=msg_sizeof(msg);
    msg.service=0;
    msg.command=0;
    msg.arg=*c;
    r = syscall_que_put(q, &msg);
    if(r<0) {
      display_puts("que put=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      break;
    }
    if(*c=='\n')
      break;
  }
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
  display_puts("lock tst2\n");
  m1 = memory_alloc(10,ctl);
  display_puts("alloc tst2\n");
  syscall_mtx_unlock(mutex);
  display_puts("unlock tst2\n");

  memcpy(m1,"efgh",5);
  display_puts("memcpy tst2\n");
  syscall_wait(1000);
  display_puts("sleeping tst2\n");
  display_puts("dump tst2=");
  display_puts(m1);
  display_puts("\n");

}

void tst_key(void)
{
  int r;

  r=keyboard_init();
  if(r<0) {
    display_puts("kbd init=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return;
  }

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
/*    display_puts("key=");
    byte2hex(r,s);
    display_puts(s);
*/
    s[0]=r;
    s[1]='1';
    s[2]=0;
    display_puts(s);
/*
    int2dec(r,s);
    display_puts(s);
    display_puts("1");
*/
  }

}

#define BKT_QNM_BUCKET     MSGQUENAMES_WINMGR
#define BKT_SRV_BUCKET     MSGQUENAMES_WINMGR
#define BKT_CMD_BLOCK      0x0001

int tst_bucket()
{
  int fd;
  char s[16];
  int rc;
  char *buffer;
  int j;

  rc=fd=bucket_open();
  if(rc<0) {
    display_puts("open error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }

  rc=bucket_connect(fd, BKT_QNM_BUCKET, BKT_SRV_BUCKET);
  if(rc<0) {
    display_puts("connect error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }


  buffer=malloc(4096);
  memcpy(buffer,"ABC",3);
  rc=bucket_send(fd, buffer, 3);
  if(rc<0) {
    display_puts("send error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }
  memcpy(buffer,"DEF",3);
  rc=bucket_send(fd, buffer, 3);
  if(rc<0) {
    display_puts("send error=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return 1;
  }

//  rc=bucket_shutdown(fd);
//  if(rc<0) {
//    display_puts("shutdown error=");
//    int2dec(-rc,s);
//    display_puts(s);
//    display_puts("\n");
//    return 1;
//  }

  j=0;
  while(j<5) {
    display_puts("receiving(client)...");
    rc=bucket_select();
    if(rc<0) {
      display_puts("select error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 1;
    }
    display_puts("\n");

    rc=bucket_recv(fd, buffer, 6);
    if(rc<0) {
      display_puts("recv error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 1;
    }
    display_puts("received(client) bucket:");
    int2dec(rc,s);
    display_puts(s);
    display_puts("byte.(");
    buffer[rc]=0;
    display_puts(buffer);
    display_puts(")\n");

    if(rc==0)
      break;

syscall_wait(100);
    j++;
  }

  display_puts("done(client)\n");

  return 0;
}


/*
int tst_window(void)
{
  int r;
  int winid;
  int win_x,win_y,win_w,win_h,drv_w,drv_h;
  int event,mou_x,mou_y,button;
  int color;
  int fgcolor,bgcolor;
  int gc=0;

  window_init();

  drv_w=640;
  drv_h=480;

  win_x=3;
  win_y=3;
  win_w=300;
  win_h=150;

//  win_x = drv_w/2;
//  win_y = drv_h/2;
  color=1;
  winid =window_create(0, 0, win_w, win_h,"tst2");
  if(winid<0) {
    return winid;
  }
  window_move(winid, win_x, win_y);
//  window_raise(winid);
//  draw_window(winid,win_w,win_h,"Title");

//  keyboard_getcode();

  gc =window_gc_create();

  fgcolor=window_get_color(winid,WIN_COLOR_BLACK);
  bgcolor=window_get_color(winid,WIN_COLOR_WHITE);


  for(;;) {
    char s[16];

    r=window_get_event(winid, &event, &mou_x, &mou_y, &button);
    if(r<0)
      break;

    window_set_color(gc,bgcolor);
    window_draw_boxfill(winid,gc,1,20,50,68);
    window_set_color(gc,fgcolor);

    sint2dec(mou_x,s);
    window_draw_text(winid,gc,1,20,s);
    sint2dec(mou_y,s);
    window_draw_text(winid,gc,1,36,s);
    byte2hex(event,s);
    window_draw_text(winid,gc,1,52,s);

    window_set_color(gc,color);
    window_draw_pixel(winid,gc,mou_x,mou_y);

    if((event&0xff) == WIN_EVT_MOU_LEFT_DOWN) {
      color++;
      if(color>15) {
        color=1;
      }
      window_set_color(gc,color);
      window_raise(winid);
    }
    else if((event&0xff) == WIN_EVT_MOU_RIGHT_DOWN) {
      break;
    }
  }

  window_gc_delete(gc);

  return 0;
}
*/

int start()
{

  display_puts("hello tst2\n");
//  tst_console();
//  tst_que();
//  tst_key();
//  tst_window();
//  tst_shm();
//  tst_shm2();
//  tst_mutex();
  tst_bucket();

  return 789;
}

