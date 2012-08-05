#include "math.h"

double bpow(double x, int n)
{
  double y,z;

  if(n == 0)
    return 1.0;

  y = bpow(x, n / 2);
  z = y * y;
  if(n & 1) {
    if(n > 0)
      z *= x;
    else
      z /= x;
  }

  return z;
}
