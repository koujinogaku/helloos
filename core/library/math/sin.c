#include "math.h"

double sin(double angle)
{
  double r;

  asm volatile ("fsin" : "=t"(r) : "0"(angle));
  return r;
}
