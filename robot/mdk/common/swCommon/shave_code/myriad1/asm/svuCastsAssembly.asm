; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   Casts assembly file 
; ///

.version 00.51.05
;-------------------------------------------------------------------------------

.code .text.char8CastToshort8
char8CastToshort8:
    bru.jmp i30
    //MoviCompile uses char8 set in v23 same as short8 elements
    //So shuffle elements in place for cmu.cpvv.i8.i16 first
    cmu.vszmbyte v23, v23, [Z2Z0]
    cmu.cpvv.u16.u8s v23, v23
    cmu.cpvv.i8.i16 v23, v23
    nop 2

.code .text.uchar8CastToushort8
uchar8CastToushort8:
    bru.jmp i30
    //MoviCompile uses char8 set in v23 same as short8 elements
    //So shuffle elements in place for cmu.cpvv.i8.i16 first
    cmu.vszmbyte v23, v23, [Z2Z0]
    cmu.cpvv.u16.u8s v23, v23
    cmu.cpvv.u8.u16 v23, v23
    nop 2

.code .text.short4CastToint4
short4CastToint4:
    bru.jmp i30
    cmu.vszmbyte v23, v23, [ZZ10]
    cmu.cpvv.u32.u16s v23, v23
    CMU.CPVV.i16.i32 v23, v23
    nop 2

.code .text.ushort4CastTouint4
ushort4CastTouint4:
    bru.jmp i30
    cmu.vszmbyte v23, v23, [ZZ10]
    nop 4
    
.code .text.char4CastToint4
char4CastToint4:
    bru.jmp i30
    CMU.CPVV.i8.i32 v23, v23
    nop 4

.code .text.int4CastToshort4
int4CastToshort4:
    bru.jmp i30
    cmu.vszmbyte v23, v23, [ZZ10]    
    CMU.CPVV v23, v23
    nop 3
    
.code .text.short8CastTochar8
short8CastTochar8:
    bru.jmp i30
    cmu.vszmbyte v23, v23, [Z2Z0]
    nop 4
    
.code .text.int4CastTochar4
int4CastTochar4:
    bru.jmp i30
    CMU.CPVV.i32.i8s v23, v23
    nop 4

.code .text.uint4CastToushort4
uint4CastToushort4:
    bru.jmp i30
    CMU.vszmbyte v23, v23, [ZZ10]
    nop 4
    
.code .text.ushort8CastTouchar8
ushort8CastTouchar8:
    bru.jmp i30
    cmu.vszmbyte v23, v23, [Z2Z0]
    nop 4
    
.code .text.uint4CastTouchar4
uint4CastTouchar4:
    bru.jmp i30
    CMU.CPVV.u32.u8s v23, v23
    nop 4 
    
.end
