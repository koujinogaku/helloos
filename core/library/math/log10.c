#include "math.h"

double log10(double x)
{
  double r;

  asm volatile("fld1; fld %%st(1); fyl2x; fldl2t; fdivrp; fstp %%st(1)" : "=t" (r) : "0" (x) );
  return r;
}
