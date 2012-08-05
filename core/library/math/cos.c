#include "math.h"

double cos(double angle)
{
  double r;

  asm volatile ("fcos" : "=t"(r) : "0"(angle));
  return r;
}
