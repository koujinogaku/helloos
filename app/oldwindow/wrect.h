#ifndef WRECT_H
#define WRECT_H

#define WRECT_GC_MODE_PSET 0x00
#define WRECT_GC_MODE_AND  0x08
#define WRECT_GC_MODE_OR   0x10
#define WRECT_GC_MODE_XOR  0x18

struct WRECT {
  int x1;
  int y1;
  int x2;
  int y2;
};

struct wrect_driver_info {
  int width;
  int height;
  unsigned char *videobuff;
  unsigned short mapped;
  unsigned short color;
  unsigned short text_bgcolor;
  unsigned short colorcode_white;
  unsigned short colorcode_black;
  unsigned short colorcode_gray;
  unsigned short calcmode;
  struct wrect_driver_func *fn;
};
struct wrect_driver_func {
  void (*draw_pixel)(struct wrect_driver_info *drv, unsigned int x, unsigned int y);
  void (*draw_horzline)(struct wrect_driver_info *drv, unsigned int x1, unsigned int x2, unsigned int y);
  void (*draw_vertline)(struct wrect_driver_info *drv, unsigned int x, unsigned int y1, unsigned int y2);
  void (*draw_bitmap)(struct wrect_driver_info *drv, unsigned int x, unsigned int y, unsigned char *chr, int ysize,struct WRECT *clip);
  void (*draw_char)(struct wrect_driver_info *drv, unsigned int x,unsigned int y,int chr,struct WRECT *clip);
  void (*draw_pixmap)(struct wrect_driver_info *drv, unsigned int x, unsigned int y, unsigned int xsize, unsigned char *pixmap);
  void (*set_colorcode)(struct wrect_driver_info *drv, int color);
  void (*set_textbgcolor)(struct wrect_driver_info *drv, int color);
  void (*set_calcmode)(struct wrect_driver_info *drv, int mode);
};

int wrect_draw_box(struct wrect_driver_info *drv,struct WRECT *rect,int x1,int y1,int x2,int y2);
int wrect_draw_boxfill(struct wrect_driver_info *drv,struct WRECT *rect,int x1,int y1,int x2,int y2);
int wrect_draw_pixmap(struct wrect_driver_info *drv, struct WRECT *rect,
		int x,int y,int w,int h, unsigned char *pixmap);
int wrect_draw_horzline(struct wrect_driver_info *drv,struct WRECT *rect,int x1,int x2,int y);
int wrect_draw_vertline(struct wrect_driver_info *drv,struct WRECT *rect,int x, int y1, int y2);
int wrect_draw_pixel(struct wrect_driver_info *drv,struct WRECT *rect,int x,int y);
int wrect_draw_bitmap(struct wrect_driver_info *drv, struct WRECT *rect,
		int x, int y,unsigned char *map,int ysize);
int wrect_draw_char(struct wrect_driver_info *drv,struct WRECT *rect, int x,int y,int chr);
int wrect_draw_text(struct wrect_driver_info *drv,struct WRECT *rect, int x,int y,char *str);
int wrect_draw_wall(struct wrect_driver_info *drv, struct WRECT *rect, unsigned char *wall);
int wrect_set_color(struct wrect_driver_info *drv,int color);
int wrect_set_bgcolor(struct wrect_driver_info *drv,int color);
int wrect_set_calcmode(struct wrect_driver_info *drv,int mode);
int wrect_init(struct wrect_driver_info *drv,struct WRECT *rect);
int wrect_textmode(struct wrect_driver_info *drv);
int wrect_adjust_pixmap(struct wrect_driver_info *drv,struct wrect_driver_info *drv2);
int wrect_create_pixmap(struct wrect_driver_info *drv,struct WRECT *rect,int win_x, int win_y, int win_w, int win_h);
int wrect_delete_pixmap(struct wrect_driver_info *drv);

#endif
