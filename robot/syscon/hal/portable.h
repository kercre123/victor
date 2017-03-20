#ifndef __PORTABLE_H
#define __PORTABLE_H

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;
typedef float               f32;
typedef double              f64;

#ifndef Fixed
typedef s32                 Fixed;
#endif

#ifndef MAX
#define MAX(x,y) ((x < y) ? (y) : (x))
#endif

#define ABS(x) ((x) < 0 ? -(x) : (x))

#endif
