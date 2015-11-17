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

#define GPIO_SET_MP(p, x)         switch((p)) {case 0: GPIO_SET(P0, (x)); break; case 1: GPIO_SET(P1, (x)); break;}
#define GPIO_RESET_MP(p, x)       switch((p)) {case 0: GPIO_RESET(P0, (x)); break; case 1: GPIO_RESET(P1, (x)); break;}
#define GPIO_READ_MP(p)           switch((p)) {case 0: GPIO_READ(P0); break; case 1: GPIO_READ(P1) ; break;}
#define PIN_IN_MP(p, x)           switch((p)) {case 0: PIN_IN(P0DIR, (x)); break; case 1: PIN_IN(P1DIR, (x); break;}
#define PIN_OUT_MP(p, x)          switch((p)) {case 0: PIN_OUT(P0DIR, (x)); break; case 1: PIN_OUT(P1DIR, (x)); break;}



#endif /* PORTABLE_H__ */