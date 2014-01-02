#ifndef __MIRCEA_MATH_H_
#define __MIRCEA_MATH_H_

#ifndef __ASSEMBLER__

// Double-precision functions:

double __builtin_fabs( double );
static inline double fabs( double x ) { return __builtin_fabs( x ); }

static inline double sqrt( double x ) { double y; asm( "fsqrtd %1, %0" : "=f"(y) : "f"(x) ); return y; }

// Single-precision functions:

float __builtin_fabsf( float );
static inline float fabsf( float x ) { return __builtin_fabsf( x ); }

static inline float sqrtf( float x ) { float y; asm( "fsqrts %1, %0" : "=f"(y) : "f"(x) ); return y; }

float acosf( float );
float asinf( float );

float expf( float );
float exp2f( float );
float exp10f( float );

float frexpf( float x, int *p );

float ldexpf( float, int );

float logf( float );
float log2f( float );
float log10f( float );

float modff( float, float* );
float ceilf( float );
float floorf( float );

float cosf( float );
float sinf( float );

float tanf( float );

float infinityf();
float nanf( const char* );

int finitef( float );
int isinff( float );
int isnanf( float );
int isposf( float );

float copysignf( float, float );

int numtestf( float );
int __fpclassifyf( float );

extern const float fp32consts[];

#endif

// numtest constants
#define NUM 3
#define NAN 2
#define INF 1 

// __fpclassify constants
#define FP_NAN         0
#define FP_INFINITE    1
#define FP_ZERO        2
#define FP_SUBNORMAL   3
#define FP_NORMAL      4  

// math constants
#define _M_LN2     (fp32consts[16]) // 0.693147180559945309417

#define M_E	   (fp32consts[15]) // 2.7182818284590452354
#define M_LOG2E	   (fp32consts[24]) // 1.4426950408889634074
#define M_LOG10E   (fp32consts[21]) // 0.43429448190325182765
#define M_LN2	   (fp32consts[16]) // _M_LN2
#define M_LN10	   (fp32consts[19]) // 2.30258509299404568402
#define M_PI	   (fp32consts[29]) // 3.14159265358979323846
#define M_TWOPI    (fp32consts[30])
#define M_PI_2	   (fp32consts[31]) // 1.57079632679489661923
#define M_PI_4	   (fp32consts[32]) // 0.78539816339744830962
#define M_3PI_4	   (fp32consts[33]) // 2.3561944901923448370E0
#define M_SQRTPI   (fp32consts[36]) // 1.77245385090551602792981
#define M_1_PI	   (fp32consts[34]) // 0.31830988618379067154
#define M_2_PI	   (fp32consts[35]) // 0.63661977236758134308
#define M_2_SQRTPI (fp32consts[37]) // 1.12837916709551257390
#define M_SQRT2    (fp32consts[39]) // 1.41421356237309504880
#define M_SQRT1_2  (fp32consts[38]) // 0.70710678118654752440
#define M_SQRT3    (fp32consts[40]) // 1.73205080756887719000
#define M_IVLN10   (fp32consts[21]) // M_LOG10E
#define M_LOG2_E   (fp32consts[16]) // _M_LN2;
#define M_INVLN2   (fp32consts[24]) // M_LOG2E

#define MAXFLOAT   (fp32consts[7])

#define M_LN2LO    1.9082149292705877000E-10
#define M_LN2HI    6.9314718036912381649E-1

#endif
