#ifdef __cplusplus
extern "C" {
#endif

/*
 (C) Copyright 2009 Signal Essence; All Rights Reserved

 Module Name  - se_error.h

 Author: Hugh McLaughlin
         Rob Yu

 History

   04-08-09       hjm    created
   2012-09-24     ryu    rewrote for platform independence; added user-defined callback

 Description

 Machine/Compiler: ANSI C

-------------------------------------------------------------*/

#ifndef __se_error_h
#define __se_error_h

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "se_types.h"

/*
define SE_ERROR_SUPPORT_TEST_CALLBACK when you want to 
define your own callback function, which is invoked when
an assert fails.

The filename as linenumber are passed are arguments.

This functionality is used for unit testing (e.g. see test_scratch.c)
*/
#if defined(SE_ERROR_SUPPORT_TEST_CALLBACK)
typedef void (*SeErrorAssertCallback_t)(const char *pFilename, int lineNumber);

extern SeErrorAssertCallback_t SeErrorAssertCallback;

void SeErrorSetAssertCallback(SeErrorAssertCallback_t func);
SeErrorAssertCallback_t SeErrorGetAssertCallback(void);
#else
/*
Define the callback functions to something innocuous.
This is a workaround for when certain customers require 2 instances of
the MMFX libraries.
*/
    #define SeErrorSetAssertCallback(x) ;
    #define SeErrorGetAssertCallback(x) ;
    #define SeErrorAssertCallback(x,y)  ;
#endif  

//
// assertions should *never* be defined as <nothing>
// because the se_lib code contains assertions in which
// real work is done in the expression, such as:
//      SeAssert(  0==free_memory(p)   )
// In the above example, if SeAssert is mapped to nothing,
// then the free_memory() function is never called
//
// Likewise, we have to be careful about using <assert.h>::assert(), because
// in gcc, defining NDEBUG maps assert() to nothing.
//
#ifdef SeAssertCustom

void SeAssert(int _expr);
#define SeAssertString(_expr, _str)  SeAssert(_expr)

#elif defined(SE_ERROR_SUPPORT_TEST_CALLBACK)

/*
define SeAssert and SeAssertString to support the callback function
*/
#define SeAssert(_expr)                                                 \
{                                                                                          \
    volatile int exprResult;                                                               \
    int *px = NULL;                                                                        \
    exprResult = (int)(_expr);                                                             \
    if (0==exprResult)                                                                     \
    {                                                                                      \
        if (NULL != SeErrorAssertCallback) /* if user defined callback, then invoke it */  \
        {                                                                            \
            (SeErrorAssertCallback)(__FILE__,__LINE__);                              \
            /* For testing, we *don't* want to cause an assert so that test harness */ \
            /* can recover from an assert                                           */ \
        }                                                                            \
        else                                                                         \
        {                                                                            \
            fprintf(stderr, "\nSeAssert: %s:%d\n",__FILE__,__LINE__);fflush(stderr); \
            *px = 0;  /* try to induce a SEGV */                                     \
            assert(0); /* also call assert, just for good measure */                 \
        }                                                                            \
    }                                                                                \
}

#define SeAssertString(_expr, _str) \
{                                      \
    volatile int exprResult;           \
    exprResult = (int)(_expr);         \
    if (0==exprResult)                 \
      fprintf(stderr, "%s:%d, %s\n", __FILE__, __LINE__, _str);	\
    SeAssert(_expr); \
} 
#else //defined(SE_ERROR_SUPPORT_TEST_CALLBACK)

/*
define SeAssert and SeAssertString without callback functionality (default)
*/
#define SeAssert(_expr)                                                 \
{                                                                                          \
    volatile int exprResult;                                                               \
    int *px = NULL;                                                                        \
    exprResult = (int)(_expr);                                                             \
    if (0==exprResult)                                                                     \
    {                                                                                      \
        {                                                                            \
            fprintf(stderr, "\nSeAssert: %s:%d\n",__FILE__,__LINE__);fflush(stderr); \
            *px = 0;  /* try to induce a SEGV */                                     \
            assert(0); /* also call assert, just for good measure */                 \
        }                                                                            \
    }                                                                                \
}

#define SeAssertString(_expr, _str) \
{                                      \
    volatile int exprResult;           \
    exprResult = (int)(_expr);         \
    if (0==exprResult)                 \
      fprintf(stderr, "%s:%d, %s\n", __FILE__, __LINE__, _str);	\
    SeAssert(_expr); \
} 
#endif //#else


#endif  // ifndef

#ifdef __cplusplus
}
#endif

