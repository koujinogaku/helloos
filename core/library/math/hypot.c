#include "math.h"

double hypot(double x, double y)
{
  x = fabs(x);
  if(y > x) {
    double tmp;

    tmp = x;
    x = y;
    y = tmp;
  }

  return x * sqrt(1 + (y / x) * (y / x));
}
