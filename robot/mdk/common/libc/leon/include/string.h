#ifndef _MV_STRING_H_
#define _MV_STRING_H_

#include <stddef.h>

void *memcpy( void*, const void*, size_t );
void *__memcpy_fast( void *dest, const void *src, unsigned count );
void *memmove( void*, const void*, size_t );
char *strcpy( char*, const char* );
char *strncpy( char*, const char*, size_t );

char *strcat( char*, const char* );
char *strncat( char*, const char*, size_t );

int memcmp( const void*, const void*, size_t );
int strcmp( const char*, const char* );
int strncmp( const char*, const char*, size_t );

void *memchr( const void*, int, size_t );
char *strchr( const char*, int );
size_t strcspn( const char*, const char*);
//char *strpbrk( const char*, const char* ); // not in rom.o
char *strrchr( const char*, int );
size_t strspn( const char*, const char* );
char *strstr( const char*, const char* );
//char *strtok( char*, const char* ); / /not in rom.o

void *memset( void*, int, size_t );
void *__memset_fast( void *dest, int value, unsigned count );

size_t strlen( const char* );
// locales not supported
#define strcoll(src1,src2) strcmp((src1),(src2)) 
#define strxfrm(dest,src,n) ((((n))?(memset((dest),0,((n)+1)),strncpy((dest),(src),(n))):0),strlen((dest)))
// no room for messages
#define strerror(x) ((x),"")

#endif // _MV_STRING_H_
