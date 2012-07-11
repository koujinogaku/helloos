#include <rand.h>

static long holdrand;

void srand (unsigned int seed)
{
  holdrand = (long)seed;
}

int rand (void)
{
  return(((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}
