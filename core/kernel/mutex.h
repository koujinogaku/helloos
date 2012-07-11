#ifndef MUTEX_H
#define MUTEX_H

void mutex_lock(int *mutex);
void mutex_unlock(int *mutex);
int mutex_trylock(int *mutex);

#endif
