#include "display.h"

int start(int argc, char *argv[])
{
  int i;

  display_setpos(0);

  for(i=0;i<120;i++)
    //            01234567890123456789
    display_puts("                    ");

  display_setpos(0);
  return 0;
}
