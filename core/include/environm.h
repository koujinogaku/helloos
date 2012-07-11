#ifndef ENVIRONM_H
#define ENVIRONM_H
#include "kmessage.h"

/* environm.c */

int environment_init(void);
int environment_free(void);
int environment_getqueid(void);
int environment_getargc(void);
char **environment_getargv(void);

#endif
