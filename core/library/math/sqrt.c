#include "math.h"

double sqrt(double x)
{
  double r;

  asm volatile ("fsqrt" : "=t"(r) : "0"(x));
  return r;
}
