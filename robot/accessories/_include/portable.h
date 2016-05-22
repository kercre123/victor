// Portability is an amusing concept on an 8051
#ifndef PORTABLE_H__
#define PORTABLE_H__

typedef unsigned char         u8;
typedef unsigned int          u16;
typedef unsigned long         u32;

typedef signed char           s8;
typedef signed int            s16;
typedef signed long           s32;

#define ilsb(v)  (*(((unsigned char idata *) (&v) + 1)))
#define imsb(v)  (*((unsigned char idata *) (&v)))

#define u16p(v) (*((unsigned short *) (&v)))
  
#define lessthanpow2(x,y)  !((x)&(u8)(~((y)-1)))

// XRAM is a 512-byte scratchpad for buffering accelerometer and OTA data
// It is most efficiently accessed as a 256-byte "pdata" page via _pram
#define _pram ((u8 pdata *)0)

#endif /* PORTABLE_H__ */
