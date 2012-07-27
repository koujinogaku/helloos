#ifndef MOUSE_H
#define MOUSE_H
#include "syscall.h"
#include "kmessage.h"

#define MOU_QNM_MOUSE		MSGQUENAMES_MOUSE
#define MOU_SRV_MOUSE		MOU_QNM_MOUSE
#define MOU_CMD_GETCODE		0x0001

#define MOU_BTN_LEFT  	0x1
#define MOU_BTN_RIGHT  	0x2

union mou_msg {
  struct mou_req_s {
    struct msg_head h;
    unsigned short queid;
  } req;
  struct mou_res_s {
    struct msg_head h;
    char dx;
    char dy;
  } res;
};

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

int mouse_init(void);
int mouse_getcode(int *button, int *dx, int *dy);
int mouse_decode_code(void *msg_v, int *button, int *dx, int *dy);
int mouse_request_code(int mode);
int mouse_poll(void);
int mouse_set_wait(int mode);

#endif
