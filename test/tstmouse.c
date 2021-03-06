#include "string.h"
#include "display.h"
#include "mouse.h"

static char s[10];

int tst_mouse(void)
{
  int button,dx,dy;
  int r;
  int carry;

  r=mouse_init();
  if(r<0)
    return 255;
  r=mouse_request_code(TRUE);
  if(r<0)
    return 255;

  mouse_set_wait(TRUE);
  for(;;) {
    r=mouse_getcode(&button, &dx, &dy);
    if(r<0) {
      display_puts("dsp puts=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      return -1;
    }
    if(r==0) {
      display_puts("--");
      continue;
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
    carry = button & 0xcc;
    byte2hex(carry,s);
    display_puts(s);

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

    if(button & 0x10)
      display_puts("L");
    else
      display_puts(".");

    if(button & 0x20)
      display_puts("D");
    else
      display_puts(".");

    display_puts("\n");

  }
}
int start(int argc, char *argv[])
{
  tst_mouse();
  return 456;
}
