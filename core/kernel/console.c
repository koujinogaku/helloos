#include "config.h"
#include "console.h"
#include "cpu.h"
#include "font_8x16.c"
#include "string.h"
#include "kmem.h"

#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9

struct scrn_t {
	char chr;
	char attr;
};

// display_vmode
// VGA_MODE_TEXT80x25         1
// VGA_MODE_GRAPH640x480x16   2
// VGA_MODE_GRAPH320x200x256  3
static int console_vmode = 0;
static int console_size = 2000;
static int console_size_x = 80;
static int console_size_y = 25;
static int console_graph_size_x = 0;
static int console_graph_size_y = 0;
static int console_pos = 0;
static char console_attr = 0x07;
static struct scrn_t *console_scrn = (void *)CFG_MEM_VGATEXT;
static struct bios_info *binfo=0;

static char g_pallet[3*256];

void console_write_pallet( int start, int end, char *pallet )
{
	int i;

BEGIN_CPULOCK();
	cpu_out8(VGA_DAC_WRITE_INDEX, start);
	for (i = start; i <= end; i++) {
		cpu_out8(VGA_DAC_DATA, (pallet[0] >> 2) & 0x3f);
		cpu_out8(VGA_DAC_DATA, (pallet[1] >> 2) & 0x3f);
		cpu_out8(VGA_DAC_DATA, (pallet[2] >> 2) & 0x3f);
		pallet += 3;
	}
END_CPULOCK();

}
void console_init_pallet(char *pallet)
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

void console_putc(char chr)
{
  int x,y,x_pos,y_pos;
  char *g_pos;
  unsigned char pattern;

  switch(chr) {
  case '\n':
    console_pos = (console_pos/console_size_x + 1)*console_size_x;
    break;
  default:
    if(console_vmode==1) {
      console_scrn[console_pos].chr = chr;
      console_scrn[console_pos].attr = console_attr;
      ++console_pos;
    }
    else {
      x_pos = console_pos%console_size_x;
      y_pos = console_pos/console_size_x;
      for(y=0;y<16;y++) {
        g_pos = (char*)(((unsigned int)console_scrn)+(unsigned int)(((y_pos*16+y)*console_graph_size_x) + x_pos*8));
        pattern = font_8x16[((unsigned char)chr)*(int)16 + y];
        for(x=0;x<8;x++) {
          if(pattern & 0x80) {
            *g_pos = console_attr;
          }
          else {
            *g_pos = 0;
          }
          pattern = pattern << 1;
          g_pos++;
        }
      }
      ++console_pos;
    }
  }
/*
  if(console_pos >= console_size)
    console_pos = 0;
  if((console_pos%console_size_x)==0){
    for(i=0;i<console_size_x;i++){
      console_scrn[console_pos+i].chr = ' ';
      console_scrn[console_pos+i].attr = console_attr;
    }
  }
*/
  if(console_pos >= console_size) {
    console_scroll();
    console_pos = console_size - console_size_x;
  }
}

void console_scroll(void)
{
  if(console_vmode==1) {
    int i,j,e;

    e=console_size-console_size_x;
    for(i=0,j=console_size_x;i<e;i++,j++) {
      console_scrn[i].chr=console_scrn[j].chr;
      console_scrn[i].attr=console_scrn[j].attr;
    }
    for(;i<console_size;i++){
      console_scrn[i].chr = ' ';
      console_scrn[i].attr = console_attr;
    }
  }
  else {
    char *g_dst,*g_src,*g_end;

    g_dst = (char*)console_scrn;
    g_src = (char*)(((unsigned int)console_scrn)+(unsigned int)(console_size_x*8*16));
    g_end = (char*)(((unsigned int)console_scrn)+(unsigned int)(console_size*8*16));
    for(;g_src<g_end;) {
      *g_dst++ = *g_src++;
    }
    for(;g_dst<g_end;) {
      *g_dst++ = 0;
    }
  }
}

void console_puts(char *str)
{
  for(;*str != 0;++str)
    console_putc(*str);
}

void console_locatepos(int posx, int posy)
{
  if(posx >= console_size_x)
    posx = console_size_x - 1;
  if(posy >= console_size_y)
    posy = console_size_y - 1;
  console_pos = posx + posy * console_size_x;
}

int console_getpos(void)
{
  return console_pos;
}
void console_putpos(int pos)
{
  console_pos = pos;
}

void console_set_virtual_addr_mode(void)
{
  if(console_vmode < 0x100)  // VGA GRAPHICS
    console_scrn   = (void *)(binfo->vram);  // Physical Address
  else                       // VESA MODE
    console_scrn = (void*)CFG_MEM_VESAWINDOWSTART;                 // Virtual  Address
}

void console_setup(void *binfov)
{
  unsigned char *c;
  int x,y;
  binfo = binfov;

  console_vmode = binfo->vmode;
  if(console_vmode==1) { // TEXT MODE
    console_size_x = binfo->scrnx;
    if(console_size_x <= 0)
      console_size_x = 1;
    console_size_y = binfo->scrny;
    if(console_size_y <= 0)
      console_size_y = 1;
    console_scrn   = (void *)(binfo->vram);
    console_size = console_size_x * console_size_y;
    console_pos = 0;
    console_attr = 0x0f;
  }
  else {                 // GRAPHICS MODE
    console_graph_size_x = binfo->scrnx;
    if(console_graph_size_x <= 0)
      console_graph_size_x = 1;
    console_graph_size_y = binfo->scrny;
    if(console_graph_size_y <= 0)
      console_graph_size_y = 1;

    console_size_x = console_graph_size_x / 8;
    console_size_y = console_graph_size_y / 16;

    if(console_vmode < 0x100)  // VGA GRAPHICS
      console_scrn   = (void *)(binfo->vram);  // Physical Address
    else                       // VESA MODE
      console_scrn   = (void *)(binfo->vram);  // Physical Address Mode until the Page Address Mode turn on.
    console_size = console_size_x * console_size_y;
    console_pos = 0;
    console_attr = 0x07;

    console_init_pallet(g_pallet);
    console_write_pallet(0,255,g_pallet);
  }

  c = (unsigned char*)binfo->loaderscreen;
  for(y=0;y<24;y++) {
    for(x=0;x<80;x++) {
      if((*c)>' ' && (*c)<='~') {
        if(x<console_size_x && y<console_size_y) {
          console_locatepos(x,y);
          console_putc(*c);
        }
      }
      c++;
    }
  }
  console_locatepos(binfo->cursorx,binfo->cursory);
/*
  {
  char s[16];
  console_puts("x=");
  int2dec(binfo->cursorx,s);
  console_puts(s);
  console_puts(" y=");
  int2dec(binfo->cursory,s);
  console_puts(s);
  console_puts(" back=");
  long2hex(binfo->loaderscreen,s);
  console_puts(s);
  console_puts("\n");
  }
*/
}
