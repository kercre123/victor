#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*-----------------------------------------------------------
 (C) Copyright 2014 Signal Essence LLC; All Rights Reserved 

Module Name  - se_snprintf.h

Author: Robert Yu

History
2014-05-05   ryu  created

Machine/Compiler: ANSI C
-------------------------------------------------------------*/
#ifndef SE_SNPRINTF_H
#define SE_SNPRINTF_H

#include <stdlib.h>


#ifdef _MSC_VER
//
// ms visual studio doesn't support snprintf,
// so we have to supply our own
int se_snprintf_msvc_hack(char* str, size_t size, const char* format, ...);
#define se_snprintf se_snprintf_msvc_hack

#else
//
// use stdlib version
#define se_snprintf snprintf

#endif // _msc_ver

#endif // se_snprintf_h

#ifdef __cplusplus
}
#endif
