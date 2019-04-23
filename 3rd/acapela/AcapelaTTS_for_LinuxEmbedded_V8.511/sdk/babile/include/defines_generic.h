/*
IMPORTANT/ TO USE THIS FILE, import defines.h from CVS in BABILNLP branch
*/

#ifndef __GENERIC_DEFS_H__
#define __GENERIC_DEFS_H__

#define ROMTOOLS_0 /* Special platform, see below */
#define RESEARCH_0 1 /* Special platform, see below */
#define MALLOCED_0 /* valgrinding purpose */

/* ================= MODULES AVAILABILITY CONFIGURATION ===================== */
#ifndef	      ACA_COMPILEFLAGS_DEFINED   /* COMMAND LINE CONFIGURATION */
#  define     BBNLP_COMPILEFLAG_NLPEENABLE
#  define     BBNLP_COMPILEFLAG_TTFENABLE_0
#  define     BABILE_COMPILEFLAG_SELECTORENABLE
#  define     BABILE_COMPILEFLAG_MBROLAENABLE
#  define     BABILE_COMPILEFLAG_COLIBRIENABLE
#endif

#ifdef BABILE_COMPILEFLAG_SELECTORENABLE /* SELECTOR FLAGS ====================  */
# define BBSELECTOR_COMPILEFLAG_MALLOCENABLE
# ifndef BBSELECTOR_COMPILEFLAGS_DEFINED   /* COMMAND LINE CONFIGURATION */
#  define BBSELECTOR_COMPILEFLAG_TREMORENABLE    
#  define BBSELECTOR_COMPILEFLAG_PICOLAENABLE
/* Those 4 flags below have to be defined either to 0 either to 1 */
#  define BBSELECTOR_COMPILEFLAG_OPUSENABLE 0
#  define BBSELECTOR_COMPILEFLAG_OGGENABLE 1
#  define BBSELECTOR_COMPILEFLAG_HNMENABLE 0
#  define BBSELECTOR_COMPILEFLAG_WAVENABLE 1
/* Those 4 flags above have to be defined either to 0 either to 1 */
# endif /* BBSELECTOR_COMPILEFLAGS_DEFINED */
# define BBSELECTOR_ROPI_NOVIRTUAL_ONLYOGG_0     /* Override of C++ virtuals function to use only OGG */
#endif
#ifdef BBNLP_COMPILEFLAG_TTFENABLE  /* TTF FLAGS ================================ */
# define IVXTTF_COMPILEFLAG_NORMALIZERENABLE
#endif
#ifdef BABILE_COMPILEFLAG_MBROLAENABLE  /* MBROLAE FLAGS ======================== */
# define ALAW
# define BACON
# define DEBUG_MESSAGE_0
# define READFRTOFILE_0
# define SAMPLESSTREAM_0
# define COPRO_OVERLAPADD_BASIC_0
#endif
#define BBNLP_COMPILEFLAG_MALLOCENABLE

#define BABILE_COMPILEFLAG_MALLOCENABLE

#define JA_JP_OPTIM
#define ARBRECOD    /*Read .trx format */


#define SHOW_SPLASH   /* babile demo cpp dll */
#define USER_DCT_CALLBACK_TEST_0 /* babile demo */

/* ================    Global defines ============================================= */
#define EMBEDDED             /* global define for static buffered memory model */
#define EMBEDDED_LEVEL1_0    /* reserved for future / past use */
#define EMBEDDED_LEVEL2_0    /* reserved for future / past use */
#define FILE_ACCESS_MEMORY   /* alows reading files (FILE*), fread, etc ... */
#define READFILEDICO         /* Allows reading text dictionary files (dictl, dicm,dictman) */
#ifdef MALLOCED
# undef EMBEDDED
#endif

/* ============================== Logging ============================================*/
/* all off by default!!! */
#define DEBUG_DLSTTS_0    /* should be moved to a NLPE configuration (it is not global) */
#define SAVEPHOONFILE_0   /* Hack in NLPE + babile_demo to outoout phonetics -> should become obsolette (console_babile)*/
#define DEBUG_MALLOC_0  /* Log mallocs / free (with EMBEDDED) */
#define BBDEBUG_SPI_0   /* log BB_dba accesses (access to data, files) */
#define DEBUG_BNF_0      /* bnfdbg.c */
#define OGG_DEBUG_MALLOC_0 /* Log mallocs / free (with EMBEDDED - for ogg - tremor) */
#define NOWARNNING_0   /* removes some VC warnings */
#define COLIBRI_LOG_0  /* Colibri log */
#define __BABILE_TIMMING__0
#define __NLPE_TIMMING__0
#define __SELECTOR_TIMMING__0
/* handling of PRINT_MESSAGE, WARNING_MESSAGE, FATAL_MESSAGE */
#define NO_MESSAGES    /* removes messages *//* handling of PRINT_MESSAGE, WARNING_MESSAGE, FATAL_MESSAGE */


/* OBSOLETE, MISSPLACED, REMOVED defines: 
# define ARABICTTS => ??? seems unused
API_OLD_VERSION: obsolete
BIESIENLP: obsolete
DICTMANAGE: always true
CRYPT = not used
NLPEP_NLPE = not used
PHONETIZER = obsolette
ARM_ACA = obsolette
DEVEL_JA_JP = not used anymore
DEVEL_ZH_CN = not used anymore
*/

/* ============================= PLATFORM incompatible defines ============================== */
#ifdef ROMTOOLS		/* ROMDIC configuration */
	#define ROMDIC 1
	#undef EMBEDDED    /* uncompatible */
	#undef NO_MESSAGES
	#define FILE_ACCESS_MEMORY
	#define READFILEDIC
#endif
#ifdef ROMTREE
#       undef  FILE_ACCESS_MEMORY
#endif

#if defined(RESEARCH)
# if !defined(ROMTOOLS)
#  define DEVELOPMENT_CODE
#  define COLIBRI_RD
#  define COLIBRI_LOG
# endif
# undef EMBEDDED
# undef BBSELECTOR_COMPILEFLAG_OPUSENABLE
# undef BBNLP_COMPILEFLAG_TTFENABLE
# define BBSELECTOR_COMPILEFLAG_OPUSENABLE 0
#endif

#if defined(BB_DIRECTIV_USE_BABTTS_DEFS_H)
/* "BB_DIRECTIV_USE_BABTTS_DEFS_H" is defined, use the 
   PCProd/AcaMul/AcaTTS-specific common definitions. */
# if !defined(__BABTTS_DEFS_H__)
#  define __BABTTS_DEFS_H__ 1
#  define BBNLP_USED_BY_BABTTS
# endif
# undef BBNLP_COMPILEFLAG_TTFENABLE
# undef EMBEDDED
#endif

#if defined(WIN64) || defined(_WIN64) || defined(__LP64__) || defined(_LP64)  /* Not ported to 64 bits */
#  undef BBNLP_COMPILEFLAG_TTFENABLE
#  undef BBSELECTOR_COMPILEFLAG_OPUSENABLE
#endif

#ifdef __SYMBIAN32__  
#undef READFILEDICO
#endif

#ifdef LILY
# undef BBSELECTOR_COMPILEFLAG_OGGENABLE
# define BBSELECTOR_COMPILEFLAG_OGGENABLE 0
#endif

#if (defined(BBSELECTOR_ROPI_NOVIRTUAL_ONLYOGG) ) /* Polymorphism is not allowed in this mode */
	#if ((defined(BBSELECTOR_COMPILEFLAG_HNMENABLE) && BBSELECTOR_COMPILEFLAG_HNMENABLE==1) || (defined(BBSELECTOR_COMPILEFLAG_WAVENABLE) && BBSELECTOR_COMPILEFLAG_WAVENABLE==1) )
		#error
	#endif
#endif

#ifdef WIN32
# ifdef NOWARNNING
#  pragma warning (disable : 4100)		/* unreferenced formal parameter      */
#  pragma warning (disable : 4127)		/* conditional expression is constant */
#  pragma warning (disable : 4310)		/* cast truncates constant value      */
#  pragma warning (disable : 4710)		/* function  not inlined	*/
# endif
# pragma warning (disable : 4996)		/* was declared deprecated */
#endif

#ifndef NO_MESSAGES    /*  choose type of messages */
# define BBNLP_USED_BY_BABTTS_0
# define BB_CONSOLE     /* CONSOLE ( _CONSOLE) application OR (_WINDOWS + BABILSTATIC OR POCKETPC)*/
#endif 
#define __BB_DEFINES_DEFINED__

#endif /* __GENERIC_DEFS_H__ */

