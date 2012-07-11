#include "syscall.h"
#include "display.h"
#include "string.h"

#include "vga.h"
#include "wrect.h"

#define	VGA_NUM_SEQ_REGS	5
#define	VGA_NUM_CRTC_REGS	25
#define	VGA_NUM_GC_REGS		9
#define	VGA_NUM_AC_REGS		21
#define	VGA_NUM_REGS		(1 + VGA_NUM_SEQ_REGS + VGA_NUM_CRTC_REGS + \
				VGA_NUM_GC_REGS + VGA_NUM_AC_REGS)

void print_uint(unsigned int d);
void print_hex(char c);
void print_regs(unsigned char *regs);

int gr_mode=0;

int start()
{
  struct wrect_driver_info drv;
  unsigned char regs[VGA_NUM_REGS];

  //vga_set_videomode(&drv,VGA_MODE_TEXT80x25,1);

  vga_read_regs(regs);
  vga_dump_regs(regs);

  return 0;
}
