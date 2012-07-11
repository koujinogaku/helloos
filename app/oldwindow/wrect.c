#include "string.h"
#include "keyboard.h"
#include "wrect.h"
#include "memory.h"
#include "vga.h"

#define TEST 0

//static struct wrect_driver_info wrect_device_driver_info;


int wrect_draw_box(struct wrect_driver_info *drv, struct WRECT *rect,
		int x1,int y1,int x2,int y2)
{
  int t,tx1,ty1,tx2,ty2;

  if(x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }
  if(y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }


  tx1=x1; ty1=y1; tx2=x2; ty2=y2;

  if(x2 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x1) {
    return -1;
  }
  if(y2 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y1) {
    return -1;
  }


  if(x1 < rect->x1) {
    x1 = rect->x1;
  }
  if(rect->x2 < x2) {
    x2 = rect->x2;
  }
  if(y1 < rect->y1) {
    y1 = rect->y1;
  }
  if(rect->y2 < y2) {
    y2 = rect->y2;
  }


  if(ty1 >= rect->y1) {
    drv->fn->draw_horzline(drv,x1,x2,y1);
  }
  if(rect->y2 >= ty2) {
    drv->fn->draw_horzline(drv,x1,x2,y2);
  }
  if(tx1 >= rect->x1) {
    drv->fn->draw_vertline(drv,x1,y1,y2);
  }
  if(rect->x2 >= tx2) {
    drv->fn->draw_vertline(drv,x2,y1,y2);
  }
  return 0;
}

int wrect_draw_boxfill(struct wrect_driver_info *drv, struct WRECT *rect,
		int x1,int y1,int x2,int y2)
{
  int t;

  if(x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }
  if(y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if(x2 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x1) {
    return -1;
  }
  if(y2 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y1) {
    return -1;
  }

  if(x1 < rect->x1) {
    x1 = rect->x1;
  }
  if(rect->x2 < x2) {
    x2 = rect->x2;
  }
  if(y1 < rect->y1) {
    y1 = rect->y1;
  }
  if(rect->y2 < y2) {
    y2 = rect->y2;
  }

  for(t=y1;t<=y2;t++)
    drv->fn->draw_horzline(drv,x1,x2,t);

  return 0;
}

int wrect_draw_pixmap(struct wrect_driver_info *drv, struct WRECT *rect,
		int x,int y,int w,int h, unsigned char *pixmap)
{
  int t,s;
  int px,py,pw,ph;
  int x1,y1,x2,y2;

  x1=x;
  y1=y;
  x2=x+w-1;
  y2=y+h-1;

  px=0;
  py=0;
  pw=w;
  ph=h;

  if(x2 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x1) {
    return -1;
  }
  if(y2 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y1) {
    return -1;
  }

  if(x1 < rect->x1) {
    px = rect->x1 - x1;
    x1  = rect->x1;
  }
  if(rect->x2 < x2) {
    x2  = rect->x2;
  }
  if(y1 < rect->y1) {
    py = rect->y1 - y1;
    y1  = rect->y1;
  }
  if(rect->y2 < y2) {
    y2  = rect->y2;
  }

  pixmap += px + py*pw;
  s=x2-x1+1;


  for(t=y1;t<=y2;t++) {
    drv->fn->draw_pixmap(drv,x1,t,s,pixmap);
    pixmap += pw;
  }
/*
  drv->fn->set_colorcode(drv,drv->colorcode_white);
  for(t=y1;t<=y2;t++) {
    drv->fn->draw_horzline(drv,x1,x1+w,t);
  }
*/
  return 0;
}


int wrect_draw_horzline(struct wrect_driver_info *drv, struct WRECT *rect,
			int x1,int x2,int y)
{
  int t;

  if(x1 > x2) {
    t = x1;
    x1 = x2;
    x2 = t;
  }

  if(x2 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x1) {
    return -1;
  }
  if(y < rect->y1) {
    return -1;
  }
  if(rect->y2 < y) {
    return -1;
  }

  if(x1 < rect->x1) {
    x1 = rect->x1;
  }
  if(rect->x2 < x2) {
    x2 = rect->x2;
  }

  drv->fn->draw_horzline(drv,x1,x2,y);
  return 0;
}

int wrect_draw_vertline(struct wrect_driver_info *drv, struct WRECT *rect,
			int x, int y1, int y2)
{
  int t;

  if(y1 > y2) {
    t = y1;
    y1 = y2;
    y2 = t;
  }

  if(x < rect->x1) {
    return -1;
  }
  if(rect->x2 < x) {
    return -1;
  }
  if(y2 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y1) {
    return -1;
  }

  if(y1 < rect->y1) {
    y1 = rect->y1;
  }
  if(rect->y2 < y2) {
    y2 = rect->y2;
  }

  drv->fn->draw_vertline(drv,x,y1,y2);
  return 0;
}

int wrect_draw_pixel(struct wrect_driver_info *drv, struct WRECT *rect,
		int x,int y)
{
  if(x < rect->x1) {
    return -1;
  }
  if(rect->x2 < x) {
    return -1;
  }
  if(y < rect->y1) {
    return -1;
  }
  if(rect->y2 < y) {
    return -1;
  }

  drv->fn->draw_pixel(drv,x,y);
  return 0;
}

int wrect_draw_bitmap(struct wrect_driver_info *drv, struct WRECT *rect,
		int x, int y,unsigned char *map,int ysize)
{
  struct WRECT clip;
  int xbyte=1;

  if(x+xbyte*8-1 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x) {
    return -1;
  }
  if(y+ysize-1 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y) {
    return -1;
  }

  if(x < rect->x1) clip.x1 = rect->x1 - x;
  else             clip.x1 = 0;
  if(y < rect->y1) clip.y1 = rect->y1 - y;
  else             clip.y1 = 0;
  if(rect->x2 < x+xbyte*8-1) clip.x2 = rect->x2 - x+1;
  else                       clip.x2 = xbyte*8;
  if(rect->y2 < y+ysize-1)   clip.y2 = rect->y2 - y+1;
  else                       clip.y2 = ysize;
  
  drv->fn->draw_bitmap(drv,x,y,map,ysize,&clip);
  return 0;
}
int wrect_draw_char(struct wrect_driver_info *drv, struct WRECT *rect,
			int x, int y,int chr)
{
  struct WRECT clip;
  if(x+8-1 < rect->x1) {
    return -1;
  }
  if(rect->x2 < x) {
    return -1;
  }
  if(y+16-1 < rect->y1) {
    return -1;
  }
  if(rect->y2 < y) {
    return -1;
  }

  if(x < rect->x1) clip.x1 = rect->x1 - x;
  else             clip.x1 = 0;
  if(y < rect->y1) clip.y1 = rect->y1 - y;
  else             clip.y1 = 0;
  if(rect->x2 < x+8-1) clip.x2 = rect->x2 - x+1;
  else                       clip.x2 = 8;
  if(rect->y2 < y+16-1)      clip.y2 = rect->y2 - y+1;
  else                       clip.y2 = 16;

  drv->fn->draw_char(drv,x,y,chr,&clip);
  return 0;
}

int wrect_draw_text(struct wrect_driver_info *drv, struct WRECT *rect, 
			int x, int y,char *str)
{
  for(;*str!=0;str++) {
    wrect_draw_char(drv,rect, x,y,*str);
    x += 8;
  }
  return 0;
}

int wrect_draw_wall(struct wrect_driver_info *drv, struct WRECT *rect,
		unsigned char *wall)
{
  int x,y;
  int fgcolor=drv->colorcode_black;
  int bgcolor=drv->colorcode_white;

  wrect_set_color(drv,fgcolor);
  wrect_draw_boxfill(drv,rect,rect->x1,rect->y1,rect->x2,rect->y2);
  wrect_set_color(drv,bgcolor);

  for(y=0;y<drv->height;y+=8)
    for(x=0;x< drv->width;x+=8)
      wrect_draw_bitmap(drv,rect,x,y,wall,8);

  return 0;
}

int wrect_set_color(struct wrect_driver_info *drv, int color)
{
  drv->fn->set_colorcode(drv,color);
  return 0;
}
int wrect_set_bgcolor(struct wrect_driver_info *drv, int color)
{
  drv->fn->set_textbgcolor(drv,color);
  return 0;
}
int wrect_set_calcmode(struct wrect_driver_info *drv, int mode)
{
  drv->fn->set_calcmode(drv,mode);
  return 0;
}

int wrect_init(struct wrect_driver_info *drv,struct WRECT *rect)
{
  vga_set_videomode(drv, VGA_MODE_GRAPH640x480x16, 1);
//  vga_set_videomode(drv,VGA_MODE_GRAPH320x200x256, 1);
  rect->x1=0;
  rect->y1=0;
  rect->x2=drv->width-1;
  rect->y2=drv->height-1;
  wrect_set_color(drv,drv->colorcode_black);
  wrect_draw_boxfill(drv,rect,0,0,rect->x2,rect->y2);
  return 0;
}

int wrect_textmode(struct wrect_driver_info *drv)
{
  vga_set_videomode(drv, VGA_MODE_TEXT80x25,1);
  return 0;
}

int wrect_adjust_pixmap(struct wrect_driver_info *drv,struct wrect_driver_info *drv2)
{
  drv->colorcode_white=drv2->colorcode_white;
  drv->colorcode_black=drv2->colorcode_black;
  drv->colorcode_gray =drv2->colorcode_gray;
  return 0;
}

int wrect_create_pixmap(struct wrect_driver_info *drv,struct WRECT *rect,int win_x, int win_y, int win_w, int win_h)
{
  unsigned int vbsize;
  vga_set_videomode(drv,VGA_MODE_GRAPH320x200x256, 0);

  drv->width=win_w;
  drv->height=win_h;
  vbsize=drv->width * drv->height;
  drv->videobuff = malloc(vbsize);
  rect->x1=0;
  rect->y1=0;
  rect->x2=drv->width-1;
  rect->y2=drv->height-1;
  return 0;
}

int wrect_delete_pixmap(struct wrect_driver_info *drv)
{
  mfree(drv->videobuff);
  return 0;
}

#if TEST

void draw_window(struct wrect_driver_info *drv, struct WRECT *rect, 
		int wx, int wy, int ww, int wh, char *title)
{
  int fgcolor=drv->colorcode_black;
  int bgcolor=drv->colorcode_white;
  int grcolor=drv->colorcode_gray;

  int titlelen=strlen(title);
  wrect_set_color(drv,bgcolor);
  wrect_draw_boxfill(drv,rect,wx,wy,wx+ww-1,wy+wh-1);
  wrect_set_color(drv,fgcolor);
  wrect_draw_box(drv,rect,wx,wy,wx+ww-2,wy+wh-2);
  wrect_draw_horzline(drv,rect,wx,wx+ww-2,wy+18);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+4);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+6);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+8);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+10);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+12);
  wrect_draw_horzline(drv,rect,wx+2,wx+ww-2-2,wy+14);
  wrect_set_color(drv,bgcolor);
  wrect_draw_boxfill(drv,rect,wx+7,wy+4,wx+19,wy+14);
  wrect_set_color(drv,fgcolor);
  wrect_draw_box(drv,rect,wx+8,wy+4,wx+18,wy+14);
  wrect_draw_horzline(drv,rect,wx+1,wx+ww-2,wy+19);
  wrect_set_color(drv,grcolor);
  wrect_draw_horzline(drv,rect,wx+1,wx+ww-1,wy+wh-1);
  wrect_draw_vertline(drv,rect,wx+ww-1,wy+1,wy+wh-1);
  wrect_set_color(drv,bgcolor);
  wrect_draw_boxfill(drv,rect,wx+ww/2-(titlelen*8/2)-3,wy+2,wx+ww/2+(titlelen*8/2)+1,wy+17);
  wrect_set_color(drv,fgcolor);
  wrect_set_bgcolor(drv,bgcolor);
  wrect_draw_text(drv,rect,wx+ww/2-(titlelen*8/2),wy+2,title);

}

int draw_wall(struct wrect_driver_info *drv, struct WRECT *rect) {
  int x,y;
  int fgcolor=drv->colorcode_black;
  int bgcolor=drv->colorcode_white;
  static unsigned char wall[] = { 0xef, 0xff, 0xfe, 0xff, 0xbf, 0xff, 0xfb, 0xff };

  wrect_set_color(drv,fgcolor);
  wrect_draw_boxfill(drv,rect,rect->x1,rect->y1,rect->x2,rect->y2);
  wrect_set_color(drv,bgcolor);

  for(y=0;y<drv->height;y+=8)
    for(x=0;x< drv->width;x+=8)
      wrect_draw_bitmap(drv,rect,x,y,wall,8);

  return 0;
}

int start(void)
{
  struct wrect_driver_info wrect_driver;
  struct wrect_driver_info wrect_driver2;
  struct WRECT rect;
  struct WRECT rect2;
  int win_x,win_y,win_w,win_h;

  wrect_init(&wrect_driver2,&rect2);
  draw_wall(&wrect_driver2,&rect2);

  win_x=5;
  win_y=5;
  win_w=200;
  win_h=150;

  wrect_create_pixmap(&wrect_driver,&rect,win_x,win_y,win_w,win_h);
  wrect_adjust_pixmap(&wrect_driver,&wrect_driver2);

  wrect_set_color(&wrect_driver,wrect_driver.colorcode_white);
  wrect_draw_boxfill(&wrect_driver,&rect,0,0,rect.x2,rect.y2);

  draw_window(&wrect_driver,&rect,0,0,win_w,win_h,"title");
//  draw_window(&rect,40,100,300,200,"Hello World");
//  wrect_set_color(0);
//  wrect_draw_text(&rect, 500,0,"hit key");

  wrect_set_color(&wrect_driver2, wrect_driver2.colorcode_white);
  wrect_set_bgcolor(&wrect_driver2, wrect_driver2.colorcode_black);
  wrect_draw_text(&wrect_driver2, &rect2, 0,0,"hit key");

//  memcpy(wrect_driver2.videobuff,wrect_driver.videobuff,vbsize);
  wrect_draw_pixmap(&wrect_driver2, &rect2, 
		win_x,win_y,win_w,win_h, wrect_driver.videobuff);
  keyboard_getcode();

  vga_set_videomode(&wrect_driver, VGA_MODE_TEXT80x25,1);
  return 0;
}
#endif
