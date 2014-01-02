#ifndef _MV_STDLIB_H_
#define _MV_STDLIB_H_

#include <stddef.h>

// char *itoa( int value, char *buffer, int base );
// char *ftoa( int value, char *buffer, int precision );
long strtol(const char *nptr, char **endptr, int base);
int atoi(const char *str);

// abs is a built-in gcc function
int abs( int );

void exit( int );

// Note: does NOT conform to ANSI C
// ANSI C:
//   int atexit( void (*)() );
//   - return 0 on success
// MOVIDIA implementation:
//   void (*atexit)()( void (*)() );
//   - always succeeds, returns the pointer to the previously-set atexit function
//   - the return pointer is guaranteed to be always callable (a dummy default function is implemented)
// typedef void (* atexit_function_t)( void );
// atexit_function_t atexit( atexit_function_t );

#endif //_STDLIB_H_
