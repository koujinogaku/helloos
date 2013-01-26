#ifndef CONSOLE_H
#define CONSOLE_H


/* console.c */
void console_setup(void *);
void console_putc(char chr);
int  console_puts(char *str);
void console_scroll(void);
void console_locatepos(int posx, int posy);
int console_getpos(void);
void console_putpos(int pos);
void console_set_virtual_addr_mode(void);


#endif
