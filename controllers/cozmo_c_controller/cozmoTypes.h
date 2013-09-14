#ifndef COZMO_TYPES_H
#define COZMO_TYPES_H

// Some of this should be moved to a shared file to be used by basestation as well


// We specify types according to their sign and bits. We should use these in
// our code instead of the normal 'int','short', etc. because different
// compilers on different architectures treat these differently.
typedef unsigned char       u8;
typedef signed char         s8;
typedef unsigned short      u16;
typedef signed short        s16;
typedef unsigned long       u32;
typedef signed long         s32;
typedef unsigned long long  u64;
typedef signed long long    s64;

// Maximum values
#define u8_MAX ((u8)0xFF)
#define u16_MAX ((u16)0xFFFF)
#define u32_MAX ((u32)0xFFFFFFFF)
#define s8_MAX ((s8)0x7F)
#define s16_MAX ((s16)0x7FFF)
#define s32_MAX ((s32)0x7FFFFFFF)
#define s8_MIN ((s8)0x80)
#define s16_MIN ((s16)0x8000)
#define s32_MIN ((s32)0x80000000)

// Memory map for our core (STM32F051)
#define CORE_CLOCK          48000000
#define CORE_CLOCK_LOW      (CORE_CLOCK / 6)
#define ROM_SIZE            65536
#define RAM_SIZE            8192
#define ROM_START           0x08000000
#define FACTORY_BLOCK       0x08001400
#define USER_BLOCK_A        0x08001800
#define USER_BLOCK_B        0x08001C00    // Must follow block A
#define APP_START           0x08002000
#define APP_END             0x08010000
#define RAM_START           0x20000000
#define CRASH_REPORT_START  0x20001fc0    // Final 64 bytes of RAM
#define VECTOR_TABLE_ENTRIES 48

// So we can explicitly specify when something is interpreted as a boolean value
typedef u8 BOOL;

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef MAX
  #define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
  #define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

#ifndef CLIP
  #define CLIP(n, min, max) ( MIN(MAX(min, n), max ) )
#endif

#ifndef ABS
  #define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

#ifndef SIGN
  #define SIGN(a) ((a) >= 0)
#endif

#ifndef NULL
  #define NULL (0)
#endif


#endif // COZMO_TYPES_H