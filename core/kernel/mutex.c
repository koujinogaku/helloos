#include "config.h"
#include "cpu.h"
#include "task.h"
#include "mutex.h"

void mutex_lock(int *mutex)
{
  int hold_locked;

  while(1) {
    cpu_hold_lock(hold_locked);
    if(*mutex==0) {
      *mutex=1;
      cpu_hold_unlock(hold_locked);
      return;
    }
    cpu_hold_unlock(hold_locked);
    task_dispatch();
  }
}

int mutex_trylock(int *mutex)
{
  int hold_locked;

  cpu_hold_lock(hold_locked);
  if(*mutex!=0) {
    cpu_hold_unlock(hold_locked);
    return -1;
  }
  *mutex=1;
  cpu_hold_unlock(hold_locked);

  return 0;
}

void mutex_unlock(int *mutex)
{
  *mutex=0;
}

