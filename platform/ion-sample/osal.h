/* Copyright (c) 2011-2013, 2016 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

/**
    @file osal.h
    @brief  This header file defines the OS Abstraction
    Layer for the test infrastructure.
*/

#ifndef CLST_OSAL_H
#define CLST_OSAL_H



#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

// Buffer size for _list_of_files_node
// Should be large enough to store the largest full path to a file.
#define FULLPATH_BUFFER_SIZE 256

// This is necessary for Android to define size_t
#if !defined(_WIN32)
    #include <sys/types.h>
#endif

//////////////////////////////////////////////////////////////////////////////
//   entrypoint abstraction
//////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32) && !defined (_WIN32_WCE)
    #ifdef BUILD_STATIC_OSAL
        #define OSAL_DLLEXPORT	extern
        #define OSAL_DLLIMPORT	extern
    #else
        #ifdef __KERNEL_MODE__
            #define OSAL_DLLEXPORT	extern
        #else	// __KERNEL_MODE__
            // the kernel module has no exports, but the user module does
            #define OSAL_DLLEXPORT	__declspec(dllexport)
        #endif
        #define OSAL_DLLIMPORT	__declspec(dllimport)
    #endif
#elif defined(_WIN32) && defined (_WIN32_WCE)
    #define OSAL_DLLEXPORT	__declspec(dllexport)
    #define OSAL_DLLIMPORT
#elif defined(__SYMBIAN64__)
    // this is a temporary definition for Symbian OS since
    // currently the gsl & os libs are wrapped into a single dll
    #ifdef __KERNEL_MODE__
        #define OSAL_DLLEXPORT	extern
    #else	// __KERNEL_MODE__
        // the kernel module has no exports, but the user module does
        #define OSAL_DLLEXPORT	EXPORT_C
    #endif	// __KERNEL_MODE__
    #define OSAL_DLLIMPORT	IMPORT_C
#else
    #define OSAL_DLLEXPORT	extern
    #define OSAL_DLLIMPORT	extern
#endif

//////////////////////////////////////////////////////////////////////////////
//   OS lib entrypoints
//////////////////////////////////////////////////////////////////////////////
#ifdef OSAL_EXPORTS
    #define OSAL_API			OSAL_DLLEXPORT
#else
    #define OSAL_API			OSAL_DLLIMPORT
#endif // OSAL_EXPORTS

#if (defined (WIN32) && (_MSC_VER))
typedef __int64        long64;
#elif defined(_LINUX)
typedef long long      long64;
#else
typedef long long      long64;
#endif

#include <assert.h>
#define CLST_ASSERT(expr) assert(expr)

#define CLST_UNREFERENCED_PARAM(param) \
    param;

//////////////////////////////////////////////////////////////////////////////
typedef enum _clst_color
{
    CLST_COLOR_DEFAULT = 0,
    CLST_COLOR_RED     = 1,
    CLST_COLOR_GREEN   = 2
} clst_color;
//////////////////////////////////////////////////////////////////////////////

OSAL_API int clst_printf(const char *format, ...);

/** Print a formatted string into another string.
  * \param[out] dest   Pre-allocated string.
  * \param[in]  format A formatted string.
  * \return Number of characters printed or a negative number upon error.
  */
OSAL_API int clst_sprintf(char *dest, const char *format, ...);

OSAL_API int clst_snprintf(char *dest, size_t count, const char *format, ...);

/** Return a string's length without the NULL terminator. */
OSAL_API size_t clst_strlen(const char *str);

OSAL_API void*  clst_calloc(unsigned num_bytes);

OSAL_API void   clst_free(void* ptr);

OSAL_API void*  clst_memset(void *s, int c, size_t n);

OSAL_API void* clst_memcpy(void *s1, const void *s2, size_t n);

OSAL_API int clst_memcmp(const void *s1, const void *s2, size_t n);

/** Returns a timestamp in some unknown units of time */
OSAL_API long64   clst_clock_start(void);

/** Returns the time elapsed since the given #start timestamp, measured in seconds. */
OSAL_API double   clst_clock_diff(long64 start);

OSAL_API void     clst_usleep(unsigned useconds);

// See https://llvm.org/svn/llvm-project/llvm/trunk/lib/System/Win32/Process.inc and
//     https://llvm.org/svn/llvm-project/llvm/trunk/lib/System/Unix/Process.inc
/** Change the color used to display the next messages on the standard output. */
OSAL_API void     clst_set_console_color(clst_color color);

/* Definitions for function clst_get_filelist and clst_read_file */
struct _list_of_files_node
{
   char     filename[FULLPATH_BUFFER_SIZE];
   char     path[FULLPATH_BUFFER_SIZE];
   struct   _list_of_files_node* next;
};
typedef struct _list_of_files_node* filelist;

OSAL_API void   clst_get_filelist(char* find_path, char* file_extension, filelist* list_head);

OSAL_API void   clst_read_file(char* path, char** data);

typedef void (*clst_thread_func_type)(void*);

/** OS Agnostic interface for creating threads. On success will
  specify non-null handle and will return associated opaque
  thread_handle.  clst_thread_terminate must be called.

  On failure, thread_handle will be NULL and
  clst_thread_terminate should not be called.

  @param[in,out] thread_handle  Opaque handle to thread
  @param[in] stack_size         Stack size in bytes, 0 = default, OS
  @param[in] func               Thread function
  @param[in] args               Arguments to pass to function

  @return:
    0 on success
    negative value on failure
*/
OSAL_API  int clst_thread_create(void **thread_handle,
                                 int  stack_size,
                                 clst_thread_func_type func,
                                 void *args);

/** OS Agnostic interface for waiting on a thread to complete.

  @param[in] thread_handle  Opaque handle to thread.
*/
OSAL_API void clst_thread_join(void *thread_handle);

/** OS Agnostic interface for terminating a thread. This
  function will free the resources associated with
  thread_handle.

  @param[in] thread_handle  Opaque handle to thread.

  @return
    0 on success
    negative value on failure
*/
OSAL_API int clst_thread_terminate(void * thread_handle);

OSAL_API int clst_abs(int number);

#endif // !defined CLST_OSAL_H
