#ifndef __PLATFORM_H__
#define __PLATFORM_H__

//till MDK people wake up, I put these defs here...
//they should be in mvtypes
typedef unsigned long long UInt64;
typedef unsigned int       UInt32;
typedef unsigned short     UInt16;
typedef unsigned char      UInt8;

typedef          int       Int32;
typedef          short     Int16;
typedef          char      Int8;

#if defined(__sparc)
  #define INLINE inline
  #define ALIGNED(x) __attribute__((aligned(x)))
  #define SECTION(x) __attribute__((section(x)))
  #define DBG_PRINT(x...)
#else // PC world
  #define INLINE     //__inline
  #define ALIGNED(x) //nothing
  #define SECTION(x) //nothing
  #define DBG_PRINT printf
#endif

//#############################################################################
#if defined(__sparc) && !defined(MYRIAD2) //Thus SPARC for Myriad1 (Sabre)
 
 #include <isaac_registers.h>    // various registers used under the hood by some functions
 #include <stdio.h>              // printf
 #include <stdlib.h>             // memset, memcpy, exit
 #include <swcShaveLoader.h>
 #include <swcLeonUtils.h>
 #include <swcSliceUtils.h>
 #include <swcMemoryTransfer.h>

 #define VCS_PRINT_INT(a)        //nothing
 
 typedef unsigned short half; //Leon needs to know storage requirements
 #define WRESOLVE(addr,slice)    ((void *)swcSolveShaveRelAddrAHB((UInt32)addr, slice))
 #define CONCAT(A,B) A ## B
 #define SVU_SYM(s) CONCAT(SS_,s)  // moviLink adds extra "SS_",
 #define SHAVE_MAIN main            //Leon will see something like SS__main through SVU_SYM macro
 #define memcpy                  swcU32memcpy //Gizas ... memcpy reported undefined on Leon
 #define sipp_memset             swcU32memsetU32

 #define SIPP_MBIN(x)            (x)
 #define SIPP_WINREGS(x)         (x)

 #define DDR_TEXT   __attribute__((section(".ddr_direct.text")))
 #define DDR_DATA   __attribute__((section(".ddr_direct.data")))
 #define DDR_RODATA __attribute__((section(".ddr_direct.rodata")))
 #define DDR_BSS    __attribute__((section(".ddr_direct.bss")))
 #define CMX_TEXT   __attribute__((section(".cmx.text")))
 #define CMX_RODATA __attribute__((section(".cmx.rodata")))
 #define CMX_DATA   __attribute__((section(".cmx.data")))
 #define CMX_BSS    __attribute__((section(".cmx.bss")))
 
//#############################################################################
#elif defined(__sparc) && defined(MYRIAD2) //Thus SPARC for Myriad2 
 #include <sippHwMacros.h>
 #include <registersMyriad2.h>    //various registers used under the hood by some functions
 #include <swcLeonUtilsDefines.h> //for NOP

#if defined(SIPP_VCS)
 #include <UnitTestApi.h>
 #include <VcsHooksApi.h>
#endif

 #include <stdio.h>               //printf
 #include <swcSliceUtils.h>
 #include <swcShaveLoader.h>
 #include <swcMemoryTransfer.h>
 
 #if defined(SIPP_VCS)
 #define VCS_PRINT_INT(a) printInt((UInt32)a)
 #else
 #define VCS_PRINT_INT(a)        //nothing
 #endif

 typedef unsigned short half; //Leon needs to know storage requirements
 void   swcU32memcpy(u32* dst, u32* src, u32 len);
 void   swcU32memsetU32(u32 *addr, u32 lenB, u32 val);
 
 void   sippSetShaveWindow(UInt32 svuNo, UInt32 winNo, UInt32 value);
 void   sippLoadMbin     (u8 *mbinImg, u32 targetS);
 void   sippLoadMbinOpt  (u8 *mbinImg, u32 firstSlc, u32 lastSlc);
 void   sippLoadMbinMulti(u8 *mbinImg, u32 firstSlc, u32 lastSlc);
 
 UInt32 sippSolveShaveAddr(u32 inAddr, u32 sliceNo);
  
 #define WRESOLVE(addr,slice)    ((void *)sippSolveShaveAddr((UInt32)addr, slice))
 #define CONCAT(A,B) A ## B
 #define SVU_SYM(s) CONCAT(SS_,s)   // moviLink adds extra "SS_"
 #define SHAVE_MAIN main            //Leon will see something like SS__main through SVU_SYM macro
 #define memcpy                  swcU32memcpy //Gizas ... memcpy reported undefined on Leon
 #define sipp_memset             swcU32memsetU32

 #define SIPP_MBIN(x)            (x)
 #define SIPP_WINREGS(x)         (x)

//for now dummy
 #define DDR_TEXT   
 #define DDR_DATA     __attribute__((section(".ddr.data")))
 #define DDR_RODATA 
 #define DDR_BSS    
 #define CMX_TEXT   
 #define CMX_RODATA 
 #define CMX_DATA   
 #define CMX_BSS   

 void sippWaitHWUnit(UInt32 id);

 //#############################################################################
#elif defined(__MOVICOMPILE__)
 //On Shave, Compiler knows half data type by default
 #include <string.h>
 #include <stdint.h>
 #define WRESOLVE(addr,slice)    (addr) //On Shave, do nothing (the hardware does it)
 #define SVU_SYM(s) s
 #define SHAVE_MAIN main     //on Sabre, this is the main function
 typedef half Half;

 //#############################################################################
#else //PC world
 
 #define SIPP_PC

 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>
 #include <stdlib.h>
 #include <math.h>
 #include "sippHw.h"
 
 typedef fp16 half;
 typedef fp16 Half;
 void *  sipp_mem_resolve_windowed_addr(void *addr, int slice);
 #define WRESOLVE(addr,slice)    sipp_mem_resolve_windowed_addr(addr, slice)
 #define SVU_SYM(s)              s
 #define SHAVE_MAIN              shave_main 
 #define sipp_memset             memset

 #define SIPP_MBIN(x)            0
 #define SIPP_WINREGS(x)         0
 
 #define VCS_PRINT_INT(a)        //nothing
 //#define MXI_CMX_BASE_ADR        0x10000000 //what really matters is that lower 24 bits are ZERO

 #define CMX_TEXT   //dummy
 #define CMX_RODATA //dummy
 #define CMX_DATA   //dummy
 #define CMX_BSS    //dummy
 #define DDR_TEXT   //dummy
 #define DDR_RODATA //dummy 
 #define DDR_DATA   //dummy
 #define DDR_BSS    //dummy

 #define NOP
#endif




#if defined(SIPP_PC)
 //on PC, can use larger slice size for frame check (must be <= 16MB)
  //#define SIPP_SLICE_SZ (16*1024*1024)
    #define SIPP_SLICE_SZ (128*1024)
#else
  #if !defined(SIPP_SLICE_SZ)
  #define SIPP_SLICE_SZ (128*1024)
  #endif
#endif

//an assertion: virtual slice can't exceed 24 bits of addr
//available through a window register
#if (SIPP_SLICE_SZ > 16*1024*1024)
 #error EXCEEDING_24_bits_of_ADDR
#endif

#endif // __PLATFORM_H__
