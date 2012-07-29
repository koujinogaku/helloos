#include "syscall.h"
#include "environm.h"
#include "stdlib.h"

void exit(int r)
{
  environment_free();
  syscall_exit(r);
  for(;;) ;
}
