#ifndef PORTABLE_H
#define PORTABLE_H

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define static_assert(e, msg) \
  enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
  
#endif
