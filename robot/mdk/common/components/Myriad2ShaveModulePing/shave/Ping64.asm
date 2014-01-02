.version 00.51.04

; i18,    i17,                   i16,                 i15, i14
;int pingRegister64(u32 regAddress,u32 writeHi, u32 writeLo,u32 expectedHi, u32 expectedLo)
;{
;    int numErrors = 0;
;    u64 valueToWrite,u64 valueExpected
;    u64 readDWord;;
;
;    valueToWrite  = (writeHi << 32) + writeLo;
;    valueExpected = (expectedHi << 32) + expectedLo;
;
;    // Check Register using LSU0
;    shaveWrite64Lsu0(regAddress, valueToWrite);
;    readDWord = shaveRead64Lsu0(regAddress);
;    numErrors += (readDWord == valueExpected ? 0:1);
;    shaveWrite64Lsu0(regAddress, 0x0);            // Reset register to zero before we test access using LSU1
;
;    // Check Register using LSU1
;    shaveWrite64Lsu1(regAddress, valueToWrite);
;    readDWord = shaveRead64Lsu1(regAddress);
;    numErrors += (readDWord == valueExpected ? 0:1);
;    return numErrors;
;}

.code .text.pingRegister64
.salign 16

pingRegister64:
	lsu0.ldil i9, 0x0000
	lsu1.ldil i10, 0x0000
	cmu.cpii i0, i16
	cmu.cpii i1, i17
	lsu0.st.64 i0, i18
	nop
	lsu0.ld.64 i2, i18
	nop 8
	iau.xor i4, i2, i14
	iau.xor i5, i3, i15
	IAU.ONES i6, i4
	IAU.ONES i7, i5
	iau.add i8, i7, i6
	;  // Reset register to zero before we test access using LSU1
	lsu0.st.64 i9, i18
	nop
	lsu1.st.64 i0, i18
	nop
	lsu1.ld.64 i2, i18
	nop 8
	iau.xor i4, i2, i14
	iau.xor i5, i3, i15
	IAU.ONES i6, i4
	IAU.ONES i7, i5
	iau.add i11, i7, i6
	
	; update return value
	iau.add i18, i8, i11
	; have to be call from a C function from shave a
	bru.jmp i30
	nop 6

