#ifndef QUEUE_H
#define QUEUE_H
#include "kmessage.h"
#include "kmem.h"

int queue_init(void);
int queue_create(unsigned int quename);
int queue_setname(int queid, unsigned int quename);
int queue_destroy(int queid);
int queue_put(int queid, struct msg_head *msg);
int queue_tryget(int queid, struct msg_head* msg);
int queue_get(int queid, struct msg_head* msg);
int queue_lookup(unsigned int quename);
int queue_trypeek_nextsize(int queid);
int queue_peek_nextsize(int queid);
void queue_make_msg(struct msg_head *msg, int size, int srv, int cmd, int arg, void *data);
int queue_list(int start, int count, struct kmem_queue *qlist);

#endif
