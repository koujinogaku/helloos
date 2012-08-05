#include "math.h"

double tan(double angle)
{
  double r;

  asm volatile ("fptan" : "=t"(r) : "0"(angle));
  return r;
}
