#ifndef KEYBOARD_H
#define KEYBOARD_H
#include "syscall.h"
#include "kmessage.h"
#include "msgquenames.h"

#define KBD_QNM_KEYBOARD   MSGQUENAMES_KEYBOARD
#define KBD_SRV_KEYBOARD   KBD_QNM_KEYBOARD
#define KBD_CMD_GETCODE    0x0001

union kbd_msg {
  struct kbd_req_s {
    struct msg_head h;
    unsigned short queid;
  } req;
  struct kbd_res_s {
    struct msg_head h;
    int key;
  } res;
};

enum {
  KBS  = 0x08,
  KTAB = 0x09,
  KRET = 0x0a,
  KESC = 0x1b,
  KL   = 0x1c,
  KD   = 0x1d,
  KR   = 0x1e,
  KU   = 0x1f,
  KDEL = 0x7f,

  KF1  = 0x101,
  KF2  = 0x102,
  KF3  = 0x103,
  KF4  = 0x104,
  KF5  = 0x105,
  KF6  = 0x106,
  KF7  = 0x107,
  KF8  = 0x108,
  KF9  = 0x109,
  KF10 = 0x10a,
  KF11 = 0x10b,
  KF12 = 0x10c,

  KPSC = 0x110,
  KWIN = 0x111,
  KBRK = 0x112,
  KSRQ = 0x113,
  KPGU = 0x114,
  KPGD = 0x115,
  KINS = 0x116,
  KHOM = 0x117,
  KEND = 0x118,

  KSCR = 0x120,
  KNUM = 0x121,
  KCAP = 0x122,
  KCTL = 0x123,
  KSFT = 0x124,
  KALT = 0x125,
};

int keyboard_init(void);
int keyboard_getcode(void);
int keyboard_request_key(void);
int keyboard_decode_key(void *msg_v);
int keyboard_poll(void);
int keyboard_setmsg(void *msg_v);

#endif
