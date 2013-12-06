#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveKernels.h"

#define INNER_LOOP_VERSION 1

void predicateTests()
{
  const s32 lower = 2;
  const s32 upper = 6;

#if INNER_LOOP_VERSION == 1
  s32 x =-2, y=-3, v=-4;

  __asm(
  ".set test i2 \n"
    ".set x %0 \n"
    ".set y %1 \n"
    ".set v %2 \n"

    ".set iEnd i20 \n"
    ".set i i21 \n"

    "lsu0.ldil i 0xFFFE ; i=-2 \n"
    "lsu0.ldih i 0xFFFF \n"
    "lsu0.ldil iEnd 0x0008 ; iEnd=8 \n"

    "; for(x=3; x<xEnd; x++) \n"
    ".lalign \n"
    "_loopy: \n"

    "nop 5 \n"

    "cmu.cpii x i \n"
    "lsu0.ldil y 0x0001 ; y=1 \n"
    "lsu0.ldil v 0xFFF6 ; v=-10 \n"
    "lsu0.ldih v 0xFFFF \n"

    //"iau.sub test x y \n"
    "iau.add test x y \n"
    "peu.pc1i gte || cmu.cpii v x \n" // LT-2 GT-1 EQ-0

    // printf
    "LSU1.LDIL i30 printf ||  LSU0.LDIL i17 ___.str1 \n"
    "LSU1.STO32 v i19 48 ||  LSU0.LDIH i30 printf \n"
    "LSU1.LDIH i17 ___.str1 ||  LSU0.STO32 y i19 32 \n"
    "LSU1.STO32 x i19 16 ||  LSU0.STO32 i17 i19 0  \n"
    "BRU.SWP i30 \n"
    "NOP 5 \n"

    "iau.add i i 1 \n"
    "cmu.cmii.i32 i iEnd \n"
    "peu.pc1c lt || bru.bra _loopy \n"
    "nop 5 \n"

    //".data .data.__static_data_P98b45870_c3f9d440T_ \n"
    //".salign	 16 \n"
    //"___.str: \n"
    //".byte	 101, 113, 41, 32, 120, 58, 37, 100, 32, 121, 58, 37, 100, 32, 118, 58 \n" // "eq) x:%d y:%d v:%d\n"
    //".byte	 37, 100, 10, 0 \n"
    //".end \n"
    : //Output registers
  : "r"(x), "r"(y), "r"(v) //Input registers
    : "i20", "i21" //Clobbered registers
    );

  printf("\ndone\n");
  printf("eq) x:%d y:%d v:%d\n", (int)x, (int)y, (int)v);

#endif // INNER_LOOP_VERSION
}