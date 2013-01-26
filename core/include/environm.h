#ifndef ENVIRONM_H
#define ENVIRONM_H

/* environm.c */

int environment_init(void);
int environment_free(void);
int environment_getqueid(void);
int environment_getargc(void);
char **environment_getargv(void);
int environment_exec(char *filename, void *session);
int environment_allocimage(char *progamname, unsigned long size);
int environment_loadimage(int taskid, void *image, unsigned long size);
int environment_execimage(int taskid, void *session);
int environment_get_session_size(void);
void *environment_copy_session(void);
void environment_make_args(void *process_arg_v, int argc, char *argv[]);
void environment_make_display(void *process_arg_v, int display_queid);
void environment_make_keyboard(void *process_arg_v, int keyboard_queid);
int environment_get_display(void *process_arg_v);
int environment_get_keyboard(void *process_arg_v);
void environment_exit(int exitcode);
int environment_kill(int taskid);
int environment_wait(int *exitcode, int tryflg, void *userargs, int usersize);

#endif
