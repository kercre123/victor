///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Standard I/O Library header
/// 
/// The stdio library is implemented as using #defines to link to approriate Uart driver functions
/// 

#ifndef STDIO_H
#define STDIO_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include <stddef.h>
#include <DrvUart.h>

// 2: Types
// ----------------------------------------------------------------------------
typedef int FILE;

// 3:  External prototypes
// ----------------------------------------------------------------------------
int _xprintf( int (*)(int), char*, const char*,... );

// 4:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
#define stdin ((FILE*)1)
#define stdout ((FILE*)2)

// 5:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// Optionally disable all system printf operations
// Note: The SYSTEM_PUT_CHAR_FUNCTION Macro atomatically resolves the appropriate 
// means for displaying a character on the target platform. (e.g. Board, Software Sim, Hardware sim)
#ifdef NO_PRINT
#   define printf(...) (void)0
#   define putchar(c)  (void)0
#   define putc(c,f)   (void)0
#   define fputc(c,f)  (void)0
#   define puts(s)     (void)0
#   define fputs(s,f)  (void)0
#   define fflush(f)   (void)0
#else 
#   define printf(...) (_xprintf(SYSTEM_PUTCHAR_FUNCTION , 0, __VA_ARGS__ ) )
#   define putchar(c)      (SYSTEM_PUTCHAR_FUNCTION(c))
#   define putc(c,f)       (((void)(f)),DrvApbUartPutChar(c))
#   define fputc(c,f)      (((void)(f)),DrvApbUartPutChar(c))
#   define puts(s)         (systemPuts(SYSTEM_PUTCHAR_FUNCTION,s))
#   define fputs(s,f)      (((void)(f)),UART_fputs(s))
#   define fflush(f)       (((void)(f)),DrvApbUartFlush())
#endif

#define getchar()       (UART_getchar())
#define getc(f)         (((void)(f)),UART_getchar())
#define fgetc(f)        (((void)(f)),UART_fgetc())
#define gets(s)         (UART_gets(s))
#define fgets(s,f)      (((void)(f)),UART_fgets(s))
#define sprintf(s,...)  _xprintf(0,(s),__VA_ARGS__)

int systemPuts(int (*systemPutCharFn)(int character) ,const char * str);

#endif // STDIO_H
