#include "math.h"

double fabs(double x)
{
  double r;

  asm volatile ("fabs" : "=t"(r) : "0"(x));
  return r;
}
