#ifndef MESSAGE_H
#define MESSAGE_H

#include "kmessage.h"

#define MESSAGE_MODE_WAIT 0
#define MESSAGE_MODE_TRY  1
#ifndef NULL
#define NULL (0)
#endif

struct msg_list {
  struct msg_list *next;
  struct msg_list *prev;
  struct msg_head *msg;
};

int message_init(void);
int message_send(int queid, void *msg);
int message_receive(int tryflg, int srv, int cmd, void *msgret);
int message_poll(int srv, int cmd, void *vmsgret);

struct msg_list *message_create_userqueue(void);
int message_put_userqueue(struct msg_list *head, void *vmsg);
int message_get_userqueue(struct msg_list *head, void *vmsg);

#endif
