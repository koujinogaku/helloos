/*
** startappl.c --- start entry
*/
#if 0
asm(
".text				\n"
"__start:			\n"
"	call	start		\n"
"       movl    %eax, %ebx      \n"
"       movl    $0x01,%eax      \n" /*SYSCALL_FN_EXIT=1*/
"	int	$0x40		\n"
"__start_endloop:		\n"
"	jmp	__start_endloop	\n"
);
#endif

#include "syscall.h"
#include "environm.h"

int start(int argc, char *argv[]);
void __start(void)
{
  int r;

//syscall_putc('*');
  if(environment_init()<0) {
    syscall_exit(255);
  }
  r=start(environment_getargc(),environment_getargv());
  environment_free();
  syscall_exit(r);
  for(;;) ;
}
