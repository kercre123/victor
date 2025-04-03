// math_finite_shim.c
#include <math.h>

double __log10_finite(double x) { return log10(x); }
double __log_finite(double x)   { return log(x); }
double __atan2_finite(double y, double x) { return atan2(y, x); }
double __fmod_finite(double x, double y)  { return fmod(x, y); }
