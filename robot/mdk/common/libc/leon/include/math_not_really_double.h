#ifndef MATH_NOT_REALLY_DOUBLE_H
#define MATH_NOT_REALLY_DOUBLE_H

// Can be used to ease porting of applications that use double-precision arithmetic where they only need single-precision.

static inline double acos( double x ) { return acosf( (float)x ); }
static inline double asin( double x ) { return asinf( (float)x ); }
static inline double exp( double x ) { return expf( (float)x ); }
static inline double exp2( double x ) { return exp2f( (float)x ); }
static inline double exp10( double x ) { return exp10f( (float)x ); }
static inline double frexp( double x, int *p ) { return frexpf( (float)x, p ); }
static inline double ldexp( double x, int y ) { return ldexpf( (float)x, y ); }
static inline double log( double x ) { return logf( (float)x ); }
static inline double log2( double x ) { return log2f( (float)x ); }
static inline double log10( double x ) { return log10f( (float)x ); }
static inline double modf( double x, double *y ) { float _y, _r; _r = modff( (float)x, &_y ); *y = _y; return _r; }
static inline double ceil( double x ) { return ceilf( (float)x ); }
static inline double floor( double x ) { return floorf( (float)x ); }
static inline double cos( double x ) { return cosf( (float)x ); }
static inline double sin( double x ) { return sinf( (float)x ); }
static inline double tan( double x ) { return tanf( (float)x ); }
static inline double infinity() { return infinityf(); }
static inline double nan( const char *p ) { return nanf( p ); }
static inline int finite( double x ) { return finitef( (float)x ); }
static inline int isinf( double x ) { return isinff( (float)x ); }
static inline int isnan( double x ) { return x!=x; }
static inline int ispos( double x ) { return x>0; }
static inline double copysign( double x, double y ) { double t = fabs(x); return( y < 0 )?(-t):t; }
static inline int numtest( double x ) { return numtestf( (float)x ); }
static inline int __fpclassify( double x ) { return __fpclassifyf( (float)x ); }

#endif
