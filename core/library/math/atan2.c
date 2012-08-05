#include "math.h"

double atan2(double y, double x)
{
  double r;

  asm volatile("fpatan" : "=t" (r) : "0" (x), "u" (y) : "st(1)");
  return r;
}
