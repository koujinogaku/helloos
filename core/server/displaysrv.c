#include "config.h"
#include "cpu.h"
#include "syscall.h"
#include "string.h"
#include "display.h"
#include "errno.h"
#include "environm.h"
#include "font_8x16.c"

#define VGA_CRTC_INDEX		0x3D4		/* 0x3B4 */
#define VGA_CRTC_DATA		0x3D5		/* 0x3B5 */
#define VGA_REG_CURLOC_H	0x0e
#define VGA_REG_CURLOC_L	0x0f
#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9


static int display_queid=0;
static int display_alarm_id=0;
static char s[16];
static char g_pallet[3*256];

struct scrn_t {
	char chr;
	char attr;
};

// display_vmode
// VGA_MODE_TEXT80x25         1
// VGA_MODE_GRAPH640x480x16   2
// VGA_MODE_GRAPH320x200x256  3
static int display_vmode = 0;
static int display_size = 2000;
static int display_size_x = 80;
static int display_size_y = 25;
static int display_graph_size_x = 0;
static int display_graph_size_y = 0;
static int display_pos = 0;
static char display_attr = 0x07;
static struct scrn_t *display_scrn = (void *)CFG_MEM_VGATEXT;
static char display_cursor_flush_enable=0;
static char display_cursor_switch=0;
#define	DISPLAY_CURSOR_ARARM_TIME  (50)


int display_initsrv(void)
{
  int r;

  display_queid = environment_getqueid();
  if(display_queid==0) {
    syscall_puts("display_init que get error=");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }
  r = syscall_que_setname(display_queid, DISPLAY_QNM_DISPLAY);
  if(r<0) {
    syscall_puts("display_init que create error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }
/*
syscall_puts("dsp_queid=");
int2dec(display_queid,s);
syscall_puts(s);
syscall_puts("\n");
*/
  return 0;
}

void
display_set_cursorpos( int pos )
{
  if(display_vmode==1) {
    cpu_out8( VGA_CRTC_INDEX, VGA_REG_CURLOC_H );
    cpu_out8( VGA_CRTC_DATA,  (pos >> 8) & 0xff );
    cpu_out8( VGA_CRTC_INDEX, VGA_REG_CURLOC_L );
    cpu_out8( VGA_CRTC_DATA,  pos & 0xff );
  }
}

int display_cursor_draw(int flag, int arg)
{
  int x_pos,y_pos,i;
  char *g_pos;
  char color;

  if(display_vmode==1)
    return 0;
  if(flag==display_cursor_switch)
    return 0;

  display_cursor_switch=flag;
  if(flag) {
    color = display_attr;
  }
  else {
    color = 0;
  }

  x_pos = display_pos%display_size_x;
  y_pos = (display_pos/display_size_x);
  g_pos = (char*)(((unsigned int)display_scrn)+(unsigned int)(((y_pos*16+15)*display_graph_size_x) + x_pos*8));

  for(i=0;i<8;i++) {
    *g_pos=color;
    g_pos++;
  }

  return 0;
}

void display_scroll(void)
{
  if(display_vmode==1) {
    int i,j,e=display_size-display_size_x;
    for(i=0,j=display_size_x;i<e;i++,j++) {
      display_scrn[i].chr=display_scrn[j].chr;
      display_scrn[i].attr=display_scrn[j].attr;
    }
    for(;i<display_size;i++){
      display_scrn[i].chr = ' ';
      display_scrn[i].attr = display_attr;
    }
  }
  else {
    char *g_dst,*g_src,*g_end;
    display_cursor_draw(0,0);

    g_dst = (char*)display_scrn;
    g_src = (char*)(((unsigned int)display_scrn)+(unsigned int)(display_size_x*8*16));
    g_end = (char*)(((unsigned int)display_scrn)+(unsigned int)(display_size*8*16));
    for(;g_src<g_end;) {
      *g_dst++ = *g_src++;
    }
    for(;g_dst<g_end;) {
      *g_dst++ = 0;
    }
  }
}

int display_putc(int chr)
{
  int x,y,x_pos,y_pos;
  char *g_pos;
  unsigned char pattern;

  switch(chr) {
  case '\n':
    display_pos = (display_pos/display_size_x + 1)*display_size_x;
    break;
  case '\b':
    if(display_pos>0) {
      display_pos--;
    }
    break;
  case '\t':
    display_pos = (display_pos+8)&0xfffffff8;
    if(display_pos>=display_size)
      display_pos=display_size-1;
    break;
  case '\r':
    display_pos = display_pos-(display_pos%display_size_x);
    break;
  case '\v':
    display_pos += display_size_x;
    if(display_pos>=display_size)
      display_pos -= display_size_x;
    break;
  case '\f':
    display_pos = 0;
    break;
  case '\a':
    ;
    break;
  default:
    if(display_vmode==1) {
      display_scrn[display_pos].chr = chr;
      display_scrn[display_pos].attr = display_attr;
      ++display_pos;
    }
    else {
      x_pos = display_pos%display_size_x;
      y_pos = display_pos/display_size_x;
      for(y=0;y<16;y++) {
        g_pos = (char*)(((unsigned int)display_scrn)+(unsigned int)((y_pos*display_graph_size_x*16+y*display_graph_size_x) + x_pos*8));
        pattern = font_8x16[((unsigned char)chr)*(int)16 + y];
        for(x=0;x<8;x++) {
          if(pattern & 0x80) {
            *g_pos = display_attr;
          }
          else {
            *g_pos = 0;
          }
          pattern = pattern << 1;
          g_pos++;
        }
      }
      ++display_pos;
    }
  }
/*
  if(display_pos >= display_size)
    display_pos = 0;
  if((display_pos%display_size_x)==0){
    for(i=0;i<display_size_x;i++){
      display_scrn[display_pos+i].chr = ' ';
      display_scrn[display_pos+i].attr = display_attr;
    }
  }
*/
  if(display_pos >= display_size) {
    display_scroll();
    display_pos = display_size - display_size_x;
  }
  display_set_cursorpos(display_pos);
  return 0;
}

int display_puts(char *str)
{
  struct msg_head *msg =(void*)((unsigned int)str-sizeof(struct msg_head));
  int i;
/*
syscall_puts("arg=");
int2dec(msg->arg,s);
syscall_puts(s);
*/
  for(i=0; *str!=0 && i<msg->arg; ++i,++str)
    display_putc(*str);
  return 0;
}

int display_locatepos(int posx, int posy)
{
  if(posx >= display_size_x)
    posx = display_size_x - 1;
  if(posy >= display_size_y)
    posy = display_size_y - 1;
  display_pos = posx + posy * display_size_x;
  return 0;
}

int display_getpos(void)
{
  return display_pos;
}
int display_setpos(int pos)
{
  display_pos = pos;
  return 0;
}

int display_setattr(int attr)
{
  display_attr = attr;
  return 0;
}

int display_response(unsigned short clientq, union display_msg *msg)
{
  int r;

  r=syscall_que_put(clientq, &msg);
  if(r<0) {
    syscall_puts("resp queput error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts(" qid=");
    int2dec(clientq,s);
    syscall_puts(s);
    syscall_puts("\n");
  }
  return 0;
}

int display_cursor_flush(int arg)
{

  display_alarm_id = syscall_alarm_set(DISPLAY_CURSOR_ARARM_TIME, display_queid, arg+1);
  if(display_alarm_id<0) {
    syscall_puts("alarm error=");
    int2dec(-display_alarm_id,s);
    syscall_puts(s);
    syscall_puts("\n");
    return -1;
  }

  return display_cursor_draw(!display_cursor_switch,arg);
}

void display_write_pallet( int start, int end, char *pallet )
{
	int i;

	cpu_lock();
	cpu_out8(VGA_DAC_WRITE_INDEX, start);
	for (i = start; i <= end; i++) {
		cpu_out8(VGA_DAC_DATA, pallet[0] >> 2);
		cpu_out8(VGA_DAC_DATA, pallet[1] >> 2);
		cpu_out8(VGA_DAC_DATA, pallet[2] >> 2);
		pallet += 3;
	}
	cpu_unlock();
}
void display_init_pallet(char *pallet)
{
	int i;
	for(i=0;i<64;i++) {
		pallet[0]=  ((i&0x04)?0xa8:0) | ((i&0x20)?0x54:0) ;
		pallet[1]=  ((i&0x02)?0xa8:0) | ((i&0x10)?0x54:0) ;
		pallet[2]=  ((i&0x01)?0xa8:0) | ((i&0x08)?0x54:0) ;
		pallet += 3;
	}
	for(i=0;i<64;i++) {
		pallet[0]=  ((i&0x04)?0xa8:0) | ((i&0x20)?0x54:0) ;
		pallet[1]=  ((i&0x02)?0xa8:0) | ((i&0x10)?0x54:0) ;
		pallet[2]=  ((i&0x01)?0xa8:0) | ((i&0x08)?0x04:0) ;
		pallet += 3;
	}
	for(i=0;i<64;i++) {
		pallet[0]=  ((i&0x04)?0xa8:0) | ((i&0x20)?0x54:0) ;
		pallet[1]=  ((i&0x02)?0xa8:0) | ((i&0x10)?0x04:0) ;
		pallet[2]=  ((i&0x01)?0xa8:0) | ((i&0x08)?0x54:0) ;
		pallet += 3;
	}
	for(i=0;i<64;i++) {
		pallet[0]=  ((i&0x04)?0xa8:0) | ((i&0x20)?0x04:0) ;
		pallet[1]=  ((i&0x02)?0xa8:0) | ((i&0x10)?0x54:0) ;
		pallet[2]=  ((i&0x01)?0xa8:0) | ((i&0x08)?0x54:0) ;
		pallet += 3;
	}
}

int display_setup(void)
{
  struct bios_info binfo;
  syscall_kernel_get_bios_info((char *)&binfo);
  int pos;

  display_vmode = binfo.vmode;
  if(display_vmode==1) { // TEXT MODE
    display_size_x = binfo.scrnx;
    if(display_size_x <= 0)
      display_size_x = 1;
    display_size_y = binfo.scrny;
    if(display_size_y <= 0)
      display_size_y = 1;
    display_scrn   = (void *)binfo.vram;
    display_size = display_size_x * display_size_y;
    display_pos = 0;
    display_attr = 0x07;
  }
  else {                 // GRAPHICS MODE
    display_graph_size_x = binfo.scrnx;
    if(display_graph_size_x <= 0)
      display_graph_size_x = 1;
    display_graph_size_y = binfo.scrny;
    if(display_graph_size_y <= 0)
      display_graph_size_y = 1;

    display_size_x = display_graph_size_x / 8;
    display_size_y = display_graph_size_y / 16;

    if(display_vmode  < 0x100)  // VGA GRAPHICS
      display_scrn   = (void *)(binfo.vram);  // Physical Address
    else                       // VESA MODE
      display_scrn = (void*)CFG_MEM_VESAWINDOWSTART; // Virtual  Address

    display_size = display_size_x * display_size_y;
    display_pos = 0;
    display_attr = 0x07;
    display_init_pallet(g_pallet);
    display_write_pallet(0,255,g_pallet);
  }
  pos=syscall_getcursor();
/*
      syscall_puts("console pos=");
      int2dec(pos,s);
      syscall_puts(s);
      syscall_puts("\n");
*/
  display_setpos(pos);
  display_set_cursorpos(pos);
  return 0;
}


void display_comand_dispatch(union display_msg *cmd)
{
  short clientq;

  switch(cmd->h.command) {
    case DISPLAY_CMD_PUTC:
      display_putc(cmd->h.arg);
      break;
    case DISPLAY_CMD_PUTS:
      display_puts(cmd->s.s);
/*
syscall_puts("sz=");
int2dec(cmd->s.h.size,s);
syscall_puts(s);
syscall_puts("arg=");
int2dec(cmd->s.h.arg,s);
syscall_puts(s);
syscall_puts("str=");
byte2hex(cmd->s.s[0],&s[0]);
byte2hex(cmd->s.s[1],&s[2]);
syscall_puts(s);
//for(;;) syscall_wait(100);
*/
      break;
    case DISPLAY_CMD_LOCATE:
      display_locatepos(cmd->h.arg,cmd->n.arg2);
      break;
    case DISPLAY_CMD_GETPOS:
      clientq=cmd->h.arg;
      cmd->h.arg=display_getpos();
//      cmd->h.size=msg_size(sizeof(struct msg_head));
      cmd->h.size=sizeof(struct msg_head);
      display_response(clientq,cmd);
      break;
    case DISPLAY_CMD_SETPOS:
      display_setpos(cmd->h.arg);
      break;
    case DISPLAY_CMD_SETATTR:
      display_setattr(cmd->h.arg);
      break;
    default:
      syscall_puts("cmd number error srv=");
      word2hex(cmd->h.service,s);
      syscall_puts(s);
      syscall_puts(" sz=");
      word2hex(cmd->h.size,s);
      syscall_puts(s);
      syscall_puts(" cmd=");
      long2hex(cmd->h.command,s);
      syscall_puts(s);
      syscall_puts(" arg=");
      long2hex(cmd->h.arg,s);
      syscall_puts(s);
      syscall_puts("\n");
      break;
  }

}
/*****************************************************************************
*****************************************************************************/
int start(void)
{
  int r;
  union display_msg cmd;

  r=display_initsrv();
  if(r<0) {
    return 255;
  }

  display_setup();

  if(display_vmode!=1 && display_cursor_flush_enable) { // Graphics mode
    display_alarm_id = syscall_alarm_set(DISPLAY_CURSOR_ARARM_TIME, display_queid, 0);
    if(display_alarm_id<0) {
      syscall_puts("alarm error=");
      int2dec(-display_alarm_id,s);
      syscall_puts(s);
      syscall_puts("\n");
      return 255;
    }
  }

  for(;;)
  {
//    cmd.h.size=msg_size(sizeof(cmd));
    cmd.h.size=sizeof(cmd);
    r=syscall_que_get(display_queid,&cmd);
    if(r<0) {
      syscall_puts("que receive error=");
      int2dec(-r,s);
      syscall_puts(s);
      syscall_puts("\n");
      return 255;
    }
/*
syscall_puts("cmd=");
int2dec(cmd.h.command,s);
syscall_puts(s);
*/
    if(cmd.h.service==MSG_SRV_ALARM) {
      if(cmd.h.command==0) {
        r=display_cursor_flush(cmd.h.arg);
        if(r<0) {
          return 255;
        }
      }
    }
    else if(cmd.h.service==DISPLAY_SRV_DISPLAY) {
      display_cursor_draw(0,0);
      display_comand_dispatch(&cmd);
      display_cursor_draw(1,0);
    }
    else {
      syscall_puts("srv name error=");
      word2hex(cmd.h.service,s);
      syscall_puts(s);
      syscall_puts(" sz=");
      word2hex(cmd.h.size,s);
      syscall_puts(s);
      syscall_puts(" cmd=");
      long2hex(cmd.h.command,s);
      syscall_puts(s);
      syscall_puts(" arg=");
      long2hex(cmd.h.arg,s);
      syscall_puts(s);
      syscall_puts("\n");
    }
  }
}

