#include <stdio.h>
#include "config.h"
#include "desc.h"
#include "message.h"
#include "window.h"

int
main(void)
{
  /*   */
  printf("idt=%d\n",sizeof(struct desc_gate));
  printf("idtx256=%d\n",sizeof(struct desc_gate)*0xff);
  printf("msg=%d\n",msg_size(sizeof(struct win_req_stext)-WIN_STRLEN+5+5));

#if LINUX
  printf("define LINUX\n");
#endif
#if i386
  printf("define i386\n");
#endif
#if i486
  printf("define i486\n");
#endif
#if _POSIX_SOURCE
  printf("define _POSIX_SOURCE\n");
#endif

}
