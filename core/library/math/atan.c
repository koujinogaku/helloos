#include "math.h"

double atan(double x)
{
  double r;

  asm volatile("fld1; fpatan" : "=t"(r) : "0" (x) : "st(1)");
  return r;
}
