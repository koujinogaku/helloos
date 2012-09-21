/*
** startentry.c --- start entry
*/
#pragma GCC push_options
#pragma GCC optimize("O0")

void start(void);
void __start(void)
{
  start();
}
#pragma GCC pop_options
