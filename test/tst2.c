#include "cpu.h"
#include "syscall.h"
#include "string.h"
#include "keyboard.h"
#include "display.h"
#include "window.h"

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
#define SHRMEMADDR 0x6000000

void shm_tst(void)
{
  int r;

  syscall_wait(10);

  r=syscall_shm_map(SHRMEMNAME,SHRMEMADDR);
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

  r=syscall_shm_unmap(SHRMEMNAME,SHRMEMADDR);
  display_puts(" shm unmap=");
  long2hex(r,s);
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

int start()
{

  display_puts("hello tst2\n");
//  tst_console();
//  tst_que();
//  tst_key();
  tst_window();

  return 456;
}

