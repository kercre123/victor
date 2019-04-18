/*
 * BABEL TECHNOLOGIES
 *    Speech & Image Solutions
 *
 * File name:           bb_types.h
 * File description :   BABEL GENERIC TYPES DEFINITIONS
 * Module :             COMMON
 * File location :      .\common\inc\
 * Purpose:             This header is included into each BABEL project
 *                      through "defines.h"
 *                      It contains the BABEL types definition
 * Author:              NM
 * History : 
 *            21/06/2001    Created     NM
 *            03/09/2001    Update      NM
 *
 *
 * Copyright (c) 1998-2001 BABEL TECHNOLOGIES
 * All rights reserved.
 * PERMISSION IS HEREBY DENIED TO USE, COPY, MODIFY, OR DISTRIBUTE THIS
 * SOFTWARE OR ITS DOCUMENTATION FOR ANY PURPOSE WITHOUT WRITTEN
 * AGREEMENT OR ROYALTY FEES.
 *
 */


#ifndef __COMMON__BB_TYPES_H__
#define __COMMON__BB_TYPES_H__

/*
 *
 * Memory dependent INTEGER types
 * ===============================
 * BB_U8     : Unsigned 8-bit value
 * BB_S8     : Signed 8-bit value
 * BB_U16    : Unsigned 16-bit value
 * BB_S16    : Signed 16-bit value
 * BB_U32    : Unsigned 32-bit value
 * BB_S32    : Signed 32-bit value
 * BB_U64    : Unsigned 64-bit value
 * BB_S64    : Signed 64-bit value
 *
 * CPU dependent INTEGER types
 * ============================
 * BB_NU8    : Native type if able to contain the BB_U8 range else,  BB_U8 
 * BB_NS8    : Native type if able to contain the BB_S8 range else,  BB_S8 
 * BB_NU16   : Native type if able to contain the BB_U16 range else, BB_U16
 * BB_NS16   : Native type if able to contain the BB_S16 range else, BB_S16
 * BB_NU32   : Native type if able to contain the BB_U32 range else, BB_U32
 * BB_NS32   : Native type if able to contain the BB_S32 range else, BB_S32
 *
 * FLOAT types
 * ============
 * BB_F32
 *
 * BOOL types
 * ===========
 * BB_BOOL
 *
 * CHAR types
 * ===========
 * BB_TCHAR
 *
 * ERROR types
 * ============
 * BB_ERROR
 *
 * VERSION types
 * ==============
 * BB_VERSION
 *
 * DATABASE FORMAT types
 * =====================
 * BB_DBFORMAT
 *
 * ALIGNMENT
 * =========
 * BB_ALIGN
 */

/* Memory type is CPU dependant */
/**********************************************************************/
/*   MEMORY DEPENDENT INTEGER DEFINITION                              */
/**********************************************************************/
/*#if (__BB_MEMORY_TYPE__ == __BB_MEM8__)*/
/*    #error __BB_MEM8__ undefined __BB_MEMORY_TYPE__ */
    /*typedef  signed char		BB_S8;*/
    /*typedef  unsigned char		BB_U8;*/
    /*typedef  short				BB_S16;*/
    /*typedef  unsigned short		BB_U16;*/
    /*typedef  long				BB_S32;*/
    /*typedef  unsigned long		BB_U32;*/
	/*typedef	 long long			BB_S64;*/
	/*typedef  unsigned long long	BB_U64;*/
/*#elif (__BB_MEMORY_TYPE__ == __BB_MEM16__)*/
/*    #error __BB_MEM16__ undefined __BB_MEMORY_TYPE__ */
    /*typedef  signed char        BB_S8;*/
    /*typedef  unsigned char		BB_U8;*/
    /*typedef  short				BB_S16;*/
    /*typedef  unsigned short		BB_U16;*/
    /*typedef  long				BB_S32;*/
    /*typedef  unsigned long		BB_U32;*/
	/*typedef	 long long			BB_S64;*/
	/*typedef  unsigned long long	BB_U64;*/
/*#elif (__BB_MEMORY_TYPE__ == __BB_MEM32__)*/
    /*#error __BB_MEM32__ undefined __BB_MEMORY_TYPE__ */
/*    typedef  signed char           BB_S8;*/
/*    typedef  unsigned char  BB_U8;*/
/*    typedef  short          BB_S16;*/
/*    typedef  unsigned short BB_U16;*/
/*    typedef  int            BB_S32;*/
/*    typedef  unsigned int   BB_U32;*/
/*    typedef  float          BB_F32;*/
    /*#error __BB_MEMORY_TYPE__ undefined (__BB_MEM8__/__BB_MEM16__/__BB_MEM32__)*/
/*#endif (__BB_MEMORY_TYPE__ == __BB_MEMXX__) */


/**********************************************************************/
/*   CPU DEPENDENT INTEGER DEFINITION                                 */
/**********************************************************************/
#if (__BB_CPU_TYPE__ == __BB_CPU8__)
    #error __BB_CPU8__ undefined __BB_CPU_TYPE__ 
/*    typedef  int            BB_NS8;*/
/*    typedef  int            BB_NU8;*/
/*    typedef  short          BB_NS16;*/
/*    typedef  int            BB_NU16;*/
/*    typedef  long           BB_NS32;*/
/*    typedef  unsigned long  BB_NU32;*/
/*    typedef  float          BB_NF32;*/
#elif (__BB_CPU_TYPE__ == __BB_CPU16__)
/*    #error __BB_CPU16__ undefined __BB_CPU_TYPE__ */
	typedef  int            BB_NS8;
	typedef  unsigned int	BB_NU8;
	typedef  int            BB_NS16;
	typedef  unsigned int   BB_NU16;
	typedef  long           BB_NS32;
	typedef  unsigned long  BB_NU32;
	typedef  float          BB_NF32;
	#define BB_ALIGN(X)     __BB_ALIGN__((X),BB_ALIGN16)

#elif (__BB_CPU_TYPE__ == __BB_CPU32__)
	/*========================================================================
	  ========================================================================
	  ==                                                                    ==
	  ==                             32-bit CPU                             ==
	  ==                                                                    ==
	  ========================================================================
	  ========================================================================*/
	
	typedef long int          BB_NS8;
	typedef long unsigned int BB_NU8;
	typedef long int          BB_NS16;
	typedef long unsigned int BB_NU16;
	typedef long int          BB_NS32;
	typedef long unsigned int BB_NU32;
	/* --- */
	typedef long unsigned int BB_PTR;
	typedef long int          BB_SPTR;
	typedef BB_PTR            BB_UPTR;
	/* --- */
	typedef BB_UPTR	        BB_DbOffsetPtr; /* Pointer offset */
	
	#define __BB_NS8_DEFINED__
	#define __BB_SPTR_DEFINED__
	
	/* Corresponding printf() format string specifiers */
	#define _BB_NS8_PWTHSPEC          "l"
	#define _BB_NS8_PSGNSPEC          "d"
	#define _BB_NU8_PWTHSPEC          "l"
	#define _BB_NU8_PSGNSPEC          "u"
	#define _BB_NS16_PWTHSPEC         "l"
	#define _BB_NS16_PSGNSPEC         "d"
	#define _BB_NU16_PWTHSPEC         "l"
	#define _BB_NU16_PSGNSPEC         "u"
	#define _BB_NS32_PWTHSPEC         "l"
	#define _BB_NS32_PSGNSPEC         "d"
	#define _BB_NU32_PWTHSPEC         "l"
	#define _BB_NU32_PSGNSPEC         "u"
	/* --- */
	#define _BB_PTR_PWTHSPEC          "l"
	#define _BB_PTR_PSGNSPEC          "u"
	#define _BB_SPTR_PWTHSPEC         "l"
	#define _BB_SPTR_PSGNSPEC         "d"
	#define _BB_UPTR_PWTHSPEC         "l"
	#define _BB_UPTR_PSGNSPEC         "u"
	/* --- */
	#define _BB_DbOffsetPtr_PWTHSPEC  _BB_UPTR_PWTHSPEC
	#define _BB_DbOffsetPtr_PSGNSPEC  _BB_UPTR_PSGNSPEC
	/* --- */
	#define _BB_SIZEOF_PWTHSPEC       "l"
	#define _BB_SIZEOF_PSGNSPEC       "u"
	/* --- */
	#define _BB_ENUM_PWTHSPEC         ""
	#define _BB_ENUM_PSGNSPEC         "d"
	/* --- */
	#if defined(WIN32) || defined(_WIN32)
		#define _DWORD_PTR_PWTHSPEC    "l"
		#define _DWORD_PTR_PSGNSPEC    "u"
	#else
		#define _DWORD_PTR_PWTHSPEC    ""
		#define _DWORD_PTR_PSGNSPEC    "u"
	#endif
	/* --- */
	#if defined(__linux__) || defined(__MINGW32__)
		#define _SSIZE_T_PWTHSPEC         ""
		#define _SSIZE_T_PSGNSPEC         "d"
	#else
		#define _SSIZE_T_PWTHSPEC         "l"
		#define _SSIZE_T_PSGNSPEC         "d"
	#endif
	/* --- */
	#if defined(__linux__) || defined(__MINGW32__)
		#define _SIZE_T_PWTHSPEC          ""
		#define _SIZE_T_PSGNSPEC          "u"
	#else
		#define _SIZE_T_PWTHSPEC          "l"
		#define _SIZE_T_PSGNSPEC          "u"
	#endif
	
	/* Corresponding constant-definition helper macros */
	#define BB_NS8_C(cst)          cst##l
	#define BB_NU8_C(cst)          cst##ul
	#define BB_NS16_C(cst)         cst##l
	#define BB_NU16_C(cst)         cst##ul
	#define BB_NS32_C(cst)         cst##l
	#define BB_NU32_C(cst)         cst##ul
	/* --- */
	#define BB_PTR_C(cst)          cst##ul
	#define BB_SPTR_C(cst)         cst##l
	#define BB_UPTR_C(cst)         cst##ul
	/* --- */
	#define BB_DbOffsetPtr_C(cst)  BB_UPTR_C(cst)
	
	/* Corresponding number-of-bits macros */
	#define BB_NS8_BITS          BB_U32_BITS
	#define BB_NU8_BITS          BB_U32_BITS
	#define BB_NS16_BITS         BB_U32_BITS
	#define BB_NU16_BITS         BB_U32_BITS
	#define BB_NS32_BITS         BB_U32_BITS
	#define BB_NU32_BITS         BB_U32_BITS
	/* --- */
	#define BB_PTR_BITS          BB_U32_BITS
	#define BB_SPTR_BITS         BB_U32_BITS
	#define BB_UPTR_BITS         BB_U32_BITS
	/* --- */
	#define BB_DbOffsetPtr_BITS  BB_UPTR_BITS
	/* --- */
	#define BB_SIZEOF_BITS BB_U32_BITS
	
	#define BB_ALIGN(X)  __BB_ALIGN__((X),BB_ALIGN32)
	
	/**********************************************************************/
	/*   MEMORY DEPENDENT INTEGER DEFINITION                              */
	/**********************************************************************/
	#if (__BB_MEMORY_TYPE__ == __BB_MEM8__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                        8-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		typedef signed char               BB_S8;
		typedef unsigned char             BB_U8;
		typedef short                     BB_S16;
		typedef unsigned short            BB_U16;
		typedef long int                  BB_S32;
		typedef long unsigned int         BB_U32;
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
			typedef __int64		          BB_S64;
			typedef unsigned __int64       BB_U64;
			#define __BB_S64_DEFINED__
		#elif !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
			typedef long long int          BB_S64;
			typedef long long unsigned int BB_U64;
			#define __BB_S64_DEFINED__
		#endif
		/* --- */
		typedef BB_S32                    BB_DbOffset32; /* Offset for 32-bit based computation*/
		
		#define __BB_S8_DEFINED__
		
		/* Corresponding printf() format string specifiers */
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
			#define _BB_S8_PWTHSPEC       ""
			#define _BB_S8_PSGNSPEC       "d"
			#define _BB_U8_PWTHSPEC       ""
			#define _BB_U8_PSGNSPEC       "u"
		#else
			#define _BB_S8_PWTHSPEC       "hh"
			#define _BB_S8_PSGNSPEC       "d"
			#define _BB_U8_PWTHSPEC       "hh"
			#define _BB_U8_PSGNSPEC       "u"
		#endif
		#define _BB_S16_PWTHSPEC         "h"
		#define _BB_S16_PSGNSPEC         "d"
		#define _BB_U16_PWTHSPEC         "h"
		#define _BB_U16_PSGNSPEC         "u"
		#define _BB_S32_PWTHSPEC         "l"
		#define _BB_S32_PSGNSPEC         "d"
		#define _BB_U32_PWTHSPEC         "l"
		#define _BB_U32_PSGNSPEC         "u"
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
			#define _BB_S64_PWTHSPEC      "I64"
			#define _BB_S64_PSGNSPEC      "d"
			#define _BB_U64_PWTHSPEC      "I64"
			#define _BB_U64_PSGNSPEC      "u"
		#elif !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
			#define _BB_S64_PWTHSPEC      "ll"
			#define _BB_S64_PSGNSPEC      "d"
			#define _BB_U64_PWTHSPEC      "ll"
			#define _BB_U64_PSGNSPEC      "u"
		#endif
		/* --- */
		#define _BB_DbOffset32_PWTHSPEC  _BB_S32_PWTHSPEC
		#define _BB_DbOffset32_PSGNSPEC  _BB_S32_PSGNSPEC
		
		/* Corresponding constant-definition helper macros */
		#define BB_S8_C(cst)          cst
		#define BB_U8_C(cst)          cst
		#define BB_S16_C(cst)         cst
		#define BB_U16_C(cst)         cst
		#define BB_S32_C(cst)         cst##l
		#define BB_U32_C(cst)         cst##ul
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
			#define BB_S64_C(cst)      cst##i64
			#define BB_U64_C(cst)      cst##i64u
		#elif !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
			#define BB_S64_C(cst)      cst##ll
			#define BB_U64_C(cst)      cst##ull
		#endif
		/* --- */
		#define BB_DbOffset32_C(cst)  BB_S32_C(cst)
		
		/* Corresponding number-of-bits macros */
		#define BB_S8_BITS          8
		#define BB_U8_BITS          8
		#define BB_S16_BITS         16
		#define BB_U16_BITS         16
		#define BB_S32_BITS         32
		#define BB_U32_BITS         32
		#define BB_S64_BITS         64
		#define BB_U64_BITS         64
		/* --- */
		#define BB_DbOffset32_BITS  BB_S32_BITS
		
	#elif (__BB_MEMORY_TYPE__ == __BB_MEM16__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                       16-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		
		#error __BB_MEM16__: bb_types need to be CHECKED and UPDATED !
		
		
		/*typedef  signed char        BB_S8;
		typedef  unsigned char		BB_U8;*/
		typedef  short				BB_S16;
		typedef  unsigned short		BB_U16;
		typedef  long				BB_S32;
		typedef  unsigned long		BB_U32;
#if !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
		typedef	 long long			BB_S64;
		typedef  unsigned long long	BB_U64;
#endif
		
		#define __BB_S8_DEFINED__
#if !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
		#define __BB_S64_DEFINED__
#endif
		
	#elif (__BB_MEMORY_TYPE__ == __BB_MEM32__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                       32-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		
		#error __BB_MEM32__ undefined __BB_MEMORY_TYPE__ 
		
		
/*    typedef  signed char           BB_S8;*/
/*    typedef  unsigned char  BB_U8;*/
/*    typedef  short          BB_S16;*/
/*    typedef  unsigned short BB_U16;*/
/*    typedef  int            BB_S32;*/
/*    typedef  unsigned int   BB_U32;*/
/*    typedef  float          BB_F32;*/
		#error __BB_MEMORY_TYPE__ undefined (__BB_MEM8__/__BB_MEM16__/__BB_MEM32__)
	#endif /* (__BB_MEMORY_TYPE__ == __BB_MEMXX__) */
#elif (__BB_CPU_TYPE__ == __BB_CPU64__)
	/*========================================================================
	  ========================================================================
	  ==                                                                    ==
	  ==                             64-bit CPU                             ==
	  ==                                                                    ==
	  ========================================================================
	  ========================================================================*/
	
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
		typedef __int64                BB_NS8;
		typedef unsigned __int64       BB_NU8;
		typedef __int64                BB_NS16;
		typedef unsigned __int64       BB_NU16;
		typedef __int64                BB_NS32;
		typedef unsigned __int64       BB_NU32;
	#else
		typedef long long int          BB_NS8;
		typedef long long unsigned int BB_NU8;
		typedef long long int          BB_NS16;
		typedef long long unsigned int BB_NU16;
		typedef long long int          BB_NS32;
		typedef long long unsigned int BB_NU32;
	#endif
	/* --- */
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
		typedef unsigned __int64       BB_PTR;
		typedef __int64                BB_SPTR;
	#else
		typedef long long unsigned int BB_PTR;
		typedef long long int          BB_SPTR;
	#endif
	typedef BB_PTR                    BB_UPTR;
	/* --- */
	typedef BB_UPTR	                BB_DbOffsetPtr; /* Pointer offset */
	
	#define __BB_NS8_DEFINED__
	#define __BB_SPTR_DEFINED__
	
	/* Corresponding printf() format string specifiers */
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
		#define _BB_NS8_PWTHSPEC          "I64"
		#define _BB_NS8_PSGNSPEC          "d"
		#define _BB_NU8_PWTHSPEC          "I64"
		#define _BB_NU8_PSGNSPEC          "u"
		#define _BB_NS16_PWTHSPEC         "I64"
		#define _BB_NS16_PSGNSPEC         "d"
		#define _BB_NU16_PWTHSPEC         "I64"
		#define _BB_NU16_PSGNSPEC         "u"
		#define _BB_NS32_PWTHSPEC         "I64"
		#define _BB_NS32_PSGNSPEC         "d"
		#define _BB_NU32_PWTHSPEC         "I64"
		#define _BB_NU32_PSGNSPEC         "u"
	#else
		#define _BB_NS8_PWTHSPEC          "ll"
		#define _BB_NS8_PSGNSPEC          "d"
		#define _BB_NU8_PWTHSPEC          "ll"
		#define _BB_NU8_PSGNSPEC          "u"
		#define _BB_NS16_PWTHSPEC         "ll"
		#define _BB_NS16_PSGNSPEC         "d"
		#define _BB_NU16_PWTHSPEC         "ll"
		#define _BB_NU16_PSGNSPEC         "u"
		#define _BB_NS32_PWTHSPEC         "ll"
		#define _BB_NS32_PSGNSPEC         "d"
		#define _BB_NU32_PWTHSPEC         "ll"
		#define _BB_NU32_PSGNSPEC         "u"
	#endif
	/* --- */
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
		#define _BB_PTR_PWTHSPEC          "I64"
		#define _BB_PTR_PSGNSPEC          "u"
		#define _BB_SPTR_PWTHSPEC         "I64"
		#define _BB_SPTR_PSGNSPEC         "d"
		#define _BB_UPTR_PWTHSPEC         "I64"
		#define _BB_UPTR_PSGNSPEC         "u"
	#else
		#define _BB_PTR_PWTHSPEC          "ll"
		#define _BB_PTR_PSGNSPEC          "u"
		#define _BB_SPTR_PWTHSPEC         "ll"
		#define _BB_SPTR_PSGNSPEC         "d"
		#define _BB_UPTR_PWTHSPEC         "ll"
		#define _BB_UPTR_PSGNSPEC         "u"
	#endif
	/* --- */
	#define _BB_DbOffsetPtr_PWTHSPEC  _BB_UPTR_PWTHSPEC
	#define _BB_DbOffsetPtr_PSGNSPEC  _BB_UPTR_PSGNSPEC
	/* --- */
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
		#define _BB_SIZEOF_PWTHSPEC       "I64"
		#define _BB_SIZEOF_PSGNSPEC       "u"
	#else
		#define _BB_SIZEOF_PWTHSPEC       "l"
		#define _BB_SIZEOF_PSGNSPEC       "u"
	#endif
	/* --- */
	#define _BB_ENUM_PWTHSPEC         ""
	#define _BB_ENUM_PSGNSPEC         "d"
	/* --- */
	#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
		#define _DWORD_PTR_PWTHSPEC    "I64"
		#define _DWORD_PTR_PSGNSPEC    "u"
	/*#elif (defined(WIN32) || defined(_WIN32)) && defined(__clang__)
		#define _DWORD_PTR_PWTHSPEC    "ll"
		#define _DWORD_PTR_PSGNSPEC    "u"
	#else
		#define _DWORD_PTR_PWTHSPEC    "l"
		#define _DWORD_PTR_PSGNSPEC    "u"*/
	#elif defined(__LP64__) || defined(_LP64)
		#define _DWORD_PTR_PWTHSPEC    "l"
		#define _DWORD_PTR_PSGNSPEC    "u"
	#else
		#define _DWORD_PTR_PWTHSPEC    "ll"
		#define _DWORD_PTR_PSGNSPEC    "u"
	#endif
	/* --- */
	#if defined(__MINGW32__)
		#define _SSIZE_T_PWTHSPEC         "l"
		#define _SSIZE_T_PSGNSPEC         "d"
	#else
		#define _SSIZE_T_PWTHSPEC         "l"
		#define _SSIZE_T_PSGNSPEC         "d"
	#endif
	/* --- */
	#if defined(__MINGW32__)
		#define _SIZE_T_PWTHSPEC          "ll"
		#define _SIZE_T_PSGNSPEC          "u"
	#else
		#define _SIZE_T_PWTHSPEC          "l"
		#define _SIZE_T_PSGNSPEC          "u"
	#endif
	
	/* Corresponding constant-definition helper macros */
	#define BB_NS8_C(cst)          cst##ll
	#define BB_NU8_C(cst)          cst##ull
	#define BB_NS16_C(cst)         cst##ll
	#define BB_NU16_C(cst)         cst##ull
	#define BB_NS32_C(cst)         cst##ll
	#define BB_NU32_C(cst)         cst##ull
	/* --- */
	#define BB_PTR_C(cst)          cst##ull
	#define BB_SPTR_C(cst)         cst##ll
	#define BB_UPTR_C(cst)         cst##ull
	/* --- */
	#define BB_DbOffsetPtr_C(cst)  BB_UPTR_C(cst)
	
	/* Corresponding number-of-bits macros */
	#define BB_NS8_BITS          BB_U64_BITS
	#define BB_NU8_BITS          BB_U64_BITS
	#define BB_NS16_BITS         BB_U64_BITS
	#define BB_NU16_BITS         BB_U64_BITS
	#define BB_NS32_BITS         BB_U64_BITS
	#define BB_NU32_BITS         BB_U64_BITS
	/* --- */
	#define BB_PTR_BITS          BB_U64_BITS
	#define BB_SPTR_BITS         BB_U64_BITS
	#define BB_UPTR_BITS         BB_U64_BITS
	/* --- */
	#define BB_DbOffsetPtr_BITS  BB_UPTR_BITS
	/* --- */
	#define BB_SIZEOF_BITS       BB_U32_BITS
	
	#define BB_ALIGN(X)  __BB_ALIGN__((X),BB_ALIGN64)
	
	/**********************************************************************/
	/*   MEMORY DEPENDENT INTEGER DEFINITION                              */
	/**********************************************************************/
	#if (__BB_MEMORY_TYPE__ == __BB_MEM8__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                        8-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		typedef signed char               BB_S8;
		typedef unsigned char             BB_U8;
		typedef short                     BB_S16;
		typedef unsigned short            BB_U16;
		#if defined(WIN32) || defined(_WIN32)
			typedef long int               BB_S32;
			typedef long unsigned int      BB_U32;
		#else
			typedef int                    BB_S32;
			typedef unsigned int           BB_U32;
		#endif
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
			typedef __int64                BB_S64;
			typedef unsigned __int64       BB_U64;
		#else
			typedef long long int          BB_S64;
			typedef long long unsigned int BB_U64;
		#endif
		/* --- */
		typedef BB_S32                    BB_DbOffset32; /* Offset for 32-bit based computation*/
		
		#define __BB_S8_DEFINED__
		#define __BB_S64_DEFINED__
		
		/* Corresponding printf() format string specifiers */
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
			#define _BB_S8_PWTHSPEC       ""
			#define _BB_S8_PSGNSPEC       "d"
			#define _BB_U8_PWTHSPEC       ""
			#define _BB_U8_PSGNSPEC       "u"
		#else
			#define _BB_S8_PWTHSPEC       "hh"
			#define _BB_S8_PSGNSPEC       "d"
			#define _BB_U8_PWTHSPEC       "hh"
			#define _BB_U8_PSGNSPEC       "u"
		#endif
		#define _BB_S16_PWTHSPEC         "h"
		#define _BB_S16_PSGNSPEC         "d"
		#define _BB_U16_PWTHSPEC         "h"
		#define _BB_U16_PSGNSPEC         "u"
		#if defined(WIN32) || defined(_WIN32)
			#define _BB_S32_PWTHSPEC      "l"
			#define _BB_S32_PSGNSPEC      "d"
			#define _BB_U32_PWTHSPEC      "l"
			#define _BB_U32_PSGNSPEC      "u"
		#else
			#define _BB_S32_PWTHSPEC      ""
			#define _BB_S32_PSGNSPEC      "d"
			#define _BB_U32_PWTHSPEC      ""
			#define _BB_U32_PSGNSPEC      "u"
		#endif
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) /*|| defined(__MINGW32__)*/)
			#define _BB_S64_PWTHSPEC      "I64"
			#define _BB_S64_PSGNSPEC      "d"
			#define _BB_U64_PWTHSPEC      "I64"
			#define _BB_U64_PSGNSPEC      "u"
		#else
			#define _BB_S64_PWTHSPEC      "ll"
			#define _BB_S64_PSGNSPEC      "d"
			#define _BB_U64_PWTHSPEC      "ll"
			#define _BB_U64_PSGNSPEC      "u"
		#endif
		/* --- */
		#define _BB_DbOffset32_PWTHSPEC  _BB_S32_PWTHSPEC
		#define _BB_DbOffset32_PSGNSPEC  _BB_S32_PSGNSPEC
		
		/* Corresponding constant-definition helper macros */
		#define BB_S8_C(cst)          cst
		#define BB_U8_C(cst)          cst##u
		#define BB_S16_C(cst)         cst
		#define BB_U16_C(cst)         cst##u
		#if defined(_MSC_VER)
			#define BB_S32_C(cst)      cst##l
			#define BB_U32_C(cst)      cst##ul
		#else
			#define BB_S32_C(cst)      cst
			#define BB_U32_C(cst)      cst##u
		#endif
		#if (defined(WIN32) || defined(_WIN32)) && ((defined(_MSC_VER) && !defined(__clang__)) || defined(__MINGW32__))
			#define BB_S64_C(cst)         cst##i64
			#define BB_U64_C(cst)         cst##i64u
		#else
			#define BB_S64_C(cst)         cst##ll
			#define BB_U64_C(cst)         cst##ull
		#endif
		/* --- */
		#define BB_DbOffset32_C(cst)  BB_S32_C(cst)
		
		/* Corresponding number-of-bits macros */
		#define BB_S8_BITS          8
		#define BB_U8_BITS          8
		#define BB_S16_BITS         16
		#define BB_U16_BITS         16
		#define BB_S32_BITS         32
		#define BB_U32_BITS         32
		#define BB_S64_BITS         64
		#define BB_U64_BITS         64
		/* --- */
		#define BB_DbOffset32_BITS  BB_S32_BITS
		
	#elif (__BB_MEMORY_TYPE__ == __BB_MEM16__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                       16-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		
		#error __BB_MEM16__: bb_types need to be CHECKED and UPDATED !
		
		
		/*typedef  signed char        BB_S8;*/
		/*typedef  unsigned char		BB_U8;*/
		typedef  short				BB_S16;
		typedef  unsigned short		BB_U16;
		typedef  long				BB_S32;
		typedef  unsigned long		BB_U32;
		typedef	 long long			BB_S64;
		typedef  unsigned long long	BB_U64;
		typedef BB_S32 BB_DbOffset32; /* Offset for 32-bit based computation*/
		
		#define __BB_S64_DEFINED__
		
	#elif (__BB_MEMORY_TYPE__ == __BB_MEM32__)
		/*---------------------------------------------------------------------
		  ---------------------------------------------------------------------
		  --                                                                 --
		  --                       32-bit wide memory                        --
		  --                                                                 --
		  ---------------------------------------------------------------------
		  ---------------------------------------------------------------------*/
		
		
		#error __BB_MEM32__ undefined __BB_MEMORY_TYPE__ 
		
		
/*    typedef  signed char           BB_S8;*/
/*    typedef  unsigned char  BB_U8;*/
/*    typedef  short          BB_S16;*/
/*    typedef  unsigned short BB_U16;*/
/*    typedef  int            BB_S32;*/
/*    typedef  unsigned int   BB_U32;*/
/*    typedef  float          BB_F32;*/
		#error __BB_MEMORY_TYPE__ undefined (__BB_MEM8__/__BB_MEM16__/__BB_MEM32__)
	#endif /* (__BB_MEMORY_TYPE__ == __BB_MEMXX__) */
#else 
    #error __BB_CPU_TYPE__ undefined (__BB_CPU8__/__BB_CPU16__/__BB_CPU32__/__BB_CPU64__)
#endif /* (__BB_CPU_TYPE__ == __BB_CPUXX__) */


/**********************************************************************/
/*   ADDRESS SPACE DEPENDENT DEFINITION                              */
/**********************************************************************/
#if (__BB_MEMSPACE_TYPE__ == __BB_ADD16__)
    typedef  BB_U16			BB_H;
#elif (__BB_MEMSPACE_TYPE__ == __BB_ADD24__)
    typedef  BB_U32			BB_H;
#elif (__BB_MEMSPACE_TYPE__ == __BB_ADD32__)
    typedef  BB_U32			BB_H;
#elif (__BB_MEMSPACE_TYPE__ == __BB_ADD64__)
    typedef  BB_U64			BB_H;
#else 
    #error __BB_MEMSPACE_TYPE__ undefined (__BB_ADD16__/__BB_ADD24__/__BB_ADD32__/__BB_ADD64__)
#endif


/**********************************************************************/
/*   FLOAT DEFINITION                                                 */
/**********************************************************************/
typedef float  BB_F32;
typedef double BB_F64;

#define _BB_F32_PSPEC "f"
#define _BB_F64_PSPEC "f"


/**********************************************************************/
/*   CHARACTER DEFINITION                                             */
/**********************************************************************/
/* Later to be a specific module */
#if !defined(BB_TCHAR_USE_NATIVE_CHAR)
typedef signed char BB_TCHAR;
#else
/* Define `BB_TCHAR_USE_NATIVE_CHAR` for compilation as well as 
   "-fsigned-char" (GCC/Clang; VS: default) or "-funsigned-char" (GCC/Clang; 
	VS: "/J") as most appropriate for your code.
   
   By default, `char` has the same signedness as:
   
   * `signed char` on VS
   * `unsigned char` on GCC/Clang
   
   Still... `char` is **NOT** considered by the compiler as the same type as 
   either `signed char` ("-fsigned-char") or `unsigned char` 
	("-funsigned-char"; "/J"), it's considered as a third, separate type which 
	equals neither. */
typedef char BB_TCHAR;
#endif

#define _BB_TCHAR_PSPEC "c"
#define BB_TCHAR_C(cst) ((const BB_TCHAR) (cst))


/**********************************************************************/
/*   CHARACTER STRING DEFINITION                                      */
/**********************************************************************/

#define _BB_TSTR_PSPEC "s"
#define BB_TSTR_C(cst) ((const BB_TCHAR *) (cst))


/* MUST INCLUDE UNICODE / MBCS (MultiByte Character Set) STATEMENT */


/**********************************************************************/
/*   TYPE-SPECIFIC printf() ARGUMENT ON-SPECIFICATION CONVERSION      */
/**********************************************************************/

#if defined(__BB_S8_DEFINED__)
	#define BB_S8_PARG(v) ((const BB_S8) (v))
	#define BB_U8_PARG(v) ((const BB_U8) (v))
#endif
#define BB_S16_PARG(v) ((const BB_S16) (v))
#define BB_U16_PARG(v) ((const BB_U16) (v))
#define BB_S32_PARG(v) ((const BB_S32) (v))
#define BB_U32_PARG(v) ((const BB_U32) (v))
#if defined(__BB_S64_DEFINED__)
	#define BB_S64_PARG(v) ((const BB_S64) (v))
	#define BB_U64_PARG(v) ((const BB_U64) (v))
#endif
#define BB_F32_PARG(v) ((const BB_F32) (v))
#define BB_F64_PARG(v) ((const BB_F64) (v))

#if defined(__BB_NS8_DEFINED__)
	#define BB_NS8_PARG(v) ((const BB_NS8) (v))
	#define BB_NU8_PARG(v) ((const BB_NU8) (v))
#endif
#define BB_NS16_PARG(v) ((const BB_S16) (v))
#define BB_NU16_PARG(v) ((const BB_U16) (v))
#define BB_NS32_PARG(v) ((const BB_NS32) (v))
#define BB_NU32_PARG(v) ((const BB_NU32) (v))

#define BB_F32_PARG(v) ((const BB_F32) (v))
#define BB_F64_PARG(v) ((const BB_F64) (v))

#if defined(__BB_NF32_DEFINED__)
	#define BB_NF32_PARG(v) ((const BB_NF32) (v))
#endif

#define BB_TCHAR_PARG(v) ((const BB_TCHAR) (v))
#define BB_TSTR_PARG(v) ((const BB_TCHAR *) (v))

#define BB_PTR_PARG(v) ((const BB_PTR) (v))
#define BB_SPTR_PARG(v) ((const BB_SPTR) (v))
#define BB_UPTR_PARG(v) ((const BB_UPTR) (v))

#define BB_NPTR_PARG(v) BB_UPTR_PARG(v)


/**********************************************************************/
/*   ERROR DEFINITION                                                 */
/**********************************************************************/

/* error type definition (to be later placed into an error module)    */
typedef BB_NS32 BB_ERROR;

#if defined(_BB_NS32_PWTHSPEC)
	#define _BB_ERROR_PWTHSPEC _BB_NS32_PWTHSPEC
	#define _BB_ERROR_PSGNSPEC _BB_NS32_PSGNSPEC
	
	#define BB_ERROR_C(cst) BB_NS32_C(cst)
#endif

/* Error must include the module / submodule, the severity of         */
/*  the error and the error type                                      */
/* The sign-bit is the severity one                                   */
/*  -> if value of error is <0 : severity level is high               */
/*                                                                    */
/* :3:3 2 2 2 2 2 2 2 2 2:2 1 1 1 1 1 1 1 1 1:1 0 0 0 0 0 0 0 0 0 0:  */
/* :1:0 9 8 7 6 5 4 3 2 1:0 9 8 7 6 5 4 3 2 1:0 9 8 7 6 5 4 3 2 1 0:  */
/* : :                   :                   :                     :  */
/* :S:                   :                   :                     :  */
/* :E:                   :  S                :                     :  */
/* :V:   M               :   U               :   E                 :  */
/* :E:    O              :    B              :    R                :  */
/* :R:     D             :     M             :     R               :  */
/* :I:      U            :      O            :      O              :  */
/* :T:       L           :       D           :       R             :  */
/* :Y:        E          :        U          :                     :  */
/* : :                   :         L         :         I           :  */
/* :L:          I        :          E        :          D          :  */
/* :E:           D       :                   :                     :  */
/* :V:                   :            I      :                     :  */
/* :E:                   :             D     :                     :  */
/* :L:                   :                   :                     :  */
/*                                                                    */
/*  SEVERITY LEVEL :                                                  */
/*     - 0    :  LOW      (WARNING LEVEL)                             */
/*                The application can continue its process            */
/*                without risking a crash of the system               */
/*     - 1    :  HIGH     (ERROR LEVEL)                               */
/*                The application needs to correct the problem        */
/*                or the system will crash (if not yet done)          */
/*                                                                    */
/*  MODULE ID      :                                                  */
/*    ID of the module where the error occured                        */
/*                                                                    */
/*  SUBMODULE ID   :                                                  */
/*    ID of the submodule where the error occured                     */
/*                                                                    */
/*  ERROR ID       :                                                  */
/*    Error description                                               */
/*                                                                    */
/*                                                                    */

#define BB_ERRORSEVERITYMASK  (BB_ERROR)0x80000000  /*  1 bit  (bit 31)     */
#define BB_ERRORMODIDMASK     (BB_ERROR)0x7fe00000  /* 10 bits (bits 30-21) */
#define BB_ERRORSUBMODIDMASK  (BB_ERROR)0x001ff800  /* 10 bits (bits 20-11) */
#define BB_ERRORIDMASK        (BB_ERROR)0x000007ff  /* 11 bits (bits 10-00) */

#define BB_ERRORSEVERITYSHIFT 30
#define BB_ERRORMODIDSHIFT    20
#define BB_ERRORSUBMODIDSHIFT 10
#define BB_ERRORIDMASKSHIFT    0

/* Severity Level */
#define BB_ERRORSEVERITYHIGH  1
#define BB_ERRORSEVERITYLOW   0

/* Module ID */

/* Submodule ID */

/* Error ID */
#define BB_


/**********************************************************************/
/*   ERROR HANDLING FUNCTIONS                                         */
/**********************************************************************/

/* BB_createError */
#define BB_createError(severity, moduleID, subModuleID, errorID)  (BB_ERROR) (\
          (severity<<BB_ERRORSEVERITYSHIFT) | \
          (moduleID<<BB_ERRORSEVERITYSHIFT) | \
          (subModuleID<<BB_ERRORSEVERITYSHIFT) | \
          (errorID<<BB_ERRORMODIDSHIFT)

#define BB_SUCC( error ) ( ( (BB_ERROR)( error ) < 0x80000000 ) ? BB_TRUE : BB_FALSE )
#define BB_WARN( error ) ( ( ((BB_ERROR)( error ) < 0x80000000) && ((BB_ERROR)( error ) != 0)) ? BB_TRUE : BB_FALSE )



/**********************************************************************/
/*   MODULES DEFINITION                                               */
/**********************************************************************/
/* 10 bits are used to define the Module Description */
/*  9 8 7 6 5 4 3 2 1 0                   */
/* - technology (ASR / TTS / )            */
/* - Group (Embedded / PCMM / TELECOM / ) */
/* - FixPoint / Floating Point            */
/* -                                      */

/* Bit 9 is used to specify whether the module is in fixed-point (1) or floating-point (0) */
/*  Calculation mode : fixed-point or floating-point */
#define BB_MODULEIDFIXEDPOINT       (1 << 9)
#define BB_MODULEIDFLOATING         (0 << 9)

/* Bit 8-6 describes the group */
#define BB_MODULEIDEMBEDDED         (0 << 6)
#define BB_MODULEIDPCMM             (1 << 6)
#define BB_MODULEIDTELECOM          (2 << 6)
#define BB_MODULEIDGROUP4           (3 << 6)
#define BB_MODULEIDGROUP5           (4 << 6)
#define BB_MODULEIDGROUP6           (5 << 6)
#define BB_MODULEIDGROUP7           (6 << 6)
#define BB_MODULEIDGROUP8           (7 << 6)

/* Bit 5-3 describes the technology */
#define BB_MODULEIDTECH0            (0 << 3)
#define BB_MODULEIDTECH1            (1 << 3)
#define BB_MODULEIDTECH2            (2 << 3)
#define BB_MODULEIDTECH3            (3 << 3)
#define BB_MODULEIDTECH4            (4 << 3)
#define BB_MODULEIDTECH5            (5 << 3)
#define BB_MODULEIDTECH6            (6 << 3)
#define BB_MODULEIDTECH7            (7 << 3)

/* Bit 2-0 describes the platform */
#define BB_MODULEIDWIN32            (0 << 0)
#define BB_MODULEIDWINCE            (1 << 0)
#define BB_MODULEIDLINUX            (2 << 0)
#define BB_MODULEIDPLATFORM3        (3 << 0)
#define BB_MODULEIDPLATFORM4        (4 << 0)
#define BB_MODULEIDPLATFORM5        (5 << 0)
#define BB_MODULEIDPLATFORM6        (6 << 0)
#define BB_MODULEIDCUSTOM           (7 << 0)

#define BB_createModuleID(modeID, groupID, technoID, platformID) \
         (modeID + groupID + technoID + platformID)


/**********************************************************************/
/*   TYPE-SPECIFIC printf() SPECIFIERS AND PARAMETRIC SPECIFIERS      */
/**********************************************************************/

/* Cast for the argument being the width part of a printf()-based specifier. */
#define BB_WIDTH_PARG(v) ((int) (v))

/* Cast for the argument being the precision part of a printf()-based 
   specifier. */
#define BB_PREC_PARG(v) ((int) (v))

/* BB_S8 */
#if defined(_BB_S8_PWTHSPEC)
	#if !defined(_BB_S8_PSGNSPEC)
		#error _BB_S8_PWTHSPEC is defined but _BB_S8_PSGNSPEC is not
	#endif
	
	#define BB_S8_PFMT             "%"                    _BB_S8_PWTHSPEC           _BB_S8_PSGNSPEC
	#define BB_S8_PFMTX(parm)      "%"     parm           _BB_S8_PWTHSPEC           _BB_S8_PSGNSPEC
	#define BB_S8_PFMTL           L"%"            BB_LIFY(_BB_S8_PWTHSPEC)  BB_LIFY(_BB_S8_PSGNSPEC)
	#define BB_S8_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_S8_PWTHSPEC)  BB_LIFY(_BB_S8_PSGNSPEC)
	#define BB_S8_PFMTH            "%"                    _BB_S8_PWTHSPEC           "x"
	#define BB_S8_PFMTHX(parm)     "%"     parm           _BB_S8_PWTHSPEC           "x"
	#define BB_S8_PFMTHL          L"%"            BB_LIFY(_BB_S8_PWTHSPEC)         L"x"
	#define BB_S8_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_S8_PWTHSPEC)         L"x"
	
	#define BB_S8_PFMTZ          Z("%")                 Z(_BB_S8_PWTHSPEC)        Z(_BB_S8_PSGNSPEC)
	#define BB_S8_PFMTXZ(parm)   Z("%")    parm         Z(_BB_S8_PWTHSPEC)        Z(_BB_S8_PSGNSPEC)
	#define BB_S8_PFMTHZ         Z("%")                 Z(_BB_S8_PWTHSPEC)        Z("x")
	#define BB_S8_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_S8_PWTHSPEC)        Z("x")
#endif

/* BB_U8 */
#if defined(_BB_U8_PWTHSPEC)
	#if !defined(_BB_U8_PSGNSPEC)
		#error _BB_U8_PWTHSPEC is defined but _BB_U8_PSGNSPEC is not
	#endif
	
	#define BB_U8_PFMT             "%"                    _BB_U8_PWTHSPEC           _BB_U8_PSGNSPEC
	#define BB_U8_PFMTX(parm)      "%"     parm           _BB_U8_PWTHSPEC           _BB_U8_PSGNSPEC
	#define BB_U8_PFMTL           L"%"            BB_LIFY(_BB_U8_PWTHSPEC)  BB_LIFY(_BB_U8_PSGNSPEC)
	#define BB_U8_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_U8_PWTHSPEC)  BB_LIFY(_BB_U8_PSGNSPEC)
	#define BB_U8_PFMTH            "%"                    _BB_U8_PWTHSPEC           "x"
	#define BB_U8_PFMTHX(parm)     "%"     parm           _BB_U8_PWTHSPEC           "x"
	#define BB_U8_PFMTHL          L"%"            BB_LIFY(_BB_U8_PWTHSPEC)         L"x"
	#define BB_U8_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_U8_PWTHSPEC)         L"x"
	
	#define BB_U8_PFMTZ          Z("%")                 Z(_BB_U8_PWTHSPEC)        Z(_BB_U8_PSGNSPEC)
	#define BB_U8_PFMTXZ(parm)   Z("%")    parm         Z(_BB_U8_PWTHSPEC)        Z(_BB_U8_PSGNSPEC)
	#define BB_U8_PFMTHZ         Z("%")                 Z(_BB_U8_PWTHSPEC)        Z("x")
	#define BB_U8_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_U8_PWTHSPEC)        Z("x")
#endif

/* BB_S16 */
#if defined(_BB_S16_PWTHSPEC)
	#if !defined(_BB_S16_PSGNSPEC)
		#error _BB_S16_PWTHSPEC is defined but _BB_S16_PSGNSPEC is not
	#endif
	
	#define BB_S16_PFMT             "%"                    _BB_S16_PWTHSPEC           _BB_S16_PSGNSPEC
	#define BB_S16_PFMTX(parm)      "%"     parm           _BB_S16_PWTHSPEC           _BB_S16_PSGNSPEC
	#define BB_S16_PFMTL           L"%"            BB_LIFY(_BB_S16_PWTHSPEC)  BB_LIFY(_BB_S16_PSGNSPEC)
	#define BB_S16_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_S16_PWTHSPEC)  BB_LIFY(_BB_S16_PSGNSPEC)
	#define BB_S16_PFMTH            "%"                    _BB_S16_PWTHSPEC           "x"
	#define BB_S16_PFMTHX(parm)     "%"     parm           _BB_S16_PWTHSPEC           "x"
	#define BB_S16_PFMTHL          L"%"            BB_LIFY(_BB_S16_PWTHSPEC)         L"x"
	#define BB_S16_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_S16_PWTHSPEC)         L"x"
	
	#define BB_S16_PFMTZ          Z("%")                 Z(_BB_S16_PWTHSPEC)        Z(_BB_S16_PSGNSPEC)
	#define BB_S16_PFMTXZ(parm)   Z("%")    parm         Z(_BB_S16_PWTHSPEC)        Z(_BB_S16_PSGNSPEC)
	#define BB_S16_PFMTHZ         Z("%")                 Z(_BB_S16_PWTHSPEC)        Z("x")
	#define BB_S16_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_S16_PWTHSPEC)        Z("x")
#endif

/* BB_U16 */
#if defined(_BB_U16_PWTHSPEC)
	#if !defined(_BB_U16_PSGNSPEC)
		#error _BB_U16_PWTHSPEC is defined but _BB_U16_PSGNSPEC is not
	#endif
	
	#define BB_U16_PFMT             "%"                    _BB_U16_PWTHSPEC           _BB_U16_PSGNSPEC
	#define BB_U16_PFMTX(parm)      "%"     parm           _BB_U16_PWTHSPEC           _BB_U16_PSGNSPEC
	#define BB_U16_PFMTL           L"%"            BB_LIFY(_BB_U16_PWTHSPEC)  BB_LIFY(_BB_U16_PSGNSPEC)
	#define BB_U16_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_U16_PWTHSPEC)  BB_LIFY(_BB_U16_PSGNSPEC)
	#define BB_U16_PFMTH            "%"                    _BB_U16_PWTHSPEC           "x"
	#define BB_U16_PFMTHX(parm)     "%"     parm           _BB_U16_PWTHSPEC           "x"
	#define BB_U16_PFMTHL          L"%"            BB_LIFY(_BB_U16_PWTHSPEC)         L"x"
	#define BB_U16_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_U16_PWTHSPEC)         L"x"
	
	#define BB_U16_PFMTZ          Z("%")                 Z(_BB_U16_PWTHSPEC)        Z(_BB_U16_PSGNSPEC)
	#define BB_U16_PFMTXZ(parm)   Z("%")    parm         Z(_BB_U16_PWTHSPEC)        Z(_BB_U16_PSGNSPEC)
	#define BB_U16_PFMTHZ         Z("%")                 Z(_BB_U16_PWTHSPEC)        Z("x")
	#define BB_U16_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_U16_PWTHSPEC)        Z("x")
#endif

/* BB_S32 */
#if defined(_BB_S32_PWTHSPEC)
	#if !defined(_BB_S32_PSGNSPEC)
		#error _BB_S32_PWTHSPEC is defined but _BB_S32_PSGNSPEC is not
	#endif
	
	#define BB_S32_PFMT             "%"                    _BB_S32_PWTHSPEC           _BB_S32_PSGNSPEC
	#define BB_S32_PFMTX(parm)      "%"     parm           _BB_S32_PWTHSPEC           _BB_S32_PSGNSPEC
	#define BB_S32_PFMTL           L"%"            BB_LIFY(_BB_S32_PWTHSPEC)  BB_LIFY(_BB_S32_PSGNSPEC)
	#define BB_S32_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_S32_PWTHSPEC)  BB_LIFY(_BB_S32_PSGNSPEC)
	#define BB_S32_PFMTH            "%"                    _BB_S32_PWTHSPEC           "x"
	#define BB_S32_PFMTHX(parm)     "%"     parm           _BB_S32_PWTHSPEC           "x"
	#define BB_S32_PFMTHL          L"%"            BB_LIFY(_BB_S32_PWTHSPEC)         L"x"
	#define BB_S32_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_S32_PWTHSPEC)         L"x"
	
	#define BB_S32_PFMTZ          Z("%")                 Z(_BB_S32_PWTHSPEC)        Z(_BB_S32_PSGNSPEC)
	#define BB_S32_PFMTXZ(parm)   Z("%")    parm         Z(_BB_S32_PWTHSPEC)        Z(_BB_S32_PSGNSPEC)
	#define BB_S32_PFMTHZ         Z("%")                 Z(_BB_S32_PWTHSPEC)        Z("x")
	#define BB_S32_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_S32_PWTHSPEC)        Z("x")
#endif

/* BB_U32 */
#if defined(_BB_U32_PWTHSPEC)
	#if !defined(_BB_U32_PSGNSPEC)
		#error _BB_U32_PWTHSPEC is defined but _BB_U32_PSGNSPEC is not
	#endif
	
	#define BB_U32_PFMT             "%"                    _BB_U32_PWTHSPEC          _BB_U32_PSGNSPEC
	#define BB_U32_PFMTX(parm)      "%"     parm           _BB_U32_PWTHSPEC          _BB_U32_PSGNSPEC
	#define BB_U32_PFMTL           L"%"            BB_LIFY(_BB_U32_PWTHSPEC)  BB_LIFY(_BB_U32_PSGNSPEC)
	#define BB_U32_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_U32_PWTHSPEC)  BB_LIFY(_BB_U32_PSGNSPEC)
	#define BB_U32_PFMTH            "%"                    _BB_U32_PWTHSPEC          "x"
	#define BB_U32_PFMTHX(parm)     "%"     parm           _BB_U32_PWTHSPEC          "x"
	#define BB_U32_PFMTHL          L"%"            BB_LIFY(_BB_U32_PWTHSPEC)        L"x"
	#define BB_U32_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_U32_PWTHSPEC)        L"x"
	
	#define BB_U32_PFMTZ          Z("%")                 Z(_BB_U32_PWTHSPEC)        Z(_BB_U32_PSGNSPEC)
	#define BB_U32_PFMTXZ(parm)   Z("%")    parm         Z(_BB_U32_PWTHSPEC)        Z(_BB_U32_PSGNSPEC)
	#define BB_U32_PFMTHZ         Z("%")                 Z(_BB_U32_PWTHSPEC)        Z("x")
	#define BB_U32_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_U32_PWTHSPEC)        Z("x")
#endif

/* BB_S64 */
#if defined(_BB_S64_PWTHSPEC)
	#if !defined(_BB_S64_PSGNSPEC)
		#error _BB_S64_PWTHSPEC is defined but _BB_S64_PSGNSPEC is not
	#endif
	
	#define BB_S64_PFMT             "%"                    _BB_S64_PWTHSPEC           _BB_S64_PSGNSPEC
	#define BB_S64_PFMTX(parm)      "%"     parm           _BB_S64_PWTHSPEC           _BB_S64_PSGNSPEC
	#define BB_S64_PFMTL           L"%"            BB_LIFY(_BB_S64_PWTHSPEC)  BB_LIFY(_BB_S64_PSGNSPEC)
	#define BB_S64_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_S64_PWTHSPEC)  BB_LIFY(_BB_S64_PSGNSPEC)
	#define BB_S64_PFMTH            "%"                    _BB_S64_PWTHSPEC           "x"
	#define BB_S64_PFMTHX(parm)     "%"     parm           _BB_S64_PWTHSPEC           "x"
	#define BB_S64_PFMTHL          L"%"            BB_LIFY(_BB_S64_PWTHSPEC)         L"x"
	#define BB_S64_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_S64_PWTHSPEC)         L"x"
	
	#define BB_S64_PFMTZ          Z("%")                 Z(_BB_S64_PWTHSPEC)        Z(_BB_S64_PSGNSPEC)
	#define BB_S64_PFMTXZ(parm)   Z("%")    parm         Z(_BB_S64_PWTHSPEC)        Z(_BB_S64_PSGNSPEC)
	#define BB_S64_PFMTHZ         Z("%")                 Z(_BB_S64_PWTHSPEC)        Z("x")
	#define BB_S64_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_S64_PWTHSPEC)        Z("x")
#endif

/* BB_U64 */
#if defined(_BB_U64_PWTHSPEC)
	#if !defined(_BB_U64_PSGNSPEC)
		#error _BB_U64_PWTHSPEC is defined but _BB_U64_PSGNSPEC is not
	#endif
	
	#define BB_U64_PFMT             "%"                   _BB_U64_PWTHSPEC           _BB_U64_PSGNSPEC
	#define BB_U64_PFMTX(parm)      "%"     parm          _BB_U64_PWTHSPEC           _BB_U64_PSGNSPEC
	#define BB_U64_PFMTL           L"%"           BB_LIFY(_BB_U64_PWTHSPEC)  BB_LIFY(_BB_U64_PSGNSPEC)
	#define BB_U64_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_U64_PWTHSPEC)  BB_LIFY(_BB_U64_PSGNSPEC)
	#define BB_U64_PFMTH            "%"                   _BB_U64_PWTHSPEC           "x"
	#define BB_U64_PFMTHX(parm)     "%"     parm          _BB_U64_PWTHSPEC           "x"
	#define BB_U64_PFMTHL          L"%"           BB_LIFY(_BB_U64_PWTHSPEC)         L"x"
	#define BB_U64_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_U64_PWTHSPEC)         L"x"
	
	#define BB_U64_PFMTZ          Z("%")                Z(_BB_U64_PWTHSPEC)        Z(_BB_U64_PSGNSPEC)
	#define BB_U64_PFMTXZ(parm)   Z("%")    parm        Z(_BB_U64_PWTHSPEC)        Z(_BB_U64_PSGNSPEC)
	#define BB_U64_PFMTHZ         Z("%")                Z(_BB_U64_PWTHSPEC)        Z("x")
	#define BB_U64_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_U64_PWTHSPEC)        Z("x")
#endif


/* BB_NS8 */
#if defined(_BB_NS8_PWTHSPEC)
	#if !defined(_BB_NS8_PSGNSPEC)
		#error _BB_NS8_PWTHSPEC is defined but _BB_NS8_PSGNSPEC is not
	#endif
	
	#define BB_NS8_PFMT             "%"                   _BB_NS8_PWTHSPEC           _BB_NS8_PSGNSPEC
	#define BB_NS8_PFMTX(parm)      "%"     parm          _BB_NS8_PWTHSPEC           _BB_NS8_PSGNSPEC
	#define BB_NS8_PFMTL           L"%"           BB_LIFY(_BB_NS8_PWTHSPEC)  BB_LIFY(_BB_NS8_PSGNSPEC)
	#define BB_NS8_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_NS8_PWTHSPEC)  BB_LIFY(_BB_NS8_PSGNSPEC)
	#define BB_NS8_PFMTH            "%"                   _BB_NS8_PWTHSPEC           "x"
	#define BB_NS8_PFMTHX(parm)     "%"     parm          _BB_NS8_PWTHSPEC           "x"
	#define BB_NS8_PFMTHL          L"%"           BB_LIFY(_BB_NS8_PWTHSPEC)         L"x"
	#define BB_NS8_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_NS8_PWTHSPEC)         L"x"
	
	#define BB_NS8_PFMTZ          Z("%")                Z(_BB_NS8_PWTHSPEC)        Z(_BB_NS8_PSGNSPEC)
	#define BB_NS8_PFMTXZ(parm)   Z("%")    parm        Z(_BB_NS8_PWTHSPEC)        Z(_BB_NS8_PSGNSPEC)
	#define BB_NS8_PFMTHZ         Z("%")                Z(_BB_NS8_PWTHSPEC)        Z("x")
	#define BB_NS8_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_NS8_PWTHSPEC)        Z("x")
#endif

/* BB_NU8 */
#if defined(_BB_NU8_PWTHSPEC)
	#if !defined(_BB_NU8_PSGNSPEC)
		#error _BB_NU8_PWTHSPEC is defined but _BB_NU8_PSGNSPEC is not
	#endif
	
	#define BB_NU8_PFMT             "%"                      _BB_NU8_PWTHSPEC           _BB_NU8_PSGNSPEC
	#define BB_NU8_PFMTX(parm)      "%"     parm             _BB_NU8_PWTHSPEC           _BB_NU8_PSGNSPEC
	#define BB_NU8_PFMTL           L"%"              BB_LIFY(_BB_NU8_PWTHSPEC)  BB_LIFY(_BB_NU8_PSGNSPEC)
	#define BB_NU8_PFMTLX(parm)    L"%"     parm     BB_LIFY(_BB_NU8_PWTHSPEC)  BB_LIFY(_BB_NU8_PSGNSPEC)
	#define BB_NU8_PFMTH            "%"                      _BB_NU8_PWTHSPEC           "x"
	#define BB_NU8_PFMTHX(parm)     "%"     parm             _BB_NU8_PWTHSPEC           "x"
	#define BB_NU8_PFMTHL          L"%"              BB_LIFY(_BB_NU8_PWTHSPEC)         L"x"
	#define BB_NU8_PFMTHLX(parm)   L"%"     parm     BB_LIFY(_BB_NU8_PWTHSPEC)         L"x"
	
	#define BB_NU8_PFMTZ          Z("%")                   Z(_BB_NU8_PWTHSPEC)        Z(_BB_NU8_PSGNSPEC)
	#define BB_NU8_PFMTXZ(parm)   Z("%")    parm           Z(_BB_NU8_PWTHSPEC)        Z(_BB_NU8_PSGNSPEC)
	#define BB_NU8_PFMTHZ         Z("%")                   Z(_BB_NU8_PWTHSPEC)        Z("x")
	#define BB_NU8_PFMTHXZ(parm)  Z("%")    parm           Z(_BB_NU8_PWTHSPEC)        Z("x")
#endif

/* BB_NS16 */
#if defined(_BB_NS16_PWTHSPEC)
	#if !defined(_BB_NS16_PSGNSPEC)
		#error _BB_NS16_PWTHSPEC is defined but _BB_NS16_PSGNSPEC is not
	#endif
	
	#define BB_NS16_PFMT             "%"                   _BB_NS16_PWTHSPEC           _BB_NS16_PSGNSPEC
	#define BB_NS16_PFMTX(parm)      "%"     parm          _BB_NS16_PWTHSPEC           _BB_NS16_PSGNSPEC
	#define BB_NS16_PFMTL           L"%"           BB_LIFY(_BB_NS16_PWTHSPEC)  BB_LIFY(_BB_NS16_PSGNSPEC)
	#define BB_NS16_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_NS16_PWTHSPEC)  BB_LIFY(_BB_NS16_PSGNSPEC)
	#define BB_NS16_PFMTH            "%"                   _BB_NS16_PWTHSPEC           "x"
	#define BB_NS16_PFMTHX(parm)     "%"     parm          _BB_NS16_PWTHSPEC           "x"
	#define BB_NS16_PFMTHL          L"%"           BB_LIFY(_BB_NS16_PWTHSPEC)         L"x"
	#define BB_NS16_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_NS16_PWTHSPEC)         L"x"
	
	#define BB_NS16_PFMTZ          Z("%")                Z(_BB_NS16_PWTHSPEC)        Z(_BB_NS16_PSGNSPEC)
	#define BB_NS16_PFMTXZ(parm)   Z("%")    parm        Z(_BB_NS16_PWTHSPEC)        Z(_BB_NS16_PSGNSPEC)
	#define BB_NS16_PFMTHZ         Z("%")                Z(_BB_NS16_PWTHSPEC)        Z("x")
	#define BB_NS16_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_NS16_PWTHSPEC)        Z("x")
#endif

/* BB_NU16 */
#if defined(_BB_NU16_PWTHSPEC)
	#if !defined(_BB_NU16_PSGNSPEC)
		#error _BB_NU16_PWTHSPEC is defined but _BB_NU16_PSGNSPEC is not
	#endif
	
	#define BB_NU16_PFMT             "%"                    _BB_NU16_PWTHSPEC           _BB_NU16_PSGNSPEC
	#define BB_NU16_PFMTX(parm)      "%"     parm           _BB_NU16_PWTHSPEC           _BB_NU16_PSGNSPEC
	#define BB_NU16_PFMTL           L"%"            BB_LIFY(_BB_NU16_PWTHSPEC)  BB_LIFY(_BB_NU16_PSGNSPEC)
	#define BB_NU16_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_NU16_PWTHSPEC)  BB_LIFY(_BB_NU16_PSGNSPEC)
	#define BB_NU16_PFMTH            "%"                    _BB_NU16_PWTHSPEC           "x"
	#define BB_NU16_PFMTHX(parm)     "%"     parm           _BB_NU16_PWTHSPEC           "x"
	#define BB_NU16_PFMTHL          L"%"            BB_LIFY(_BB_NU16_PWTHSPEC)         L"x"
	#define BB_NU16_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_NU16_PWTHSPEC)         L"x"
	
	#define BB_NU16_PFMTZ          Z("%")                 Z(_BB_NU16_PWTHSPEC)        Z(_BB_NU16_PSGNSPEC)
	#define BB_NU16_PFMTXZ(parm)   Z("%")    parm         Z(_BB_NU16_PWTHSPEC)        Z(_BB_NU16_PSGNSPEC)
	#define BB_NU16_PFMTHZ         Z("%")                 Z(_BB_NU16_PWTHSPEC)        Z("x")
	#define BB_NU16_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_NU16_PWTHSPEC)        Z("x")
#endif

/* BB_NS32 */
#if defined(_BB_NS32_PWTHSPEC)
	#if !defined(_BB_NS32_PSGNSPEC)
		#error _BB_NS32_PWTHSPEC is defined but _BB_NS32_PSGNSPEC is not
	#endif
	
	#define BB_NS32_PFMT             "%"                   _BB_NS32_PWTHSPEC           _BB_NS32_PSGNSPEC
	#define BB_NS32_PFMTX(parm)      "%"     parm          _BB_NS32_PWTHSPEC           _BB_NS32_PSGNSPEC
	#define BB_NS32_PFMTL           L"%"           BB_LIFY(_BB_NS32_PWTHSPEC)  BB_LIFY(_BB_NS32_PSGNSPEC)
	#define BB_NS32_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_NS32_PWTHSPEC)  BB_LIFY(_BB_NS32_PSGNSPEC)
	#define BB_NS32_PFMTH            "%"                   _BB_NS32_PWTHSPEC           "x"
	#define BB_NS32_PFMTHX(parm)     "%"     parm          _BB_NS32_PWTHSPEC           "x"
	#define BB_NS32_PFMTHL          L"%"           BB_LIFY(_BB_NS32_PWTHSPEC)         L"x"
	#define BB_NS32_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_NS32_PWTHSPEC)         L"x"
	
	#define BB_NS32_PFMTZ          Z("%")                Z(_BB_NS32_PWTHSPEC)        Z(_BB_NS32_PSGNSPEC)
	#define BB_NS32_PFMTXZ(parm)   Z("%")    parm        Z(_BB_NS32_PWTHSPEC)        Z(_BB_NS32_PSGNSPEC)
	#define BB_NS32_PFMTHZ         Z("%")                Z(_BB_NS32_PWTHSPEC)        Z("x")
	#define BB_NS32_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_NS32_PWTHSPEC)        Z("x")
#endif

/* BB_NU32 */
#if defined(_BB_NU32_PWTHSPEC)
	#if !defined(_BB_NU32_PSGNSPEC)
		#error _BB_NU32_PWTHSPEC is defined but _BB_NU32_PSGNSPEC is not
	#endif
	
	#define BB_NU32_PFMT             "%"                    _BB_NU32_PWTHSPEC           _BB_NU32_PSGNSPEC
	#define BB_NU32_PFMTX(parm)      "%"     parm           _BB_NU32_PWTHSPEC           _BB_NU32_PSGNSPEC
	#define BB_NU32_PFMTL           L"%"            BB_LIFY(_BB_NU32_PWTHSPEC)  BB_LIFY(_BB_NU32_PSGNSPEC)
	#define BB_NU32_PFMTLX(parm)    L"%"     parm   BB_LIFY(_BB_NU32_PWTHSPEC)  BB_LIFY(_BB_NU32_PSGNSPEC)
	#define BB_NU32_PFMTH            "%"                    _BB_NU32_PWTHSPEC           "x"
	#define BB_NU32_PFMTHX(parm)     "%"     parm           _BB_NU32_PWTHSPEC           "x"
	#define BB_NU32_PFMTHL          L"%"            BB_LIFY(_BB_NU32_PWTHSPEC)         L"x"
	#define BB_NU32_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_BB_NU32_PWTHSPEC)         L"x"
	
	#define BB_NU32_PFMTZ          Z("%")                 Z(_BB_NU32_PWTHSPEC)        Z(_BB_NU32_PSGNSPEC)
	#define BB_NU32_PFMTXZ(parm)   Z("%")    parm         Z(_BB_NU32_PWTHSPEC)        Z(_BB_NU32_PSGNSPEC)
	#define BB_NU32_PFMTHZ         Z("%")                 Z(_BB_NU32_PWTHSPEC)        Z("x")
	#define BB_NU32_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_NU32_PWTHSPEC)        Z("x")
#endif


/* BB_PTR */
#if defined(_BB_PTR_PWTHSPEC)
	#if !defined(_BB_PTR_PSGNSPEC)
		#error _BB_PTR_PWTHSPEC is defined but _BB_PTR_PSGNSPEC is not
	#endif
	
	#define BB_PTR_PFMT             "%"                      _BB_PTR_PWTHSPEC             _BB_PTR_PSGNSPEC
	#define BB_PTR_PFMTX(parm)      "%"     parm             _BB_PTR_PWTHSPEC             _BB_PTR_PSGNSPEC
	#define BB_PTR_PFMTL           L"%"              BB_LIFY(_BB_PTR_PWTHSPEC)    BB_LIFY(_BB_PTR_PSGNSPEC)
	#define BB_PTR_PFMTLX(parm)    L"%"     parm     BB_LIFY(_BB_PTR_PWTHSPEC)    BB_LIFY(_BB_PTR_PSGNSPEC)
	#define BB_PTR_PFMTH            "%"                      _BB_PTR_PWTHSPEC             "x"
	#define BB_PTR_PFMTHX(parm)     "%"     parm             _BB_PTR_PWTHSPEC             "x"
	#define BB_PTR_PFMTHL          L"%"              BB_LIFY(_BB_PTR_PWTHSPEC)           L"x"
	#define BB_PTR_PFMTHLX(parm)   L"%"     parm     BB_LIFY(_BB_PTR_PWTHSPEC)           L"x"
	
	#define BB_PTR_PFMTZ          Z("%")                   Z(_BB_PTR_PWTHSPEC)          Z(_BB_PTR_PSGNSPEC)
	#define BB_PTR_PFMTXZ(parm)   Z("%")    parm           Z(_BB_PTR_PWTHSPEC)          Z(_BB_PTR_PSGNSPEC)
	#define BB_PTR_PFMTHZ         Z("%")                   Z(_BB_PTR_PWTHSPEC)          Z("x")
	#define BB_PTR_PFMTHXZ(parm)  Z("%")    parm           Z(_BB_PTR_PWTHSPEC)          Z("x")
#endif

/* BB_SPTR */
#if defined(_BB_SPTR_PWTHSPEC)
	#if !defined(_BB_SPTR_PSGNSPEC)
		#error _BB_SPTR_PWTHSPEC is defined but _BB_SPTR_PSGNSPEC is not
	#endif
	
	#define BB_SPTR_PFMT            "%"                     _BB_SPTR_PWTHSPEC           _BB_SPTR_PSGNSPEC
	#define BB_SPTR_PFMTX(parm)     "%"      parm           _BB_SPTR_PWTHSPEC           _BB_SPTR_PSGNSPEC
	#define BB_SPTR_PFMTL          L"%"             BB_LIFY(_BB_SPTR_PWTHSPEC)  BB_LIFY(_BB_SPTR_PSGNSPEC)
	#define BB_SPTR_PFMTLX(parm)   L"%"      parm   BB_LIFY(_BB_SPTR_PWTHSPEC)  BB_LIFY(_BB_SPTR_PSGNSPEC)
	#define BB_SPTR_PFMTH           "%"                     _BB_SPTR_PWTHSPEC           "x"
	#define BB_SPTR_PFMTHX(parm)    "%"      parm           _BB_SPTR_PWTHSPEC           "x"
	#define BB_SPTR_PFMTHL         L"%"             BB_LIFY(_BB_SPTR_PWTHSPEC)         L"x"
	#define BB_SPTR_PFMTHLX(parm)  L"%"      parm   BB_LIFY(_BB_SPTR_PWTHSPEC)         L"x"
	
	#define BB_SPTR_PFMTZ          Z("%")                 Z(_BB_SPTR_PWTHSPEC)        Z(_BB_SPTR_PSGNSPEC)
	#define BB_SPTR_PFMTXZ(parm)   Z("%")    parm         Z(_BB_SPTR_PWTHSPEC)        Z(_BB_SPTR_PSGNSPEC)
	#define BB_SPTR_PFMTHZ         Z("%")                 Z(_BB_SPTR_PWTHSPEC)        Z("x")
	#define BB_SPTR_PFMTHXZ(parm)  Z("%")    parm         Z(_BB_SPTR_PWTHSPEC)        Z("x")
#endif

/* BB_UPTR */
#if defined(_BB_UPTR_PWTHSPEC)
	#if !defined(_BB_UPTR_PSGNSPEC)
		#error _BB_UPTR_PWTHSPEC is defined but _BB_UPTR_PSGNSPEC is not
	#endif
	
	#define BB_UPTR_PFMT            "%"                _BB_UPTR_PWTHSPEC           _BB_UPTR_PSGNSPEC
	#define BB_UPTR_PFMTX(parm)     "%"  parm          _BB_UPTR_PWTHSPEC           _BB_UPTR_PSGNSPEC
	#define BB_UPTR_PFMTL          L"%"        BB_LIFY(_BB_UPTR_PWTHSPEC)  BB_LIFY(_BB_UPTR_PSGNSPEC)
	#define BB_UPTR_PFMTLX(parm)   L"%"  parm  BB_LIFY(_BB_UPTR_PWTHSPEC)  BB_LIFY(_BB_UPTR_PSGNSPEC)
	#define BB_UPTR_PFMTH           "%"                _BB_UPTR_PWTHSPEC           "x"
	#define BB_UPTR_PFMTHX(parm)    "%"  parm          _BB_UPTR_PWTHSPEC           "x"
	#define BB_UPTR_PFMTHL         L"%"        BB_LIFY(_BB_UPTR_PWTHSPEC)         L"x"
	#define BB_UPTR_PFMTHLX(parm)  L"%"  parm  BB_LIFY(_BB_UPTR_PWTHSPEC)         L"x"
	
	#define BB_UPTR_PFMTZ            Z("%")                Z(_BB_UPTR_PWTHSPEC)           Z(_BB_UPTR_PSGNSPEC)
	#define BB_UPTR_PFMTXZ(parm)     Z("%")    parm           Z(_BB_UPTR_PWTHSPEC)           Z(_BB_UPTR_PSGNSPEC)
	#define BB_UPTR_PFMTHZ           Z("%")                Z(_BB_UPTR_PWTHSPEC)           Z("x")
	#define BB_UPTR_PFMTHXZ(parm)    Z("%")    parm           Z(_BB_UPTR_PWTHSPEC)           Z("x")
#endif


/* BB_DbOffsetPtr */
#if defined(_BB_DbOffsetPtr_PWTHSPEC)
	#if !defined(_BB_DbOffsetPtr_PSGNSPEC)
		#error _BB_DbOffsetPtr_PWTHSPEC is defined but _BB_DbOffsetPtr_PSGNSPEC is not
	#endif
	
	#define BB_DbOffsetPtr_PFMT             "%"                   _BB_DbOffsetPtr_PWTHSPEC           _BB_DbOffsetPtr_PSGNSPEC
	#define BB_DbOffsetPtr_PFMTX(parm)      "%"     parm          _BB_DbOffsetPtr_PWTHSPEC           _BB_DbOffsetPtr_PSGNSPEC
	#define BB_DbOffsetPtr_PFMTL           L"%"           BB_LIFY(_BB_DbOffsetPtr_PWTHSPEC)  BB_LIFY(_BB_DbOffsetPtr_PSGNSPEC)
	#define BB_DbOffsetPtr_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_DbOffsetPtr_PWTHSPEC)  BB_LIFY(_BB_DbOffsetPtr_PSGNSPEC)
	#define BB_DbOffsetPtr_PFMTH            "%"                   _BB_DbOffsetPtr_PWTHSPEC           "x"
	#define BB_DbOffsetPtr_PFMTHX(parm)     "%"     parm          _BB_DbOffsetPtr_PWTHSPEC           "x"
	#define BB_DbOffsetPtr_PFMTHL          L"%"           BB_LIFY(_BB_DbOffsetPtr_PWTHSPEC)         L"x"
	#define BB_DbOffsetPtr_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_DbOffsetPtr_PWTHSPEC)         L"x"
	
	#define BB_DbOffsetPtr_PFMTZ          Z("%")                Z(_BB_DbOffsetPtr_PWTHSPEC)        Z(_BB_DbOffsetPtr_PSGNSPEC)
	#define BB_DbOffsetPtr_PFMTXZ(parm)   Z("%")    parm        Z(_BB_DbOffsetPtr_PWTHSPEC)        Z(_BB_DbOffsetPtr_PSGNSPEC)
	#define BB_DbOffsetPtr_PFMTHZ         Z("%")                Z(_BB_DbOffsetPtr_PWTHSPEC)        Z("x")
	#define BB_DbOffsetPtr_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_DbOffsetPtr_PWTHSPEC)        Z("x")
#endif


/* BB_F32 */
#if defined(_BB_F32_PSPEC)
	#define BB_F32_PFMT            "%"                   _BB_F32_PSPEC
	#define BB_F32_PFMTX(parm)     "%"     parm          _BB_F32_PSPEC
	#define BB_F32_PFMTL          L"%"           BB_LIFY(_BB_F32_PSPEC)
	#define BB_F32_PFMTLX(parm)   L"%"     parm  BB_LIFY(_BB_F32_PSPEC)
	
	#define BB_F32_PFMTZ         Z("%")                Z(_BB_F32_PSPEC)
	#define BB_F32_PFMTXZ(parm)  Z("%")    parm        Z(_BB_F32_PSPEC)
#endif

/* BB_F64 */
#if defined(_BB_F64_PSPEC)
	#define BB_F64_PFMT            "%"                   _BB_F64_PSPEC
	#define BB_F64_PFMTX(parm)     "%"     parm          _BB_F64_PSPEC
	#define BB_F64_PFMTL          L"%"           BB_LIFY(_BB_F64_PSPEC)
	#define BB_F64_PFMTLX(parm)   L"%"     parm  BB_LIFY(_BB_F64_PSPEC)
	
	#define BB_F64_PFMTZ         Z("%")                Z(_BB_F64_PSPEC)
	#define BB_F64_PFMTXZ(parm)  Z("%")    parm        Z(_BB_F64_PSPEC)
#endif

/* BB_NF32 */
#if defined(_BB_NF32_PSPEC)
	#define BB_NF32_PFMT            "%"                   _BB_NF32_PSPEC
	#define BB_NF32_PFMTX(parm)     "%"     parm          _BB_NF32_PSPEC
	#define BB_NF32_PFMTL          L"%"           BB_LIFY(_BB_NF32_PSPEC)
	#define BB_NF32_PFMTLX(parm)   L"%"     parm  BB_LIFY(_BB_NF32_PSPEC)
	
	#define BB_NF32_PFMTZ         Z("%")                Z(_BB_NF32_PSPEC)
	#define BB_NF32_PFMTXZ(parm)  Z("%")    parm        Z(_BB_NF32_PSPEC)
#endif


/* BB_DbOffset32 */
#if defined(_BB_DbOffset32_PWTHSPEC)
	#if !defined(_BB_DbOffset32_PSGNSPEC)
		#error _BB_DbOffset32_PWTHSPEC is defined but _BB_DbOffset32_PSGNSPEC is not
	#endif
	
	#define BB_DbOffset32_PFMT            "%"                   _BB_DbOffset32_PWTHSPEC           _BB_DbOffset32_PSGNSPEC
	#define BB_DbOffset32_PFMTX(parm)     "%"     parm          _BB_DbOffset32_PWTHSPEC           _BB_DbOffset32_PSGNSPEC
	#define BB_DbOffset32_PFMTL          L"%"           BB_LIFY(_BB_DbOffset32_PWTHSPEC)  BB_LIFY(_BB_DbOffset32_PSGNSPEC)
	#define BB_DbOffset32_PFMTLX(parm)   L"%"     parm  BB_LIFY(_BB_DbOffset32_PWTHSPEC)  BB_LIFY(_BB_DbOffset32_PSGNSPEC)
	#define BB_DbOffset32_PFMTH           "%"                   _BB_DbOffset32_PWTHSPEC           "x"
	#define BB_DbOffset32_PFMTHX(parm)    "%"     parm          _BB_DbOffset32_PWTHSPEC           "x"
	#define BB_DbOffset32_PFMTHL         L"%"           BB_LIFY(_BB_DbOffset32_PWTHSPEC)         L"x"
	#define BB_DbOffset32_PFMTHLX(parm)  L"%"     parm  BB_LIFY(_BB_DbOffset32_PWTHSPEC)         L"x"
	
	#define BB_DbOffset32_PFMTZ          Z("%")               Z(_BB_DbOffset32_PWTHSPEC)        Z(_BB_DbOffset32_PSGNSPEC)
	#define BB_DbOffset32_PFMTXZ(parm)   Z("%")    parm       Z(_BB_DbOffset32_PWTHSPEC)        Z(_BB_DbOffset32_PSGNSPEC)
	#define BB_DbOffset32_PFMTHZ         Z("%")               Z(_BB_DbOffset32_PWTHSPEC)        Z("x")
	#define BB_DbOffset32_PFMTHXZ(parm)  Z("%")    parm       Z(_BB_DbOffset32_PWTHSPEC)        Z("x")
#endif


/* BB_ERROR */
#if defined(_BB_ERROR_PWTHSPEC)
	#if !defined(_BB_ERROR_PSGNSPEC)
		#error _BB_ERROR_PWTHSPEC is defined but _BB_ERROR_PSGNSPEC is not
	#endif
	
	#define BB_ERROR_PFMT              "%"                    _BB_ERROR_PWTHSPEC           _BB_ERROR_PSGNSPEC
	#define BB_ERROR_PFMTX(parm)       "%"     parm           _BB_ERROR_PWTHSPEC           _BB_ERROR_PSGNSPEC
	#define BB_ERROR_PFMTL            L"%"            BB_LIFY(_BB_ERROR_PWTHSPEC)  BB_LIFY(_BB_ERROR_PSGNSPEC)
	#define BB_ERROR_PFMTLX(parm)     L"%"     parm   BB_LIFY(_BB_ERROR_PWTHSPEC)  BB_LIFY(_BB_ERROR_PSGNSPEC)
	#define BB_ERROR_PFMTH             "%"                    _BB_ERROR_PWTHSPEC           "x"
	#define BB_ERROR_PFMTHX(parm)      "%"     parm           _BB_ERROR_PWTHSPEC           "x"
	#define BB_ERROR_PFMTHL           L"%"            BB_LIFY(_BB_ERROR_PWTHSPEC)         L"x"
	#define BB_ERROR_PFMTHLX(parm)    L"%"     parm   BB_LIFY(_BB_ERROR_PWTHSPEC)         L"x"
	
	#define BB_ERROR_PFMTZ           Z("%")                 Z(_BB_ERROR_PWTHSPEC)        Z(_BB_ERROR_PSGNSPEC)
	#define BB_ERROR_PFMTXZ(parm)    Z("%")    parm         Z(_BB_ERROR_PWTHSPEC)        Z(_BB_ERROR_PSGNSPEC)
	#define BB_ERROR_PFMTHZ          Z("%")                 Z(_BB_ERROR_PWTHSPEC)        Z("x")
	#define BB_ERROR_PFMTHXZ(parm)   Z("%")    parm         Z(_BB_ERROR_PWTHSPEC)        Z("x")
#endif


/* BB_TCHAR */
#if defined(_BB_TCHAR_PSPEC)
	#define BB_TCHAR_PFMT            "%"                    _BB_TCHAR_PSPEC
	#define BB_TCHAR_PFMTX(parm)     "%"     parm           _BB_TCHAR_PSPEC
	#define BB_TCHAR_PFMTL          L"%"            BB_LIFY(_BB_TCHAR_PSPEC)
	#define BB_TCHAR_PFMTLX(parm)   L"%"     parm   BB_LIFY(_BB_TCHAR_PSPEC)
	
	#define BB_TCHAR_PFMTZ         Z("%")                 Z(_BB_TCHAR_PSPEC)
	#define BB_TCHAR_PFMTXZ(parm)  Z("%")    parm         Z(_BB_TCHAR_PSPEC)
#endif


/* BB_TSTR */
#if defined(_BB_TSTR_PSPEC)
	#define BB_TSTR_PFMT            "%"                    _BB_TSTR_PSPEC
	#define BB_TSTR_PFMTX(parm)     "%"     parm           _BB_TSTR_PSPEC
	#define BB_TSTR_PFMTL          L"%"            BB_LIFY(_BB_TSTR_PSPEC)
	#define BB_TSTR_PFMTLX(parm)   L"%"     parm   BB_LIFY(_BB_TSTR_PSPEC)
	
	#define BB_TSTR_PFMTZ         Z("%")                 Z(_BB_TSTR_PSPEC)
	#define BB_TSTR_PFMTXZ(parm)  Z("%")    parm         Z(_BB_TSTR_PSPEC)
#endif


/* `sizeof(x)` where `x` is a type */
#if defined(_BB_SIZEOF_PWTHSPEC)
	#if !defined(_BB_SIZEOF_PSGNSPEC)
		#error _BB_SIZEOF_PWTHSPEC is defined but _BB_SIZEOF_PSGNSPEC is not
	#endif
	
	#define BB_SIZEOF_PFMT             "%"                   _BB_SIZEOF_PWTHSPEC           _BB_SIZEOF_PSGNSPEC
	#define BB_SIZEOF_PFMTX(parm)      "%"     parm          _BB_SIZEOF_PWTHSPEC           _BB_SIZEOF_PSGNSPEC
	#define BB_SIZEOF_PFMTL           L"%"           BB_LIFY(_BB_SIZEOF_PWTHSPEC)  BB_LIFY(_BB_SIZEOF_PSGNSPEC)
	#define BB_SIZEOF_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_SIZEOF_PWTHSPEC)  BB_LIFY(_BB_SIZEOF_PSGNSPEC)
	#define BB_SIZEOF_PFMTH            "%"                   _BB_SIZEOF_PWTHSPEC           "x"
	#define BB_SIZEOF_PFMTHX(parm)     "%"     parm          _BB_SIZEOF_PWTHSPEC           "x"
	#define BB_SIZEOF_PFMTHL          L"%"           BB_LIFY(_BB_SIZEOF_PWTHSPEC)         L"x"
	#define BB_SIZEOF_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_SIZEOF_PWTHSPEC)         L"x"
	
	#define BB_SIZEOF_PFMTZ          Z("%")                Z(_BB_SIZEOF_PWTHSPEC)        Z(_BB_SIZEOF_PSGNSPEC)
	#define BB_SIZEOF_PFMTXZ(parm)   Z("%")    parm        Z(_BB_SIZEOF_PWTHSPEC)        Z(_BB_SIZEOF_PSGNSPEC)
	#define BB_SIZEOF_PFMTHZ         Z("%")                Z(_BB_SIZEOF_PWTHSPEC)        Z("x")
	#define BB_SIZEOF_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_SIZEOF_PWTHSPEC)        Z("x")
#endif


/* Native pointer representation */
#define BB_NPTR_PFMT              BB_UPTR_PFMT
#define BB_NPTR_PFMTX(parm)       BB_UPTR_PFMTX(parm)
#define BB_NPTR_PFMTL             BB_UPTR_PFMTL
#define BB_NPTR_PFMTLX(parm)      BB_UPTR_PFMTLX(parm)
#define BB_NPTR_PFMTH             BB_UPTR_PFMTH
#define BB_NPTR_PFMTHX(parm)      BB_UPTR_PFMTHX(parm)
#define BB_NPTR_PFMTHL            BB_UPTR_PFMTHL
#define BB_NPTR_PFMTHLX(parm)     BB_UPTR_PFMTHLX(parm)

#define BB_NPTR_PFMTZ             BB_UPTR_PFMTZ
#define BB_NPTR_PFMTXZ(parm)      BB_UPTR_PFMTXZ(parm)
#define BB_NPTR_PFMTHZ            BB_UPTR_PFMTHZ
#define BB_NPTR_PFMTHXZ(parm)     BB_UPTR_PFMTHXZ(parm)

#if (BB_UPTR_BITS == 64)
	#define BB_NPTR_PFMTPTR    "0x"  BB_UPTR_PFMTHX("016")
	#define BB_NPTR_PFMTPTRL  L"0x"  BB_UPTR_PFMTHLX(L"016")
	#define BB_NPTR_PFMTPTRNP        BB_UPTR_PFMTHX("016")
	#define BB_NPTR_PFMTPTRNPL       BB_UPTR_PFMTHLX(L"016")
	
	#define BB_NPTR_PFMTPTRZ   "0x"  BB_UPTR_PFMTHXZ(Z("016"))
	#define BB_NPTR_PFMTPTRNPZ       BB_UPTR_PFMTHXZ(Z("016"))
#elif (BB_UPTR_BITS == 32)
	#define BB_NPTR_PFMTPTR    "0x"  BB_UPTR_PFMTHX("08")
	#define BB_NPTR_PFMTPTRL  L"0x"  BB_UPTR_PFMTHLX(L"08")
	#define BB_NPTR_PFMTPTRNP        BB_UPTR_PFMTHX("08")
	#define BB_NPTR_PFMTPTRNPL       BB_UPTR_PFMTHLX(L"08")
	
	#define BB_NPTR_PFMTPTRZ   "0x"  BB_UPTR_PFMTHXZ(Z("08"))
	#define BB_NPTR_PFMTPTRNPZ       BB_UPTR_PFMTHXZ(Z("08"))
#endif


/* `enum` */
#if defined(_BB_ENUM_PWTHSPEC)
	#if !defined(_BB_ENUM_PSGNSPEC)
		#error _BB_ENUM_PWTHSPEC is defined but _BB_ENUM_PSGNSPEC is not
	#endif
	
	#define BB_ENUM_PFMT             "%"                   _BB_ENUM_PWTHSPEC           _BB_ENUM_PSGNSPEC
	#define BB_ENUM_PFMTX(parm)      "%"     parm          _BB_ENUM_PWTHSPEC           _BB_ENUM_PSGNSPEC
	#define BB_ENUM_PFMTL           L"%"           BB_LIFY(_BB_ENUM_PWTHSPEC)  BB_LIFY(_BB_ENUM_PSGNSPEC)
	#define BB_ENUM_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_ENUM_PWTHSPEC)  BB_LIFY(_BB_ENUM_PSGNSPEC)
	#define BB_ENUM_PFMTH            "%"                   _BB_ENUM_PWTHSPEC           "x"
	#define BB_ENUM_PFMTHX(parm)     "%"     parm          _BB_ENUM_PWTHSPEC           "x"
	#define BB_ENUM_PFMTHL          L"%"           BB_LIFY(_BB_ENUM_PWTHSPEC)         L"x"
	#define BB_ENUM_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_ENUM_PWTHSPEC)         L"x"
	
	#define BB_ENUM_PFMTZ          Z("%")                Z(_BB_ENUM_PWTHSPEC)        Z(_BB_ENUM_PSGNSPEC)
	#define BB_ENUM_PFMTXZ(parm)   Z("%")    parm        Z(_BB_ENUM_PWTHSPEC)        Z(_BB_ENUM_PSGNSPEC)
	#define BB_ENUM_PFMTHZ         Z("%")                Z(_BB_ENUM_PWTHSPEC)        Z("x")
	#define BB_ENUM_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_ENUM_PWTHSPEC)        Z("x")
#endif


/* Win32's DWORD (also defined in AcaMul SDK public headers for compatibility) */
#if defined(_BB_U32_PWTHSPEC)
	#if !defined(_BB_U32_PSGNSPEC)
		#error _BB_U32_PWTHSPEC is defined but _BB_U32_PSGNSPEC is not
	#endif
	
	#define DWORD_PFMT             "%"                   _BB_U32_PWTHSPEC            _BB_U32_PSGNSPEC
	#define DWORD_PFMTX(parm)      "%"     parm          _BB_U32_PWTHSPEC            _BB_U32_PSGNSPEC
	#define DWORD_PFMTL           L"%"           BB_LIFY(_BB_U32_PWTHSPEC)   BB_LIFY(_BB_U32_PSGNSPEC)
	#define DWORD_PFMTLX(parm)    L"%"     parm  BB_LIFY(_BB_U32_PWTHSPEC)   BB_LIFY(_BB_U32_PSGNSPEC)
	#define DWORD_PFMTH            "%"                   _BB_U32_PWTHSPEC             "x"
	#define DWORD_PFMTHX(parm)     "%"     parm          _BB_U32_PWTHSPEC             "x"
	#define DWORD_PFMTHL          L"%"           BB_LIFY(_BB_U32_PWTHSPEC)           L"x"
	#define DWORD_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_BB_U32_PWTHSPEC)           L"x"
	
	#define DWORD_PFMTZ          Z("%")                Z(_BB_U32_PWTHSPEC)          Z(_BB_U32_PSGNSPEC)
	#define DWORD_PFMTXZ(parm)   Z("%")    parm        Z(_BB_U32_PWTHSPEC)          Z(_BB_U32_PSGNSPEC)
	#define DWORD_PFMTHZ         Z("%")                Z(_BB_U32_PWTHSPEC)          Z("x")
	#define DWORD_PFMTHXZ(parm)  Z("%")    parm        Z(_BB_U32_PWTHSPEC)          Z("x")
#endif


/* Win32's DWORD_PTR (also defined in AcaMul SDK public headers for compatibility) */
#if defined(_DWORD_PTR_PWTHSPEC)
	#if !defined(_DWORD_PTR_PSGNSPEC)
		#error _DWORD_PTR_PWTHSPEC is defined but _DWORD_PTR_PSGNSPEC is not
	#endif
	
	#define DWORD_PTR_PFMT             "%"                    _DWORD_PTR_PWTHSPEC           _DWORD_PTR_PSGNSPEC
	#define DWORD_PTR_PFMTX(parm)      "%"     parm           _DWORD_PTR_PWTHSPEC           _DWORD_PTR_PSGNSPEC
	#define DWORD_PTR_PFMTL           L"%"            BB_LIFY(_DWORD_PTR_PWTHSPEC)  BB_LIFY(_DWORD_PTR_PSGNSPEC)
	#define DWORD_PTR_PFMTLX(parm)    L"%"     parm   BB_LIFY(_DWORD_PTR_PWTHSPEC)  BB_LIFY(_DWORD_PTR_PSGNSPEC)
	#define DWORD_PTR_PFMTH            "%"                    _DWORD_PTR_PWTHSPEC           "x"
	#define DWORD_PTR_PFMTHX(parm)     "%"     parm           _DWORD_PTR_PWTHSPEC           "x"
	#define DWORD_PTR_PFMTHL          L"%"            BB_LIFY(_DWORD_PTR_PWTHSPEC)         L"x"
	#define DWORD_PTR_PFMTHLX(parm)   L"%"     parm   BB_LIFY(_DWORD_PTR_PWTHSPEC)         L"x"
	
	#define DWORD_PTR_PFMTZ          Z("%")                  _DWORD_PTR_PWTHSPEC          Z(_DWORD_PTR_PSGNSPEC)
	#define DWORD_PTR_PFMTXZ(parm)   Z("%")    parm          _DWORD_PTR_PWTHSPEC          Z(_DWORD_PTR_PSGNSPEC)
	#define DWORD_PTR_PFMTHZ         Z("%")                  _DWORD_PTR_PWTHSPEC          Z("x")
	#define DWORD_PTR_PFMTHXZ(parm)  Z("%")    parm          _DWORD_PTR_PWTHSPEC          Z("x")
#endif


/* ssize_t */
#if defined(_SSIZE_T_PWTHSPEC)
	#if !defined(_SIZE_T_PSGNSPEC)
		#error _SSIZE_T_PWTHSPEC is defined but _SSIZE_T_PSGNSPEC is not
	#endif
	
	#define SSIZE_T_PFMT             "%"                   _SSIZE_T_PWTHSPEC           _SSIZE_T_PSGNSPEC
	#define SSIZE_T_PFMTX(parm)      "%"     parm          _SSIZE_T_PWTHSPEC           _SSIZE_T_PSGNSPEC
	#define SSIZE_T_PFMTL           L"%"           BB_LIFY(_SSIZE_T_PWTHSPEC)  BB_LIFY(_SSIZE_T_PSGNSPEC)
	#define SSIZE_T_PFMTLX(parm)    L"%"     parm  BB_LIFY(_SSIZE_T_PWTHSPEC)  BB_LIFY(_SSIZE_T_PSGNSPEC)
	#define SSIZE_T_PFMTH            "%"                   _SSIZE_T_PWTHSPEC           "x"
	#define SSIZE_T_PFMTHX(parm)     "%"     parm          _SSIZE_T_PWTHSPEC           "x"
	#define SSIZE_T_PFMTHL          L"%"           BB_LIFY(_SSIZE_T_PWTHSPEC)         L"x"
	#define SSIZE_T_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_SSIZE_T_PWTHSPEC)         L"x"
	
	#define SSIZE_T_PFMTZ          Z("%")                Z(_SSIZE_T_PWTHSPEC)        Z(_SSIZE_T_PSGNSPEC)
	#define SSIZE_T_PFMTXZ(parm)   Z("%")    parm        Z(_SSIZE_T_PWTHSPEC)        Z(_SSIZE_T_PSGNSPEC)
	#define SSIZE_T_PFMTHZ         Z("%")                Z(_SSIZE_T_PWTHSPEC)        Z("x")
	#define SSIZE_T_PFMTHXZ(parm)  Z("%")    parm        Z(_SSIZE_T_PWTHSPEC)        Z("x")
#endif


/* size_t */
#if defined(_SIZE_T_PWTHSPEC)
	#if !defined(_SIZE_T_PSGNSPEC)
		#error _SIZE_T_PWTHSPEC is defined but _SIZE_T_PSGNSPEC is not
	#endif
	
	#define SIZE_T_PFMT             "%"                   _SIZE_T_PWTHSPEC           _SIZE_T_PSGNSPEC
	#define SIZE_T_PFMTX(parm)      "%"     parm          _SIZE_T_PWTHSPEC           _SIZE_T_PSGNSPEC
	#define SIZE_T_PFMTL           L"%"           BB_LIFY(_SIZE_T_PWTHSPEC)  BB_LIFY(_SIZE_T_PSGNSPEC)
	#define SIZE_T_PFMTLX(parm)    L"%"     parm  BB_LIFY(_SIZE_T_PWTHSPEC)  BB_LIFY(_SIZE_T_PSGNSPEC)
	#define SIZE_T_PFMTH            "%"                   _SIZE_T_PWTHSPEC           "x"
	#define SIZE_T_PFMTHX(parm)     "%"     parm          _SIZE_T_PWTHSPEC           "x"
	#define SIZE_T_PFMTHL          L"%"           BB_LIFY(_SIZE_T_PWTHSPEC)         L"x"
	#define SIZE_T_PFMTHLX(parm)   L"%"     parm  BB_LIFY(_SIZE_T_PWTHSPEC)         L"x"
	
	#define SIZE_T_PFMTZ          Z("%")                Z(_SIZE_T_PWTHSPEC)        Z(_SIZE_T_PSGNSPEC)
	#define SIZE_T_PFMTXZ(parm)   Z("%")    parm        Z(_SIZE_T_PWTHSPEC)        Z(_SIZE_T_PSGNSPEC)
	#define SIZE_T_PFMTHZ         Z("%")                Z(_SIZE_T_PWTHSPEC)        Z("x")
	#define SIZE_T_PFMTHXZ(parm)  Z("%")    parm        Z(_SIZE_T_PWTHSPEC)        Z("x")
#endif


/**********************************************************************/
/*   VERSION TYPE DEFINITION                                          */
/**********************************************************************/
typedef struct BB_Version_tag
{
  BB_U16 moduleID ;
  BB_U16 majorVersionNumber;
  BB_U16 minorVersionNumber;
  BB_U16 revisionVersionNumber;
} BB_VERSION;



/**********************************************************************/
/*   DATABASE FORMAT TYPE DEFINITION                                  */
/**********************************************************************/
typedef struct BB_DbFormat_tag
{
  BB_U8 dbAlignment ;   /* BB_ALIGN8 | BB_ALIGN16 | BB_ALIGN32 */
  BB_U8 dbMAU;          /* BB_MAU8 | BB_MAU16 | BB_MAU32 | BB_MAU16C8 | BB_MAU32C8 | BB_MAU32C16 */
  BB_U8 dbEndianness;   /* BB_LITENDIAN | BB_MIXENDIAN | BB_BIGENDIAN */
} BB_DbFormat;



/**********************************************************************/
/*   BOOLEAN DEFINITION                                               */
/**********************************************************************/
/* For portability, don't use enum !!!!! */
#define BB_BOOL BB_S8
#define BB_NBOOL BB_NS8
#define BB_FALSE  0
#define BB_TRUE  1

/**********************************************************************/
/*   SPECIAL DEFINITION                                               */
/**********************************************************************/
/* this type refers an item which size is 1 byte located in Ram Rom or File (Read Only)*/
/* Be careful when accessing these values */

#ifdef PLATFORM_LINUX
typedef signed char BB_CCR;
#else
typedef char BB_CCR;
#endif


/**********************************************************************/
/*   TYPES ENUM                                                       */
/**********************************************************************/
typedef enum
{
  BB_U8Type=0,
  BB_U16Type,
  BB_U32Type,
  BB_S8Type,
  BB_S16Type,
  BB_S32Type,
  BB_TCHARType
} BB_DataType;

#if defined (PALMOS5) || defined (__HEW_IDE__)
#ifndef EOF
#define EOF (-1)
#endif
#endif

#if defined  (__HEW_IDE__)
#ifndef NULL
#define NULL 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR    1
#endif
#ifndef SEEK_END
#define SEEK_END    2
#endif
#ifndef SEEK_SET
#define SEEK_SET    0
#endif
#endif


/**********************************************************************/
/*   MEMORY-DEPENDENT INTEGER LIMITS                                  */
/**********************************************************************/

/* MAX & MIN constants for MEMORY-related types */

#define BB_S8_MAX  0x7f
#define BB_S8_MIN  0x80
#define BB_U8_MAX  0xffu
#define BB_U8_MIN  0x00u

#define BB_S16_MAX 0x7fff
#define BB_S16_MIN 0x8000
#define BB_U16_MAX 0xffffu
#define BB_U16_MIN 0x0000u

#define BB_S32_MAX 0x7fffffff
#define BB_S32_MIN 0x80000000
#define BB_U32_MAX 0xffffffffu
#define BB_U32_MIN 0x00000000u

#if (__BB_CPU_TYPE__ == __BB_CPU32__) && defined(_MSC_VER)
	#define BB_S64_MAX 0x7fffffffffffffffi64
	#define BB_S64_MIN 0x8000000000000000i64
	#define BB_U64_MAX 0xffffffffffffffffui64
	#define BB_U64_MIN 0x0000000000000000ui64
#elif !defined(__GNUC__) || (defined(__GNUC__) && (__GNUC__ >= 3))
	#define BB_S64_MAX 0x7fffffffffffffffll
	#define BB_S64_MIN 0x8000000000000000ll
	#define BB_U64_MAX 0xffffffffffffffffull
	#define BB_U64_MIN 0x0000000000000000ull
#endif 

#define BB_DbOffset32_MAX 0x7fffffff
#define BB_DbOffset32_MIN 0x80000000


/**********************************************************************/
/*   CPU DEPENDENT INTEGER DEFINITION                                 */
/**********************************************************************/

/* MAX & MIN constants for CPU-related (natural) types */

/* BB_Nxx are the types with the natural type width of the platform and which 
   is guaranteed to have a minimum of xx bits.
   
   The natural type width for a 32-bit architecture is 32 bits.
   
   The natural type width for a 64-bit architecture is 64 bits. */

#if (__BB_CPU_TYPE__ == __BB_CPU32__)
	#define BB_NS8_MAX          0x7fffffff
	#define BB_NS8_MIN          0x80000000
	#define BB_NU8_MAX          0xffffffffu
	#define BB_NU8_MIN          0x00000000u
	
	#define BB_NS16_MAX         0x7fffffff
	#define BB_NS16_MIN         0x80000000
	#define BB_NU16_MAX         0xffffffffu
	#define BB_NU16_MIN         0x00000000u
	
	#define BB_NS32_MAX         0x7fffffff
	#define BB_NS32_MIN         0x80000000
	#define BB_NU32_MAX         0xffffffffu
	#define BB_NU32_MIN         0x00000000u
	
	#define BB_PTR_MAX          0xffffffffu
	#define BB_PTR_MIN          0x00000000u
	#define BB_SPTR_MAX         0x7fffffff
	#define BB_SPTR_MIN         0x80000000
	#define BB_UPTR_MAX         BB_PTR_MAX
	#define BB_UPTR_MIN         BB_PTR_MIN
	
	#define BB_DbOffsetPtr_MAX  BB_UPTR_MAX
	#define BB_DbOffsetPtr_MIN  BB_UPTR_MIN
#elif (__BB_CPU_TYPE__ == __BB_CPU64__)
	#define BB_NS8_MAX          0x7fffffffffffffffll
	#define BB_NS8_MIN          0x8000000000000000ll
	#define BB_NU8_MAX          0xffffffffffffffffull
	#define BB_NU8_MIN          0x0000000000000000ull
	
	#define BB_NS16_MAX         0x7fffffffffffffffll
	#define BB_NS16_MIN         0x8000000000000000ll
	#define BB_NU16_MAX         0xffffffffffffffffull
	#define BB_NU16_MIN         0x0000000000000000ull
	
	#define BB_NS32_MAX         0x7fffffffffffffffll
	#define BB_NS32_MIN         0x8000000000000000ll
	#define BB_NU32_MAX         0xffffffffffffffffull
	#define BB_NU32_MIN         0x0000000000000000ull
	
	#define BB_PTR_MAX          0xffffffffffffffffull
	#define BB_PTR_MIN          0x0000000000000000ull
	#define BB_SPTR_MAX         0x7fffffffffffffffll
	#define BB_SPTR_MIN         0x8000000000000000ll
	#define BB_UPTR_MAX         BB_PTR_MAX
	#define BB_UPTR_MIN         BB_PTR_MIN
	
	#define BB_DbOffsetPtr_MAX  BB_UPTR_MAX
	#define BB_DbOffsetPtr_MIN  BB_UPTR_MIN
#else
	/* BB_?_MIN and BB_?_MAX are only defined for 32- and 64-bit for now. */
	#error undefined BB_?_MIN and BB_?_MAX for __BB_CPU_TYPE__
#endif


/**********************************************************************/
/*   INTEGER TYPES LIMIT ALIASES                                      */
/**********************************************************************/

#if defined(BB_S8_MAX)
	#define BB_MAX_S8  BB_S8_MAX
	#define BB_MIN_S8  BB_S8_MIN
#endif
#if defined(BB_U8_MAX)
	#define BB_MAX_U8  BB_U8_MAX
	#define BB_MIN_U8  BB_U8_MIN
#endif

#if defined(BB_S16_MAX)
	#define BB_MAX_S16 BB_S16_MAX
	#define BB_MIN_S16 BB_S16_MIN
#endif
#if defined(BB_U16_MAX)
	#define BB_MAX_U16 BB_U16_MAX
	#define BB_MIN_U16 BB_U16_MIN
#endif

#if defined(BB_S32_MAX)
	#define BB_MAX_S32 BB_S32_MAX
	#define BB_MIN_S32 BB_S32_MIN
#endif
#if defined(BB_U32_MAX)
	#define BB_MAX_U32 BB_U32_MAX
	#define BB_MIN_U32 BB_U32_MIN
#endif

#if defined(BB_S64_MAX)
	#define BB_MAX_S64 BB_S64_MAX
	#define BB_MIN_S64 BB_S64_MIN
#endif
#if defined(BB_U64_MAX)
	#define BB_MAX_U64 BB_U64_MAX
	#define BB_MIN_U64 BB_U64_MIN
#endif


#if defined(BB_NS8_MAX)
	#define BB_MAX_NS8  BB_NS8_MAX
	#define BB_MIN_NS8  BB_NS8_MIN
#endif
#if defined(BB_NU8_MAX)
	#define BB_MAX_NU8  BB_NU8_MAX
	#define BB_MIN_NU8  BB_NU8_MIN
#endif

#if defined(BB_NS16_MAX)
	#define BB_MAX_NS16 BB_NS16_MAX
	#define BB_MIN_NS16 BB_NS16_MIN
#endif
#if defined(BB_NU16_MAX)
	#define BB_MAX_NU16 BB_NU16_MAX
	#define BB_MIN_NU16 BB_NU16_MIN
#endif

#if defined(BB_NS32_MAX)
	#define BB_MAX_NS32 BB_NS32_MAX
	#define BB_MIN_NS32 BB_NS32_MIN
#endif
#if defined(BB_NU32_MAX)
	#define BB_MAX_NU32 BB_NU32_MAX
	#define BB_MIN_NU32 BB_NU32_MIN
#endif

#if defined(BB_PTR_MAX)
	#define BB_MAX_PTR  BB_PTR_MAX
	#define BB_MIN_PTR  BB_PTR_MIN
#endif
#if defined(BB_SPTR_MAX)
	#define BB_MAX_SPTR BB_SPTR_MAX
	#define BB_MIN_SPTR BB_SPTR_MIN
#endif
#if defined(BB_UPTR_MAX)
	#define BB_MAX_UPTR BB_UPTR_MAX
	#define BB_MIN_UPTR BB_UPTR_MIN
#endif

#if defined(BB_DbOffset32_MAX)
	#define BB_MAX_DbOffset32 BB_DbOffset32_MAX
	#define BB_MIN_DbOffset32 BB_DbOffset32_MIN
#endif

#if defined(BB_DbOffsetPtr_MAX)
	#define BB_MAX_DbOffsetPtr BB_DbOffsetPtr_MAX
	#define BB_MIN_DbOffsetPtr BB_DbOffsetPtr_MIN
#endif


/**********************************************************************/
/*   LOW/HIGH PART EXTRACTION FROM INTEGERS                           */
/**********************************************************************/

#define BB_S16_LOPART(val) ((BB_S8)  ( ((BB_S16) (val)) & 0x00ff))
#define BB_S16_HIPART(val) ((BB_S8)  ((((BB_S16) (val)) & 0xff00) >> 8u))

#define BB_U16_LOPART(val) ((BB_U8)  ( ((BB_U16) (val)) & 0x00ffu))
#define BB_U16_HIPART(val) ((BB_U8)  ((((BB_U16) (val)) & 0xff00u) >> 8u))

#define BB_S32_LOPART(val) ((BB_S16) ( ((BB_S32) (val)) & 0x0000ffff))
#define BB_S32_HIPART(val) ((BB_S16) ((((BB_S32) (val)) & 0xffff0000) >> 16u))

#define BB_U32_LOPART(val) ((BB_U16) ( ((BB_U32) (val)) & 0x0000ffffu))
#define BB_U32_HIPART(val) ((BB_U16) ((((BB_U32) (val)) & 0xffff0000u) >> 16u))

#define BB_S64_LOPART(val) ((BB_S32) ( ((BB_S64) (val)) & BB_S64_C(0x00000000ffffffff)))
#define BB_S64_HIPART(val) ((BB_S32) ((((BB_S64) (val)) & BB_S64_C(0xffffffff00000000)) >> 32u))

#define BB_U64_LOPART(val) ((BB_U32) ( ((BB_U64) (val)) & BB_U64_C(0x00000000ffffffff)))
#define BB_U64_HIPART(val) ((BB_U32) ((((BB_U64) (val)) & BB_U64_C(0xffffffff00000000)) >> 32u))


/**********************************************************************/
/*   CONSTRUCTION OF INTEGERS FROM THEIR DIFFERENT COMPONENTS         */
/**********************************************************************/

#define BB_U16_MAKE(low, high) ( \
	(BB_U16) ( \
		( (BB_U8) ((low)  & 0xffu)) | \
		(((BB_U8) ((high) & 0xffu)) << 8u) \
	) \
)

#define BB_U32_MAKE(low, high) ( \
	(BB_U32) ( \
		( (BB_U16) ((low)  & 0xffffu)) | \
		(((BB_U16) ((high) & 0xffffu)) << 16u) \
	) \
)

#define BB_U64_MAKE(low, high) ( \
	(BB_U64) ( \
		( (BB_U32) ((low)  & 0xffffffffu)) | \
		(((BB_U32) ((high) & 0xffffffffu)) << 32u) \
	) \
)

#endif /* __COMMON__BB_TYPES_H__ */
