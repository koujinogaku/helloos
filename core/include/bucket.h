#ifndef BUCKET_H
#define BUCKET_H

#define BUCKET_SELECT_DATA 1
#define BUCKET_SELECT_MSG  2

int bucket_open(void);
int bucket_bind(int dsc, int que_name, int service);
int bucket_connect(int dsc, int que_name, int service);
int bucket_accept(int dsc);
int bucket_select(void);
void *bucket_selected_msg(void);
int bucket_isset(int dsc);
int bucket_send(int dsc, void *buffer, int size);
int bucket_recv(int dsc, void *buffer, int size);
int bucket_shutdown(int dsc);
int bucket_close(int dsc);

#endif
