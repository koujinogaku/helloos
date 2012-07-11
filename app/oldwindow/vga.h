#ifndef VGA_H
#define VGA_H

#define VGA_MODE_TEXT80x25         1
#define VGA_MODE_GRAPH640x480x16   2
#define VGA_MODE_GRAPH320x200x256  3

void vga_set_videomode(struct wrect_driver_info *drv,int mode, int mapped);

void vga_read_regs(unsigned char *regs);
void vga_dump_regs(unsigned char *regs);

#endif
