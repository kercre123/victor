#ifndef PORTABLE_H__
#define PORTABLE_H__

typedef unsigned char         u8;
typedef unsigned int          u16;
typedef unsigned long         u32;

typedef signed char           s8;
typedef signed int            s16;
typedef signed long           s32;

// Basic abstractions for low level I/O on our processor
#define GPIO_SET(p, x)            (p) |= (x)
#define GPIO_RESET(p, x)          (p) &= ~(x)
#define GPIO_READ(p)              (p)

#define PIN_IN(p, x)              (p) |= (x)
#define PIN_OUT(p, x)             (p) &= ~(x)

#endif /* PORTABLE_H__ */