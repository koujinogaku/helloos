#ifndef BUCKET_H
#define BUCKET_H
#include "kmessage.h"

union bucket_msg {
  struct msg_head h;
  struct bucket_msg_n {
    struct msg_head h;
    void *block;
    unsigned int size;
  } n;
};

struct bucket_dsc {
  int src_qid;
  int dst_qid;
  int dst_svc;
  int dst_cmd;
  union bucket_msg last_msg;
  int pos;
  int have_block;
};

typedef struct bucket_dsc BUCKET;

int bucket_open(BUCKET **dsc);
int bucket_bind(BUCKET *dsc, int src_qid);
int bucket_connect(BUCKET *dsc, int dst_qid,int service,int command);
int bucket_send(BUCKET *dsc, void *buffer, int size);
int bucket_setmsg(BUCKET *dsc, void *msg_v);
int bucket_recv(BUCKET *dsc, void *buffer, int size);

#endif
