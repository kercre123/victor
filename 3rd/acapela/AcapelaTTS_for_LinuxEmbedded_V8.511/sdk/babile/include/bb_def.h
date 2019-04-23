#ifndef __COMMON__BB_DEF_H__
#define __COMMON__BB_DEF_H__

#define BOM_STR_UTF8 "\xef" "\xbb" "\xbf"

#if defined(__cplusplus)
	#define BB_CPLUSPLUS 1
#endif


/* BB_GCC_ATTR() family
   
   Helper macros for GCC's attribute syntax and format attribute syntax. */
#if defined(__GNUC__) || defined(__clang__)
	#if defined(__MINGW32__)
		/*#define _BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF  __MINGW_PRINTF_FORMAT
		#define _BB_GCC_ATTR_FMT_ARCHETYPE_SCANF   __MINGW_SCANF_FORMAT*/
		#define _BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF  __gnu_printf__
		#define _BB_GCC_ATTR_FMT_ARCHETYPE_SCANF   __gnu_printf__
	#else
		#define _BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF  __printf__
		#define _BB_GCC_ATTR_FMT_ARCHETYPE_SCANF   __scanf__
	#endif
	
	#define _BB_GCC_ATTR_FMT_ARCHETYPE_STRFTIME  __strftime__
	#define BB_GCC_ATTR(a)                                        __attribute__((a))
	#define BB_GCC_ATTR_FMT(archetype, stridx, firstargidx)       BB_GCC_ATTR(__format__(archetype, stridx, firstargidx))
	#define BB_GCC_ATTR_FMT_THIS(archetype, stridx, firstargidx)  BB_GCC_ATTR_FMT(archetype, (stridx + 1), (firstargidx + 1))
	#define BB_GCC_ATTR_FMT_PRINTF(stridx, firstargidx)           BB_GCC_ATTR_FMT(_BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_PRINTF_THIS(stridx, firstargidx)      BB_GCC_ATTR_FMT_THIS(_BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_VPRINTF(stridx)                       BB_GCC_ATTR_FMT(_BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF, stridx, 0)
	#define BB_GCC_ATTR_FMT_VPRINTF_THIS(stridx)                  BB_GCC_ATTR_FMT(_BB_GCC_ATTR_FMT_ARCHETYPE_PRINTF, (stridx + 1), 0)
	#define BB_GCC_ATTR_FMT_SCANF(stridx, firstargidx)            BB_GCC_ATTR_FMT(_BB_GCC_ATTR_FMT_ARCHETYPE_SCANF, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_SCANF_THIS(stridx, firstargidx)       BB_GCC_ATTR_FMT_THIS(_BB_GCC_ATTR_FMT_ARCHETYPE_SCANF, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_STRFTIME(stridx, firstargidx)         BB_GCC_ATTR_FMT(_BB_GCC_ATTR_FMT_ARCHETYPE_STRFTIME, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_STRFTIME_THIS(stridx, firstargidx)    BB_GCC_ATTR_FMT_THIS(_BB_GCC_ATTR_FMT_ARCHETYPE_STRFTIME, stridx, firstargidx)
	#define BB_GCC_ATTR_UNUSED                                    BB_GCC_ATTR(__unused__)
	#define BB_GCC_ATTR_USED                                      BB_GCC_ATTR(__used__)
	#define BB_GCC_ATTR_NONNULL                                   BB_GCC_ATTR(__nonnull__)
	#define BB_GCC_ATTR_NORET                                     BB_GCC_ATTR(__noreturn__)
	#define BB_GCC_ATTR_NOINL                                     BB_GCC_ATTR(__noinline__)
	#define BB_GCC_ATTR_PURE                                      BB_GCC_ATTR(__pure__)
	#define BB_GCC_ATTR_CONST                                     BB_GCC_ATTR(__const__)
	#define BB_GCC_ATTR_WARNUNUSEDRES                             BB_GCC_ATTR(__warn_unused_result__)
#else
	#define BB_GCC_ATTR(a)
	#define BB_GCC_ATTR_FMT(archetype, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_THIS(archetype, stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_PRINTF(stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_PRINTF_THIS(stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_VPRINTF(stridx)
	#define BB_GCC_ATTR_FMT_VPRINTF_THIS(stridx)
	#define BB_GCC_ATTR_FMT_SCANF(stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_SCANF_THIS(stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_STRFTIME(stridx, firstargidx)
	#define BB_GCC_ATTR_FMT_STRFTIME_THIS(stridx, firstargidx)
	#define BB_GCC_ATTR_UNUSED
	#define BB_GCC_ATTR_USED
	#define BB_GCC_ATTR_NONNULL
	#define BB_GCC_ATTR_NORET
	#define BB_GCC_ATTR_NOINL
	#define BB_GCC_ATTR_PURE
	#define BB_GCC_ATTR_CONST
	#define BB_GCC_ATTR_WARNUNUSEDRES
#endif


/* PUBLIC_CBASE and NONSHARABLE_CLASS definition */
#ifdef BB_CPLUSPLUS
#	if defined (__SYMBIAN32__) /* SYMBIAN C++ specifics */
#		include <e32base.h> /* CBase class definition, must be parent for all C++ classes */
#		define PUBLIC_CBASE	:public CBase      
#	else
#       define NONSHARABLE_CLASS(x) class x
#   endif
#endif

/* BBT_EXPORT_ and _BBT_EXPORT definition */
#if defined (__SYMBIAN32__) /* Take care that __SYMBIAN32__ and WIN32 may be defined together */
# if defined (BB_LIBRARY_EXPORT)
	#define BBT_EXPORT_ EXPORT_C
# else
	#define BBT_EXPORT_ 
# endif
	#define _BBT_EXPORT
#elif defined (WIN32) || defined (_WIN32) || defined (_WIN32_WCE)
	#define BBT_EXPORT_ __declspec( dllexport )
	#define _BBT_EXPORT
#else
	#define _BBT_EXPORT __attribute__ ((visibility ("default"))) 
	#define BBT_EXPORT_ __attribute__ ((visibility ("default"))) 
#endif

/* BBT_IMPORT_ and _BBT_IMPORT definition */
#if defined (__SYMBIAN32__) /* Take care that __SYMBIAN32__ and WIN32 may be defined together */
# if !defined (BB_LIBRARY_EXPORT)
	#define BBT_IMPORT_ IMPORT_C
# else
	#define BBT_IMPORT_ 
# endif
	#define _BBT_IMPORT
#elif defined (WIN32) || defined (_WIN32) || defined (_WIN32_WCE)
	#define _BBT_IMPORT __stdcall
	#define BBT_IMPORT_
#else
	#define _BBT_IMPORT
	#define BBT_IMPORT_
#endif 

#define COMPILE_TIME_ASSERT(condition)   ((void)sizeof(char[1 - 2*!!(condition)]))


# define BB_TRACE_LEVEL 1
#if defined (__IAR_EWB__) || (/* GCC (MinGW) and Clang for Win32 */ (defined(WIN32) || defined(_WIN32)) && (!defined(_MSC_VER) || (defined(_MSC_VER) && defined(__clang__))))
# undef HAVE_ALLOCA_H
#else
# define HAVE_ALLOCA_H
#endif
/*-----------------------------------------------------------------------------------*/
/* BB_malloc, BB_free, BB_alloca, fast_clock, BB_INLINE, FILE functions definition */
#ifdef BB_CPLUSPLUS
extern "C"
{
#endif
#ifdef __SYMBIAN32__
# define fast_clock() User::FastCounter()
# define BB_abs(A)			abs(A)
# define BB_malloc(A) malloc(A)
# define BB_calloc(N,S) calloc(N,S)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# if defined (__CW32__)
#  define BB_INLINE inline
# else
#  define BB_INLINE __inline
# endif
# ifdef HAVE_ALLOCA_H
#  include <stdlib.h>
#  include <stdio.h>
#  include <errno.h>
#  include <string.h>
#  include <math.h>
#  if (defined( __WINS__ )  && (defined(__VC32__) || defined(__CW32__))) 
#   define BB_alloca(A)		_alloca(A)
#  elif defined (__MARM__)
#   define _ARM_ASSEM_ 1
#   define BB_alloca(A) __builtin_alloca(A)
#  endif
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# if 0 /* IT WORKS BETTER (faster) WITH C ANSI functions than native SYmbian ones!!!!*/
#  define DBASPI_FTELL(A)		SYMBIAN_ftell(A)
#  define DBASPI_FSEEK(A,B,C)	SYMBIAN_fseek(A,B,C)
#  define DBASPI_FREAD(A,B,C,D)	SYMBIAN_fread(A,B,C,D)
#  define DBASPI_FOPEN_RB(A)	SYMBIAN_fopen(A,"rb",dbId->handle.pFs)
#  define DBASPI_FOPEN_WB(A)	SYMBIAN_fopen(A,"wb",dbId->handle.pFs)
#  define DBASPI_FOPEN_RWB(A)	SYMBIAN_fopen(A,"r+b",dbId->handle.pFs)
#  define DBASPI_FCLOSE(F)		SYMBIAN_fclose(F)
#  define DBASPI_FEOF(F)			feof(F)
# endif
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)	fseek((FILE*)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof((FILE*)F)
/*-----------------------------------------------------------------------------------*/
#elif defined (__HEW_IDE__)
# define BB_INLINE 
#include <stdio.h>
#include <stdlib.h>
# define EOF	(-1)
# define BB_abs(A)			((A)>=0?(A):(0-(BB_NS32)(A)))
# define fast_clock() clock()
# define BB_malloc(A) (XXXX +++)
# define BB_free(A) (+XXXX+)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# ifdef HAVE_ALLOCA_H
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define DBASPI_FTELL(A)		(dbHandle->setOffset)
# define DBASPI_FSEEK(A,B,C)	(C==SEEK_SET?dbHandle->setOffset=B:(C==SEEK_CUR?dbHandle->setOffset=dbHandle->current+B:(C==SEEK_END?dbHandle->setOffset=dbHandle->size+B:0)))
# define DBASPI_FREAD(A,B,C,D)	XXX exit()
# define DBASPI_FOPEN(A,B)		XXX exit()
# define DBASPI_FCLOSE(F)		TTSLIB_Close((int)F)
# define DBASPI_FEOF(F)			(dbHandle->current>=dbHandle->size?1:0)
#elif defined( __CLBCK_ADS12__) 
# include "Engine.h"
  extern const sEngineParam* DBASPI_FSFunct;
# define EOF	(-1)
# define BB_abs(A)			((A)>=0?(A):(0-A))
# define fast_clock() clock()
# define BB_malloc(S)		DBASPI_FSFunct->pfpMemAlloc(S)
# define BB_free(S)			DBASPI_FSFunct->pfpMemFree(S)
# define BB_memset(A,B,C)	DBASPI_FSFunct->pfpMemSet(A,B,C)
# define BB_memmove(A,B,C)	DBASPI_FSFunct->pfpMemMove(A,B,C)
# define BB_memcpy(A,B,C)	DBASPI_FSFunct->pfpMemCpy(A,(void*)B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
#  define BB_INLINE __inline
# ifdef HAVE_ALLOCA_H
#  define BB_alloca(A)		DBASPI_FSFunct->pfpMemAlloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
static  void* bbCBCKMemCalloc(UB32_T num,UB32_T size)
  {
  	void* o=BB_malloc(num*size);
  	if (o) 
  	{
  		BB_memset(o,0,num*size);
  		return o;
  	}
  	return NULL;
  }
# define BB_calloc(N,S) bbCBCKMemCalloc(N,S)

static  sBBFile* bbCBCKFileOpen (const TCHAR* pszFileName, const TCHAR* access)
  {
	  sBBFile* f=(sBBFile*)BB_malloc(sizeof(sBBFile));
	  if (f)
	  {
	  	BOOL_T b=FALSE;
	  	BB_memset(f,0,sizeof(sBBFile));
	  	if (access && *access=='w') {b=TRUE;}
	  	b=DBASPI_FSFunct->pfpFileOpen(pszFileName,f,b);
	  	if (b==TRUE)
	  	{
	  		return f;
	  	}
	  	BB_free(f);
	  }
	  return NULL;
  }
static  int bbCBCKFileClose (sBBFile* f)
  {
  	if (f)
  	{ 
  		DBASPI_FSFunct->pfpFileClose(f);
  		BB_free(f);
  	}
  	return 0;
  }

# define DBASPI_FTELL(A)		DBASPI_FSFunct->pfpFileTell(A)
# define DBASPI_FSEEK(A,B,C)	DBASPI_FSFunct->pfpFileSeek((sBBFile*)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	DBASPI_FSFunct->pfpFileRead(D,A,(B)*(C))
# define DBASPI_FWRITE(A,B,C,D)	DBASPI_FSFunct->pfpFileWrite(D,A,(B)*(C))
# define DBASPI_FOPEN_RB(A)		bbCBCKFileOpen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		bbCBCKFileOpen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	bbCBCKFileOpen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		bbCBCKFileClose(F)
# define DBASPI_FEOF(F)			DBASPI_FSFunct->pfpFileFeof(F)

/*-----------------------------------------------------------------------------------*/
#elif defined (_WIN32_WCE) /* for WCE emulator, WIN32 is also defined */
#include <stdlib.h>
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_calloc(N,S) calloc(N,S)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# ifndef _DEBUG
#  define BB_INLINE __forceinline
# else
#  define BB_INLINE 
# endif
# ifdef HAVE_ALLOCA_H
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)	fseek((FILE*)A,B,C)
# define DBASPI_FSEEKU32(A,B,C)	_fseeki64((FILE*)A, (BB_U32)B, C)
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof((FILE*)F)
/*-----------------------------------------------------------------------------------*/
#elif defined (WIN32) || defined (_WIN32)
#if defined(_MSC_VER) && !defined(__clang__)
#   pragma warning( push )
#	pragma warning (disable : 4001)		/* Avoid the warning in STD headers!!! (VS8) : warning C4001: nonstandard extension 'single line comment' was used */
#include <string.h>
#include <stdlib.h>
#	pragma warning ( pop )
#else
#include <string.h>
#include <stdlib.h>
#endif
#include <stdio.h>
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
/* `BB_ALWAYSINLINE` attempt to force the inlining of the function to which 
   this directive is applied.
   
   `BB_INLINE` and `BB_MAYINLINE` ask for inlining the function to which this 
   directive is applied (at the sole discretion of the compiler's criteria 
   and decision). */
# if defined(_MSC_VER) && !defined(__clang__)
/* DH: VS' inlining-related in-language user facilities really just suck: 
   `__forceinline` doesn't quite work */
/*#  define BB_ALWAYSINLINE __forceinline*/
#  define BB_ALWAYSINLINE __inline
# elif (defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) && (__GNUC_PATCHLEVEL__ >= 1)))) || defined(__clang__)
#  define BB_ALWAYSINLINE __inline__ BB_GCC_ATTR(__always_inline__)
# else
#  define BB_ALWAYSINLINE __inline__
# endif
# ifndef _DEBUG
#  define BB_INLINE BB_ALWAYSINLINE
# else
#  define BB_INLINE 
# endif
# if defined(_MSC_VER) && !defined(__clang__)
#  define BB_MAYINLINE __inline
# else
#  define BB_MAYINLINE __inline__
# endif
# ifdef HAVE_ALLOCA_H
#  include <malloc.h>
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define BB_calloc(N,S) calloc(N,S)
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)    fseek((FILE*)A, B, C)
# define DBASPI_FSEEKU32(A,B,C)	_fseeki64((FILE*)A, (BB_U32)B, C)
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FOPEN(A,B)		fopen((const char*)A,B)
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof((FILE*)F)
/*-----------------------------------------------------------------------------------*/
#elif defined (__IAR_EWB__)
# ifndef BB_CPLUSPLUS
#  include <stdio.h>
#  include <string.h>
#  include <stdlib.h>
# endif
# include <ioat91sam9260.h>
# include "api_f.h"
/*# include "mmcsd.h"*/
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# ifdef BB_CPLUSPLUS
#  define BB_INLINE __inline__
# else
#  define BB_INLINE
# endif
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define BB_calloc(N,S) calloc(N,S)
# define FILE  F_FILE
# define DBASPI_FTELL(A)		f_tell((F_FILE*)A)
# define DBASPI_FSEEK(A,B,C)	        f_seek((F_FILE*)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	        f_read(A,B,C,(F_FILE*)D)
# define DBASPI_FOPEN_RB(A)		f_open((const char*)A,"r")
# define DBASPI_FOPEN_WB(A)		f_open((const char*)A,"w")
# define DBASPI_FOPEN_RWB(A)	f_open((const char*)A,"r+")
# define DBASPI_FCLOSE(F)		f_close((F_FILE*)F)
# define DBASPI_FEOF(F)                 f_eof((F_FILE*)F)
/*-----------------------------------------------------------------------------------*/
#elif defined (__linux__) || defined(__APPLE__) || defined(PLATFORM_LINUX) 
/* SHQ wav files now over 2GB */
#define _FILE_OFFSET_BITS 64
#define _LARGE_FILES 64
#define _LARGEFILE64_SOURCE 1
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
/* `BB_ALWAYSINLINE` attempt to force the inlining of the function to which 
   this directive is applied.
   
   `BB_INLINE` and `BB_MAYINLINE` ask for inlining the function to which this 
   directive is applied (at the sole discretion of the compiler's criteria 
   and decision). */
# if (defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) && (__GNUC_PATCHLEVEL__ >= 1)))) || defined(__clang__)
#  define BB_ALWAYSINLINE __inline__ BB_GCC_ATTR(__always_inline__)
# else
#  define BB_ALWAYSINLINE __inline__
# endif
# ifdef BB_CPLUSPLUS
#  define BB_INLINE __inline__
# else
#  define BB_INLINE
# endif
# define BB_MAYINLINE __inline__
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define BB_calloc(N,S) calloc(N,S)
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)	fseek((FILE*)A,B,C)
#if defined(__APPLE__) || defined(ANDROID)
# define DBASPI_FSEEKU32(A,B,C)	fseeko((FILE*)A, (BB_U32)B,C)
#elif defined(__BABMACX__)
# define DBASPI_FSEEKU32(A,B,C) fseek((FILE*)A,B,C)
#else
# define DBASPI_FSEEKU32(A,B,C)	fseeko64((FILE*)A, (BB_U32)B,C)
#endif
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FOPEN(A,B)		fopen((const char*)A,B)
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof(F)
/*-----------------------------------------------------------------------------------*/
#elif defined __QNX__
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# define BB_INLINE inline
# ifdef HAVE_ALLOCA_H
#  include <alloca.h>
#  define BB_alloca(A)		alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)	fseek((FILE*)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof(F)
/*-----------------------------------------------------------------------------------*/
#elif defined OS_VXWORKS 
# define BB_INLINE
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# define BB_alloca(A)		BB_malloc(A)
# define BB_allocaFree(A)	BB_free(A)
# define DBASPI_FTELL(A)		ftell((FILE*)A)
# define DBASPI_FSEEK(A,B,C)	fseek((FILE*)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	fread(A,B,C,(FILE*)D)
# define DBASPI_FOPEN_RB(A)		fopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fopen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		fclose((FILE*)F)
# define DBASPI_FEOF(F)			feof(F)
/*-----------------------------------------------------------------------------------*/
#elif defined __EPSON_C33__
# define BB_INLINE __inline__
# define BB_abs(A)			abs(A)
# define fast_clock() clock()
# define BB_malloc(A) malloc(A)
# define BB_free(A) free(A)
# define BB_memset(A,B,C)	memset(A,B,C)
# define BB_memmove(A,B,C)	memmove(A,B,C)
# define BB_memcpy(A,B,C)	memcpy(A,B,C)
# define BB_memcmp(A,B,C)	BBANSI_memcmp(A,B,C)
# define BB_memchr(A,B,C)	BBANSI_memchr(A,B,C)
# ifdef HAVE_ALLOCA_H
#  define BB_alloca(A) __builtin_alloca(A)
#  define BB_allocaFree(A)
# else /* HAVE_ALLOCA_H */
#  define BB_alloca(A)		BB_malloc(A)
#  define BB_allocaFree(A)	BB_free(A)
# endif /* HAVE_ALLOCA_H */
# include "fs.h"
# define DBASPI_FTELL(A)		fsftell((FHANDLE)A)
# define DBASPI_FSEEK(A,B,C)	fsfseek((FHANDLE)A,B,C)
# define DBASPI_FREAD(A,B,C,D)	fsfread(A,B,C,(FHANDLE)D)
# define DBASPI_FOPEN_RB(A)		fsfopen((const char*)A,"rb")
# define DBASPI_FOPEN_WB(A)		fsfopen((const char*)A,"wb")
# define DBASPI_FOPEN_RWB(A)	fsfopen((const char*)A,"r+b")
# define DBASPI_FCLOSE(F)		fsfclose((FHANDLE)F)
# define DBASPI_FEOF(F)			fsfeof((FHANDLE)F)
#endif
#ifdef BB_CPLUSPLUS
}
#endif
/*-----------------------------------------------------------------------------------*/




/* ================================================================== */
/* + TYPES RELATED CONSTANTS                                        + */
/* ================================================================== */

/*
 *
 * Memory dependent INTEGER types
 * ===============================
 * BB_MAX_U8   BB_MIN_U8   BB_U8_MAX   BB_U8_MIN   
 * BB_MAX_S8   BB_MIN_S8   BB_S8_MAX   BB_S8_MIN   
 * BB_MAX_U16  BB_MIN_U16  BB_U16_MAX  BB_U16_MIN  
 * BB_MAX_S16  BB_MIN_S16  BB_S16_MAX  BB_S16_MIN  
 * BB_MAX_U32  BB_MIN_U32  BB_U32_MAX  BB_U32_MIN  
 * BB_MAX_S32  BB_MIN_S32  BB_S32_MAX  BB_S32_MIN  
 *
 * CPU dependent INTEGER types
 * ============================
 * BB_MAX_NU8  BB_MIN_NU8  BB_NU8_MAX  BB_NU8_MIN  
 * BB_MAX_NS8  BB_MIN_NS8  BB_NS8_MAX  BB_NS8_MIN  
 * BB_MAX_NU16 BB_MIN_NU16 BB_NU16_MAX BB_NU16_MIN 
 * BB_MAX_NS16 BB_MIN_NS16 BB_NS16_MAX BB_NS16_MIN 
 * BB_MAX_NU32 BB_MIN_NU32 BB_NU32_MAX BB_NU32_MIN 
 * BB_MAX_NS32 BB_MIN_NS32 BB_NS32_MAX BB_NS32_MIN 
 *
 * FLOAT types
 * ============
 * /
 *
 * BOOL types
 * ===========
 * /
 *
 * CHAR types
 * ===========
 * /
 *
 * ERROR types
 * ============
 * /
 *
 * VERSION types
 * ==============
 * /
 *
 * DATABASE FORMAT types
 * =====================
 * 
 *
 */


/**********************************************************************/
/*   MEMORY DEPENDENT INTEGER DEFINITION                              */
/**********************************************************************/


/* These (and other companions) are defined bb_types.h */


/* MAX & MIN constants for MEMORY-related types */
/*#if !defined(__GNUC__) || (__GNUC__ >= 3)
	#define BB_MAX_S64 0x7FFFFFFFFFFFFFFFll
	#define BB_MIN_S64 0x8000000000000000ll
	#define BB_MAX_U64 0xFFFFFFFFFFFFFFFFull
	#define BB_MIN_U64 0x0000000000000000ull
	#define BB_S64_MAX BB_MAX_S64
	#define BB_S64_MIN BB_MIN_S64
	#define BB_U64_MAX BB_MAX_U64
	#define BB_U64_MIN BB_MIN_U64
#endif 

#define BB_MAX_S32 0x7FFFFFFF
#define BB_MIN_S32 0x80000000
#define BB_MAX_U32 0xFFFFFFFFu
#define BB_MIN_U32 0x00000000u
#define BB_S32_MAX BB_MAX_S32
#define BB_S32_MIN BB_MIN_S32
#define BB_U32_MAX BB_MAX_U32
#define BB_U32_MIN BB_MIN_U32

#define BB_MAX_S16 0x7FFF
#define BB_MIN_S16 0x8000
#define BB_MAX_U16 0xFFFFu
#define BB_MIN_U16 0x0000u
#define BB_S16_MAX BB_MAX_S16
#define BB_S16_MIN BB_MIN_S16
#define BB_U16_MAX BB_MAX_U16
#define BB_U16_MIN BB_MIN_U16

#define BB_MAX_S8  0x7F
#define BB_MIN_S8  0x80
#define BB_MAX_U8  0xFFu
#define BB_MIN_U8  0x00u
#define BB_S8_MAX  BB_MAX_S8
#define BB_S8_MIN  BB_MIN_S8
#define BB_U8_MAX  BB_MAX_U8
#define BB_U8_MIN  BB_MIN_U8*/



/* ================================================================== */
/* + PLATFORM RELATED CONSTANTS                                     + */
/* ================================================================== */
 
/* Create CPU ID's */
#define __BB_CPU8__  0x01
#define __BB_CPU16__ 0x02
#define __BB_CPU32__ 0x03
#define __BB_CPU64__ 0x04
                   
/* Create MEM ID's */                   
#define __BB_MEM8__  0x10
#define __BB_MEM16__ 0x20
#define __BB_MEM32__ 0x30

/* Create ADDRESS ID's */                   
#define __BB_ADD16__ 0x80
#define __BB_ADD24__ 0x90
#define __BB_ADD32__ 0xA0
#define __BB_ADD64__ 0xB0

#ifndef _W64
#define _W64 /*to avoid warnings....*/
#endif

/* Predefined Platforms */
#if defined(WIN64) || defined(_WIN64) || defined(__LP64__) || defined(_LP64)
	 #define __BB_CPU_TYPE__ __BB_CPU64__
    #define __BB_MEMORY_TYPE__ __BB_MEM8__
	#define __BB_MEMSPACE_TYPE__ __BB_ADD64__
#elif defined(WIN32) || defined(_WIN32) || defined(TMS470Rxx_SIM) || defined(DARWIN_PPC) || defined(DARWIN_I386) || defined(LINUX_INTEL) || defined(SUN) || defined(CPU_RISC_ARM7) || defined(_WIN32_WCE) || defined(__SYMBIAN32__) || defined(PALMOSARM) || defined(OS_VXWORKS) || defined(__EPSON_C33__) || defined (ADS12) || defined (__HEW_IDE__) || defined (__linux__)  || defined(PLATFORM_LINUX)

    #define __BB_CPU_TYPE__ __BB_CPU32__
    #define __BB_MEMORY_TYPE__ __BB_MEM8__
	#define __BB_MEMSPACE_TYPE__ __BB_ADD32__
#endif /* WIN32 */

/* PalmOS 68k code */

#ifdef PALMOS68K

    #define __BB_CPU_TYPE__ __BB_CPU16__
    #define __BB_MEMORY_TYPE__ __BB_MEM8__
	#define __BB_MEMSPACE_TYPE__ __BB_ADD32__
#endif /* PALMOS5 */

#ifdef TI_C54X
    #define __BB_CPU_TYPE__ __BB_CPU16__
    #define __BB_MEMORY_TYPE__ __BB_MEM16__
	#define __BB_MEMSPACE_TYPE__ __BB_ADD16__
#endif /* TI_C54X */

#ifdef TI_C55X
    #define __BB_CPU_TYPE__ __BB_CPU16__
    #define __BB_MEMORY_TYPE__ __BB_MEM16__
	#define __BB_MEMSPACE_TYPE__ __BB_ADD24__
#endif /* TI_C54X */


/**********************************************************************/
/*   GENERIC CONSTANTS                                                */
/**********************************************************************/
#define BB_OK           0x0000
#define BB_NOK          0xFFFF


/**********************************************************************/
/*   BITMASK CONSTANTS                                                */
/**********************************************************************/
#define BB_BIT0       (1<<0)
#define BB_BIT1       (1<<1)
#define BB_BIT2       (1<<2)
#define BB_BIT3       (1<<3)
#define BB_BIT4       (1<<4)
#define BB_BIT5       (1<<5)
#define BB_BIT6       (1<<6)
#define BB_BIT7       (1<<7)
#define BB_BIT8       (1<<8)
#define BB_BIT9       (1<<9)
#define BB_BIT10      (1<<10)
#define BB_BIT11      (1<<11) 
#define BB_BIT12      (1<<12)
#define BB_BIT13      (1<<13)
#define BB_BIT14      (1<<14)
#define BB_BIT15      (1<<15)
#define BB_BIT16      (1<<16)
#define BB_BIT17      (1<<17)
#define BB_BIT18      (1<<18)
#define BB_BIT19      (1<<19)
#define BB_BIT20      (1<<20)
#define BB_BIT21      (1<<21)
#define BB_BIT22      (1<<22)
#define BB_BIT23      (1<<23)
#define BB_BIT24      (1<<24)
#define BB_BIT25      (1<<25)
#define BB_BIT26      (1<<26)
#define BB_BIT27      (1<<27)
#define BB_BIT28      (1<<28)
#define BB_BIT29      (1<<29)
#define BB_BIT30      (1<<30)
#define BB_BIT31      (1<<31)


/**********************************************************************/
/*   DATABASE FORMAT CONSTANTS                                        */
/**********************************************************************/

/* Alignment */
#define BB_ALIGN8     BB_BIT0   /* 8-bits boundary alignement */
#define BB_ALIGN16    BB_BIT1   /* 16-bits boundary alignement */
#define BB_ALIGN32    BB_BIT2   /* 32-bits boundary alignement */
#define BB_ALIGN64    BB_BIT3   /* 64-bits boundary alignement */
#define __BB_ALIGN__(X, A) (((X)+((A)-1))&(~((A)-1)))

/* MAU : Minimum Allocatable Unit */
#define BB_MAU8       BB_BIT0   /* 8-bits width MAU */
#define BB_MAU16      BB_BIT1   /* 16-bits width MAU */
#define BB_MAU32      BB_BIT2   /* 32-bits width MAU */
#define BB_MAU16C8    BB_BIT3   /* 16-bits width MAU compacted as 8-bits virtual addressing */
#define BB_MAU32C8    BB_BIT4   /* 32-bits width MAU compacted as 8-bits virtual addressing */
#define BB_MAU32C16   BB_BIT5   /* 32-bits width MAU compacted as 16-bits virtual addressing */

/* Endianess */
  /* 8-bits value :         0xaa */
  /* 16-bits value :      0xbbaa */
  /* 32-bits value :  0xddccbbaa */
#define BB_LITENDIAN    BB_BIT0 /* aa ; aa-bb ; aa-bb-cc-dd */
#define BB_MIXENDIAN    BB_BIT1 /* aa ; bbaa  ; bbaa-ddcc */
#define BB_BIGENDIAN    BB_BIT2 /* aa ; bb-aa ; dd-cc-bb-aa */


/* Tracing macro's */


#ifdef BB_LOG_ON
/*#ifdef TI_C54X*/
/*  extern FILE* my_log;*/
  #define LOG_OUTPUT(a, b) fprintf(my_log, a, b)
  #define LOG_OUTPUT2(a, b, c) fprintf(my_log, a, b, c)
/*#else*/
/*  #define LOG_OUTPUT(a, b) printf(a, b)*/
/*#endif*/ /* TI_C54X */
#else /* BB_LOG_ON */
  #define LOG_OUTPUT(a, b)
  #define LOG_OUTPUT2(a, b, c)
#endif /* BB_LOG_ON */
/*
#if (BB_TRACE_LEVEL == ALL)
  #define BB_TRACE_LEVEL 5
#endif*/ /*(BB_TRACE_LEVEL == ALL)*/

/* TRACE LEVEL 0 */
#if (BB_TRACE_LEVEL >= 0)
  #define BB_TRACE_L0_IN(message) LOG_OUTPUT("> %s \n",message)
  #define BB_TRACE_L0_OUT(message) LOG_OUTPUT("< %s \n",message)
#else 
  #define BB_TRACE_L0_IN(message)
  #define BB_TRACE_L0_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 0)*/

/* TRACE LEVEL 1 */
#if (BB_TRACE_LEVEL >= 1)
  #define BB_TRACE_L1_IN(message) LOG_OUTPUT(" > %s \n",message)
  #define BB_TRACE_L1_OUT(message) LOG_OUTPUT(" < %s \n",message)
#else 
  #define BB_TRACE_L1_IN(message)
  #define BB_TRACE_L1_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 1)*/

/* TRACE LEVEL 2 */
#if (BB_TRACE_LEVEL >= 2)
  #define BB_TRACE_L2_IN(message) LOG_OUTPUT("  > %s \n",message)
  #define BB_TRACE_L2_OUT(message) LOG_OUTPUT("  < %s \n",message)
#else 
  #define BB_TRACE_L2_IN(message)
  #define BB_TRACE_L2_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 2)*/

/* TRACE LEVEL 3 */
#if (BB_TRACE_LEVEL >= 3)
  #define BB_TRACE_L3_IN(message) LOG_OUTPUT("   > %s \n",message)
  #define BB_TRACE_L3_OUT(message) LOG_OUTPUT("   < %s \n",message)
#else 
  #define BB_TRACE_L3_IN(message)
  #define BB_TRACE_L3_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 3)*/

/* TRACE LEVEL 4 */
#if (BB_TRACE_LEVEL >= 4)
  #define BB_TRACE_L4_IN(message) LOG_OUTPUT("    > %s \n",message)
  #define BB_TRACE_L4_OUT(message) LOG_OUTPUT("    < %s \n",message)
#else 
  #define BB_TRACE_L4_IN(message)
  #define BB_TRACE_L4_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 4)*/

/* TRACE LEVEL 5 */
#if (BB_TRACE_LEVEL >= 5)
  #define BB_TRACE_L5_IN(message) LOG_OUTPUT("     > %s \n",message)
  #define BB_TRACE_L5_OUT(message) LOG_OUTPUT("     < %s \n",message)
#else 
  #define BB_TRACE_L5_IN(message)
  #define BB_TRACE_L5_OUT(message)
#endif /*(BB_TRACE_LEVEL >= 5)*/

#define BB_RETURN(a)    LOG_OUTPUT(" # %d\n", ((BB_S32)a));
#define BB_LOG_PARAM(a,b)    LOG_OUTPUT2(" :Param: %s %d\n", a, b)


/**********************************************************************/
/*   BIT MANIPULATION MACROS                                          */
/**********************************************************************/

/* Generate a number in which the nth 'bit' is set, all others are clear. */
#define BB_GENBITMASK(bit) (1u << (bit))

/* Check whether the given number masked using the bit-mask is not 0. */
#define BB_MATCHBITMASK(num, mask) (0 != ((num) & (mask)))

/* Check whether the nth 'bit' is set in 'num'. */
#define BB_ISBITSET(num, bit) BB_MATCHBITMASK(num, BB_GENBITMASK(bit))


/**********************************************************************/
/*   GENERAL HELPER MACROS                                            */
/**********************************************************************/

/* BB_GLUE(token1, token2)
   
   Glue two tokens together, performing full macro expansion on `token1` and 
   `token2` (which the simple use of the `##` preprocessor operator doesn't 
   perform). */
#define _BB_GLUE_HELPER(token1, token2)  token1##token2
#define BB_GLUE(token1, token2)          _BB_GLUE_HELPER(token1, token2)


/* BB_STRINGIFY(token)
   
   Transform a token into a string literal which is the textual representation 
   of that token, performing full macro expansion on `token` (which the simple 
   use of the `#` preprocessor operator doesn't perform). It's normalized by 
   the preprocessor by reducing consecutive whitespace to a single space and 
   stripping leading and trailing whitespace.
   
   Note: This macro also works recursively. */
#define _BB_STRINGIFY_HELPER(token)   #token
#define BB_STRINGIFY(token)           _BB_STRINGIFY_HELPER(token)
#define _BB_STRINGIFYL_HELPER(token)  BB_GLUE(L, #token)
#define BB_STRINGIFYL(token)          _BB_STRINGIFYL_HELPER(tok)


/* BB_WIDEN(str)
   
   Prefix a (narrow) string literal (or character literal) with the "L" 
   prefix, which indicates to the compiler it's a wide string literal (or wide 
   character literal).
   
   Example:
   
   "Hello, world!" (or 'a')
   
   becomes:
   
   L"Hello, world!" (or L'a') */
#define BB_WIDEN(str)  BB_GLUE(L, str)

/* Legacy equivalent of `BB_WIDEN()`. */
#define BB_LIFY(str)   BB_WIDEN(str)


/* BB_NUMELMINARRAY(array)
   
   Tell how many elements (slots) a fixed one-dimensional array has. 
   **NOT SUITABLE** for pointers nor arrays decayed as pointers.
   
   For example, an array declared this way:
   
   const BB_NS32 array1[6] = { 687, -9, 10, 21, 33, 5678 };
   
   the expression `BB_NUMELMINARRAY(array1)` would yield 6. another example:
   
   const BB_NF32 array2[] = { 1.5f, 3.1415f, 0.5f };
	
	the expression `BB_NUMELMINARRAY(array2)` would yield 3.
	
	Note: This macro accepts to compute the size of a pointer in some cases... 
	it's unsafe but there's no way to safe-guard against this. Static analysis 
	of the source code might help here. For more info, look here:
	
	<http://stackoverflow.com/questions/4064134/arraysize-c-macro-how-does-it-work>
	
	*/
/*#define BB_NUMELMINARRAY(array) (sizeof(array) / sizeof((array)[0]))*/
#if defined(BB_CPLUSPLUS)
	#define BB_NUMELMINARRAY(array) \
	  ((sizeof(array) / sizeof(*(array))) / static_cast<size_t>(!(sizeof(array) % sizeof(*(array)))))
#else
	#define BB_NUMELMINARRAY(array) \
	  ((sizeof(array) / sizeof(*(array))) / ((size_t) (!(sizeof(array) % sizeof(*(array))))))
#endif


/* BB_BREAK()
   
   Triggers a breakpoint in "Debug" builds, does nothing in "Release" builds.
   
   If under a debugger, the debugger will catch it at the specific location 
   it was placed, easing the debugging.
   
   If not under a debugger, the code will lead to a "crash" of the program 
   (SIGTRAP on Linux, unhandled (structured) exception on Win32). */
#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	#if (defined(WIN32) || defined(_WIN32)) && defined(_MSC_VER) && !defined(__clang__)
		#if defined(BB_CPLUSPLUS)
			extern "C" __declspec(dllimport) void __stdcall DebugBreak();
		#else
			__declspec(dllimport) void __stdcall DebugBreak(void);
		#endif
		
		#define BB_BREAK() \
			DebugBreak();
	#elif defined(__GNUC__)
		/* Use of inline assembly. Breakpoints are implemented as a special 
		   software interrupt initially set up by the CPU. The binary encoding 
		   of this opcode (it takes no parameter) is specifically designed to 
		   fit on one byte: 0xcc. */
		#define BB_BREAK() __asm__("int $3");
	#else
		#define BB_BREAK() abort();
	#endif
#else
	#define BB_BREAK()
#endif

#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	/* Helper (implementation) macro for `BB_ASSERT_IMPL_()`. */
	#define BB_ASSERT_IMPL_PRINTMSG_(condStr, file, line) \
		{ \
			static char ASSERT_FAIL_buffer_[160u]; \
			\
			setvbuf( \
				stderr, ASSERT_FAIL_buffer_, _IONBF, \
				sizeof(ASSERT_FAIL_buffer_) \
			); \
			fprintf( \
				stderr, "*** ASSERTION FAILURE: `%s` is false in file %s at " \
				"line %u\n", (condStr), (file), (line) \
			); \
		}
	
	/* Helper (implementation) macro for `BB_ASSERT_IMPL_()`. */
	#if ((defined(WIN32) || defined(_WIN32)) && !defined(_CONSOLE) && defined(_INC_WINDOWS))
		#define BB_ASSERT_IMPL_MSGBOX_(condStr, file, line) \
			{ \
				static char ASSERT_FAIL_buffer_[160u]; \
				\
				_snprintf( \
					ASSERT_FAIL_buffer_, sizeof(ASSERT_FAIL_buffer_), "`%s` is " \
					"false in file %s at line %u\n", \
					(condStr), (file), (line) \
				); \
				MessageBoxA( \
					0u, ASSERT_FAIL_buffer_, "ASSERTION FAILURE", ( \
						MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_TASKMODAL | \
						MB_SETFOREGROUND \
					) \
				); \
			}
	#else
		#define BB_ASSERT_IMPL_MSGBOX_(condStr, file, line)
	#endif
	
	/* Helper (implementation) macro for `BB_ASSERT()` and `BB_ASSERT_IF()`. */
	#define BB_ASSERT_IMPL_(cond, file, line) \
		if (!(cond)) \
			{ \
				BB_ASSERT_IMPL_PRINTMSG_(#cond, (file), (line)); \
				BB_ASSERT_IMPL_MSGBOX_(#cond, (file), (line)); \
				BB_BREAK(); \
			}
#endif


/* BB_ASSERT(cond)
   
   In "Debug" builds, check whether an assertion (expressed through 
   `cond`) is satisfied. If it isn't, print a message on the standard 
   error output stream using a big enough static buffer (to avoid internal 
   automatic dynamic memory allocation which could lead to tricky 
   situations) and trigger a breakpoint.
   
   In "Release" builds, it's substituted by emptiness, effectively dropping it 
   from such builds. */
#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	#define BB_ASSERT(cond) \
		BB_ASSERT_IMPL_((cond), __FILE__, __LINE__);
#else
	#define BB_ASSERT(cond)
#endif


/* BB_ASSERT_IF(pred, cond)
   
   In "Debug" builds, do the same as `BB_ASSERT(cond)`, the assertion check 
   through `cond` if the predicate `pred` is true. If it isn't, don't check 
   anything. 
   
   In "Release" builds, it's substituted by emptiness, effectively dropping it 
   from such builds. */
#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	#define BB_ASSERT_IF(pred, cond) \
		if (pred) \
		{ \
			BB_ASSERT_IMPL_((cond), __FILE__, __LINE__); \
		}
#else
	#define BB_ASSERT_IF(pred, cond)
#endif

#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	/* Helper (implementation) macro for `BB_UNREACHABLE()`. */
	#define BB_UNREACHABLE_PRINTMSG_(file, line) \
		{ \
			static char UNREACH_POINT_buffer_[160u]; \
			\
			setvbuf( \
				stderr, UNREACH_POINT_buffer_, _IONBF, \
				sizeof(UNREACH_POINT_buffer_) \
			); \
			fprintf( \
				stderr, "*** UNREACHABLE POINT: Reached an unreachable point in " \
				"file %s at line %u\n", (file), (line) \
			); \
			fflush(stderr); \
		}
	
	/* Helper (implementation) macro for `BB_ASSERT_IMPL_()`. */
	#if ((defined(WIN32) || defined(_WIN32)) && !defined(_CONSOLE) && defined(_INC_WINDOWS))
		#define BB_UNREACHABLE_MSGBOX_(file, line) \
			{ \
				static char UNREACH_POINT_buffer_[160u]; \
				\
				_snprintf( \
					UNREACH_POINT_buffer_, sizeof(UNREACH_POINT_buffer_), \
					"Reached an unreachable point in file %s at line %u\n", \
					(file), (line) \
				); \
				MessageBoxA( \
					0u, UNREACH_POINT_buffer_, "UNREACHABLE POINT", ( \
						MB_OK | MB_ICONERROR | MB_DEFBUTTON1 | MB_TASKMODAL | \
						MB_SETFOREGROUND \
					) \
				); \
			}
	#else
		#define BB_UNREACHABLE_MSGBOX_(file, line)
	#endif
#endif


/* BB_UNREACHABLE()
   
   In "Debug" builds, print a message on the standard error output stream 
   using a big enough static buffer (to avoid internal automatic dynamic 
   memory allocation which could lead to tricky situations) and trigger a 
   breakpoint.
   
   In "Release" builds, it's substituted by emptiness, effectively dropping it 
   from such builds. */
#if defined(_DEBUG) || defined(BB_CFG_DEBUG) || defined(BB_CFG_INTERNALRELEASE)
	#define BB_UNREACHABLE() \
		BB_UNREACHABLE_PRINTMSG_(__FILE__, __LINE__); \
		BB_UNREACHABLE_MSGBOX_(__FILE__, __LINE__); \
		BB_BREAK();
	
#else
	#define BB_UNREACHABLE()
#endif


/* "Block for unused things", which is need to circumvent syntactical 
   limitations of C and C++.
   
   It declares a static file-scope function with a gibberish name (based 
   on "dummy_body_block_unused__") in which everything in the "block" is 
   placed.
   
   Additionally, it also "declares" the such-generated pseudo-function as 
   unused (because recursion is syntactically allowed, which is abused as an 
   advantageous side effect for this trick). */
#if defined(__GNUC__)
	#define BB_UNUSED_FUNC_(name)  (void) name;
#elif defined(_MSC_VER)
	#define BB_UNUSED_FUNC_(name)  (void) *((char *) (&(name)));
#endif
	
#define BB_UNUSED_FUNC(name) \
	static void dummy_body_block_unused__##name##_() \
	{ \
		BB_UNUSED_FUNC_(dummy_body_block_unused__##name##_); \
		BB_UNUSED_FUNC_(name); \
	}

/* Cast to void the given expression on it's own statement (and block, but 
   the latter is not required though) which effectively results in doing 
   nothing, except stop the compiler from whining about an unused expression 
   while also marking it as being unused on purpose (for humans). */
#define BB_UNUSED_VAR(var)      ((void) (var))
#define BB_UNUSED_EXPRRES(expr) ((void) (expr))
#define BB_UNUSED_PARM(name)    ((void) (name))
#define BB_UNUSED_RETVAL(expr)  ((void) (expr))

/* Aliases of the macros hereabove for source code compatibility purposes. */
#define BB_UNUSED(expr)           BB_UNUSED_EXPRRES(expr)
#define BB_UNUSED_FUNCTION(name)  BB_UNUSED_FUNC(name)
#define BB_UNUSED_PARAMETER(name) BB_UNUSED_PARM(name)
#define BB_UNUSED_RETVALUE(expr)  BB_UNUSED_RETVAL(expr)
#define BB_UNUSED_RETV(expr)      BB_UNUSED_RETVAL(expr)


/* Source file location string. */
#define BB_SRCFILELOCSTR()  __FILE__  "("  BB_STRINGIFY(__LINE__)  ")"
#define BB_SRCFILELOCSTRL() __FILE__ L"(" BB_STRINGIFYL(__LINE__) L")"


/* BB_PP_COUNTER()
   
   C/C++ preprocessor counter. Incremented each time it's used, effectively 
   giving unique values/IDs. */
#define BB_PP_COUNTER() __COUNTER__


/* BB_STATIC_ASSERT(condition, message)
   
   Static assertion test: not perfect (cryptic error message in case of 
   failure) but better than nothing.
   
   Implementation note: The (unused) typedef is created in an anonymous scope.
   
   Note: `message` isn't directly used but is there mainly for forward 
   compatibility in case such a feature is later used. Anyway, it's indirectly 
   useful, once the failing assertion test is located in the source code, as 
   it may give a human-readable context about why it failed. */
#define BB_STATIC_ASSERT(condition, message) \
	{ \
		typedef char BB_STATIC_ASSERTION_TEST_FAILED__ line_ ## __LINE__ ## __ ## BB_PP_COUNTER() [ \
			(condition) ? 1 : -1 \
		]; \
	}


/* BB_SAFE_CONST_EXPR(expr)
   
   Turns the constant expression `expr` into an constant expression construct 
   such that the compiler won't complaint about it.
   
   It's safe to be used **only** when it really is a constant 
   expression used in a specific way (usually in some special construct). 
   
   !!! PLEASE USE PRECAUCIOUSLY AND WITH PARCIMONY  !!!
   
   !!! PLEASE USE ONLY IF YOU KNOW WHAT YOU'RE DOING WITH THIS THING  !!!
   
   */
#define BB_SAFE_CONST_EXPR(expr) \
	(BB_UNUSED_EXPRRES(expr), expr)


/* BB_ALWAYS_TRUE and BB_ALWAYS_FALSE
   
   `BB_ALWAYS_FALSE` is the same as just using plain `0` or `false` (C99, C++) 
   except for it doesn't generate any warning about the test being a constant 
   expression when used, for example, inside tests for `if`, `while`, `for` or 
   `do`-`while` statements.
   
   `BB_ALWAYS_TRUE` is pretty much the same as `BB_ALWAYS_FALSE` except it's 
   for `1` or `true` (C99, C++) instead.
   
   Note: Both cast-to-void and the infamous comma operator are abused in this 
   practical exercise for syntax bending.
   
   Typical use cases:
   
   * For `BB_ALWAYS_FALSE`: the usual C/C++ pattern 
     
     do
     {
        ...decls, expressions and statements...
     } while (0);` 
     
     which is usually used for benefitting from the `break` for branching out 
     of the block without using a real variable or loop (which would consume 
     run-time resources if not properly optimized out by the compiler), may 
     instead be written:
     
     do
     {
        ...decls, expressions and statements...
     } while (BB_ALWAYS_FALSE);` 
     
     This way, the compiler won't complain the a constant expression is used 
     for the test in the `while` statement. There's no possible alternate 
     syntax for this construction with the same effect, so this is a **MUST**.
   
   * For `BB_ALWAYS_TRUE`, it's probably much less useful as the typical use 
     would most likely be:
     
     while (BB_ALWAYS_TRUE)
     {
        ...decls, expressions and statements...
     }
     
     or:
     
     for (; BB_ALWAYS_TRUE; )
     {
        ...decls, expressions and statements...
     }
     
     but both constructions can be replaced with this syntactically correct 
     construction for the same effect/purpose (the spaces between the commas 
     isn't mandatory):
     
     for (; ; )
     {
        ...decls, expressions and statements...
     }
     */
#define BB_ALWAYS_FALSE BB_SAFE_CONST_EXPR(0)
#define BB_ALWAYS_TRUE  BB_SAFE_CONST_EXPR(1)


/* `BB_TRY`, `BB_CATCH(exc)` and `BB_CATCH_ALL`
   
   If `BB_NOCXXEXCEPT` is defined, these three macros are neutralized. 
   Otherwise, they map to `try`, `catch(exc)` and `catch(...)`, 
   respectively. */
#if defined(BB_CPLUSPLUS) && !defined(BB_NOCXXEXCEPT)
	#define BB_TRY                   try
	#define _BB_CATCH_IMPL(body)     body
	#define BB_CATCH(exc)            catch(exc) _BB_CATCH_IMPL
	#define BB_CATCH_ALL             catch(...) _BB_CATCH_IMPL
	#define BB_THROW(expr)           throw expr
	#define BB_RETHROW               throw
#else		
	#define BB_TRY                   
	#define _BB_CATCH_IMPL(body)     
	#define BB_CATCH(arg)            _BB_CATCH_IMPL
	#define BB_CATCH_ALL             _BB_CATCH_IMPL
	#define BB_THROW(expr)           
	#define BB_RETHROW               
#endif

/* BB_MIN() */
#define BB_MIN(a, b) \
	(((a) < (b)) ? (a) : (b))


/* BB_MAX() */
#define BB_MAX(a, b) \
	(((a) > (b)) ? (a) : (b))

#endif /* __COMMON__BB_DEF_H__ */
