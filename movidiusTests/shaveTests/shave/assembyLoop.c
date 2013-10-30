#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

void assemblyLoop() {
  int i;
  //int maxX = DATA_LENGTH >> 2;

  int4 * restrict v_input1 = (int4*) &(input1[0]);
  int4 * restrict v_input2 = (int4*) &(input2[0]);
  int4 * restrict v_output = (int4*) &(output[0]);

  __asm(
  "nop 5\n"
    "cmu.cpii i7 %0\n"
    "cmu.cpii i8 %1\n"
    "cmu.cpii i9 %2\n"
    ://Output registers
  :"r"(&v_input1[0]), "r"(&v_input2[0]), "r"(&v_output[0])//Input registers
    :"i7", "i8", "i9"//Clobbered registers
    );

  //for(i=0; i<(DATA_LENGTH>>2); i++) {
  //  //".set input1 v0\n"
  //  //  ".set input2 v1\n"

  //  __asm(
  //  //"lsu0.ld64.l v0 i7\n" // Load input1 into v0
  //  // "nop 5\n"
  //  // "lsu0.ldo64.h v0 i7 8\n"
  //  // "nop 5\n"
  //  // "iau.add i7, i7, 16\n"
  //  // "nop 5\n"
  //  // "lsu0.ld64.l v1 i8\n" // Load input2 into v1
  //  // "nop 5\n"
  //  // "lsu0.ldo64.h v1 i8 8\n"
  //  // "nop 5\n"
  //  // "iau.add i8, i8, 16\n"
  //  // "nop 5\n"
  //  // "vau.add.i32 v1 v1 v0 \n"
  //  // "nop 5\n"
  //  // "lsu0.st64.l v1 i9\n"
  //  // "nop 5\n"
  //  // "lsu0.sto64.h v1 i9 8\n"
  //  // "nop 5\n"
  //  // "iau.add i9, i9, 16\n"
  //  // "nop 5\n"

  //  //"lsu0.ld64.l v0 i7 || lsu1.ld64.l v1 i8 \n" // Load input1 into v0 and input2 into v1
  //  //  "lsu0.ldo64.h v0 i7 8 || lsu1.ldo64.h v1 i8 8 \n"
  //  //  "iau.add i7, i7, 16\n"
  //  //  "iau.add i8, i8, 16\n"
  //  //  "nop 3\n"
  //  //  "vau.add.i32 v1 v1 v0 \n"
  //  //  "nop 5\n"
  //  //  "lsu0.st64.l v1 i9\n"
  //  //  "nop 5\n"
  //  //  "lsu0.sto64.h v1 i9 8\n"
  //  //  "nop 5\n"
  //  //  "iau.add i9, i9, 16\n"

  //  //"lsu0.ld64.l v0 i7 || lsu1.ld64.l v1 i8 \n" // Load input1 into v0 and input2 into v1
  //  //  "lsu0.ldo64.h v0 i7 8 || lsu1.ldo64.h v1 i8 8 \n"
  //  //  "iau.add i7, i7, 16\n"
  //  //  "iau.add i8, i8, 16\n"
  //  //  "nop 3\n"
  //  //  "vau.add.i32 v1 v1 v0 \n"
  //  //  "nop 1\n"
  //  //  "lsu0.st64.l v1 i9 || lsu1.sto64.h v1 i9 8 || iau.add i9, i9, 16\n"

  //  "lsu0.ld64.l v0 i7 || lsu1.ld64.l v1 i8 \n" // Load input1 into v0 and input2 into v1
  //    "lsu0.ldo64.h v0 i7 8 || lsu1.ldo64.h v1 i8 8 \n"
  //    "iau.add i7, i7, 16\n"
  //    "iau.add i8, i8, 16\n"
  //    "nop 3\n"
  //    "vau.add.i32 v1 v1 v0 \n"
  //    "nop 1\n"
  //    "lsu0.st64.l v1 i9 || lsu1.sto64.h v1 i9 8 || iau.add i9, i9, 16\n"
  //    ://Output registers
  //  ://Input registers
  //  :"i7", "i8", "i9", "v0", "v1"//Clobbered registers
  //    );
  //}

  //__asm(
  //"lsu1.ldil i10 0x00fa\n"
  //  "___assemblyLoop_1:\n"
  //  "lsu0.ld64.l v0 i7 || lsu1.ld64.l v1 i8 \n" // Load input1 into v0 and input2 into v1
  //  "lsu0.ldo64.h v0 i7 8 || lsu1.ldo64.h v1 i8 8 || iau.incs i10 -1\n"// Also, decrement the loop counter
  //  "iau.add i7, i7, 16\n" // Update the input pointers
  //  "iau.add i8, i8, 16 || cmu.cmz.i32 i10\n"// Check if we're done with the loop
  //  "peu.pc1c neq || bru.bra ___assemblyLoop_1\n"
  //  "nop 2\n"
  //  "vau.add.i32 v1 v1 v0 \n"
  //  "nop 1\n"
  //  "lsu0.st64.l v1 i9 || lsu1.sto64.h v1 i9 8 || iau.add i9, i9, 16\n"
  //  ://Output registers
  //://Input registers
  //:"i7", "i8", "i9", "i10", "v0", "v1"//Clobbered registers
  //  );

  __asm(
  "lsu1.ldil i10 0x1388\n" // Set the loop counter
    "lsu0.ld64.l v0 i7 || lsu1.ld64.l v1 i8 \n" // Load input1 into v0 and input2 into v1
    "lsu0.ldo64.h v0 i7 8 || lsu1.ldo64.h v1 i8 8 \n"
    "iau.add i7, i7, 16 \n"
    "iau.add i8, i8, 16 \n"
    "iau.incs i10 -1\n"
    "nop 2 \n"
    "\n"
    "___assemblyLoop_1:\n"
    "vau.add.i32 v2 v1 v0 || lsu0.ld64.l v3 i7 \n"
    "lsu1.ld64.l v4 i8 \n"
    "lsu0.ldo64.h v3 i7 8 \n"
    "lsu1.ldo64.h v4 i8 8 \n"
    "lsu0.st64.l v2 i9 || iau.add i7, i7, 16 || cmu.cmz.i32 i10 \n"
    "lsu1.sto64.h v2 i9 8 || iau.add i8, i8, 16 || peu.pc1c neq || bru.bra ___assemblyLoop_1\n"
    "iau.incs i10 -1 \n"
    "iau.add i9, i9, 16 \n"
    "nop 1 \n"
    "cmu.cpvv v0 v3 \n"
    "cmu.cpvv v1 v4 \n"
    ://Output registers
  ://Input registers
  :"i7", "i8", "i9", "i10", "v0", "v1"//Clobbered registers
    );
}