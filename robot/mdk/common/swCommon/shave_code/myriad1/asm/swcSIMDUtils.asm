; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.05
;-------------------------------------------------------------------------------

; FP vector types

.code .text.swcSIMDAbs4F32
; float4 swcSIMDAbs4F32(float4 in_vec);
swcSIMDAbs4F32:
	BRU.JMP i30
	VAU.ABS.F32 v23 v23
	nop 4

.code .text.swcSIMDAbs2F32
; float2 swcSIMDAbs2F32(float2 in_vec);
swcSIMDAbs2F32:
	BRU.JMP i30
	VAU.ABS.F32 v23 v23
	nop 4

.code .text.swcSIMDAbs8F16
; half8 swcSIMDAbs8F16(half8 in_vec);
swcSIMDAbs8F16:
	BRU.JMP i30
	VAU.ABS.F16 v23 v23
	nop 4

.code .text.swcSIMDAbs4F16
; half4 swcSIMDAbs4F16(half4 in_vec);
swcSIMDAbs4F16:
	BRU.JMP i30
	VAU.ABS.F16 v23 v23
	nop 4

.code .text.swcSIMDAbs2F16
; half2 swcSIMDAbs2F16(half2 in_vec);
swcSIMDAbs2F16:
	BRU.JMP i30
	VAU.ABS.F16 v23 v23
	nop 4

; INTEGER vector types

.code .text.swcSIMDAbs4I32
; int4 swcSIMDAbs4I32(int4 in_vec);
swcSIMDAbs4I32:
	BRU.JMP i30
	VAU.ABS.I32 v23 v23
	nop 4

.code .text.swcSIMDAbs3I32
; int3 swcSIMDAbs3I32(int3 in_vec);
swcSIMDAbs3I32:
	BRU.JMP i30
	VAU.ABS.I32 v23 v23
	nop 4

.code .text.swcSIMDAbs2I32
; int2 swcSIMDAbs2I32(int2 in_vec);
swcSIMDAbs2I32:
	BRU.JMP i30
	VAU.ABS.I32 v23 v23
	nop 4

.code .text.swcSIMDAbs8I16
; short8 swcSIMDAbs8I16(short8 in_vec);
swcSIMDAbs8I16:
	BRU.JMP i30
	VAU.ABS.I16 v23 v23
	nop 4

.code .text.swcSIMDAbs4I16
; short4 swcSIMDAbs4I16(short4 in_vec);
swcSIMDAbs4I16:
	BRU.JMP i30
	VAU.ABS.I16 v23 v23
	nop 4

.code .text.swcSIMDAbs2I16
; short2 swcSIMDAbs2I16(short2 in_vec);
swcSIMDAbs2I16:
	BRU.JMP i30
	VAU.ABS.I16 v23 v23
	nop 4

.code .text.swcSIMDAbs16I8
; char16 swcSIMDAbs16I8(char16 in_vec);
swcSIMDAbs16I8:
	BRU.JMP i30
	VAU.ABS.I8 v23 v23
	nop 4

.code .text.swcSIMDAbs8I8
; char8 swcSIMDAbs8I8(char8 in_vec);
swcSIMDAbs8I8:
	BRU.JMP i30
	VAU.ABS.I8 v23 v23
	nop 4

.code .text.swcSIMDAbs4I8
; char4 swcSIMDAbs4I8(char4 in_vec);
swcSIMDAbs4I8:
	BRU.JMP i30
	VAU.ABS.I8 v23 v23
	nop 4

.code .text.swcSIMDAbs2I8
; char2 swcSIMDAbs2I8(char2 in_vec);
swcSIMDAbs2I8:
	BRU.JMP i30
	VAU.ABS.I8 v23 v23
	nop 4

; MIN for all types supported by MoviCompiler

.code .text.swcSIMDMin4F32
;float4 swcSIMDMin4F32(float4 in_vec1, float4 in_vec2);
swcSIMDMin4F32:
	BRU.JMP i30
	CMU.MIN.F32 v23 v23 v22
	nop 4


.code .text.swcSIMDMin2F32
;float2 swcSIMDMin2F32(float2 in_vec1, float2 in_vec2);
swcSIMDMin2F32:
	BRU.JMP i30
	CMU.MIN.F32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4I32
; int4 swcSIMDMin4I32(int4 in_vec1, int4 in_vec2);
swcSIMDMin4I32:
	BRU.JMP i30
	CMU.MIN.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin3I32
; int3 swcSIMDMin3I32(int3 in_vec1, int3 in_vec2);
swcSIMDMin3I32:
	BRU.JMP i30
	CMU.MIN.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2I32
; int2 swcSIMDMin2I32(int2 in_vec1, int2 in_vec2);
swcSIMDMin2I32:
	BRU.JMP i30
	CMU.MIN.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4U32
; uint4 swcSIMDMin4U32(uint4 in_vec1, uint4 in_vec2);
swcSIMDMin4U32:
	BRU.JMP i30
	CMU.MIN.U32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin3U32
; uint3 swcSIMDMin3U32(uint3 in_vec1, uint3 in_vec2);
swcSIMDMin3U32:
	BRU.JMP i30
	CMU.MIN.U32 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2U32
; uint2 swcSIMDMin2U32(uint2 in_vec1, uint2 in_vec2);
swcSIMDMin2U32:
	BRU.JMP i30
	CMU.MIN.U32 v23 v23 v22
	nop 4


.code .text.swcSIMDMin8F16
; half8 swcSIMDMin8F16(half8 in_vec1, half8 in_vec2);
swcSIMDMin8F16:
	BRU.JMP i30
	CMU.MIN.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4F16
; half4 swcSIMDMin4F16(half4 in_vec1, half4 in_vec2);
swcSIMDMin4F16:
	BRU.JMP i30
	CMU.MIN.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2F16
; half2 swcSIMDMin2F16(half2 in_vec1, half2 in_vec2);
swcSIMDMin2F16:
	BRU.JMP i30
	CMU.MIN.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin8I16
; short8 swcSIMDMin8I16(short8 in_vec1, short8 in_vec2);
swcSIMDMin8I16:
	BRU.JMP i30
	CMU.MIN.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4I16
; short4 swcSIMDMin4I16(short4 in_vec1, short4 in_vec2);
swcSIMDMin4I16:
	BRU.JMP i30
	CMU.MIN.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2I16
; short2 swcSIMDMin2I16(short2 in_vec1, short2 in_vec2);
swcSIMDMin2I16:
	BRU.JMP i30
	CMU.MIN.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin8U16
; ushort8 swcSIMDMin8U16(ushort8 in_vec1, ushort8 in_vec2);
swcSIMDMin8U16:
	BRU.JMP i30
	CMU.MIN.U16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4U16
; ushort4 swcSIMDMin4U16(ushort4 in_vec1, ushort4 in_vec2);
swcSIMDMin4U16:
	BRU.JMP i30
	CMU.MIN.U16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2U16
; ushort2 swcSIMDMin2U16(ushort2 in_vec1, ushort2 in_vec2);
swcSIMDMin2U16:
	BRU.JMP i30
	CMU.MIN.U16 v23 v23 v22
	nop 4

.code .text.swcSIMDMin16I8
; char16 swcSIMDMin16I8(char16 in_vec1, char16 in_vec2);
swcSIMDMin16I8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin8I8
; char8 swcSIMDMin8I8(char8 in_vec1, char8 in_vec2);
swcSIMDMin8I8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4I8
; char4 swcSIMDMin4I8(char4 in_vec1, char4 in_vec2);
swcSIMDMin4I8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2I8
; char2 swcSIMDMin2I8(char2 in_vec1, char2 in_vec2);
swcSIMDMin2I8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin16U8
; uchar16 swcSIMDMin16U8(uchar16 in_vec1, uchar16 in_vec2);
swcSIMDMin16U8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin8U8
; uchar8 swcSIMDMin8U8(uchar8 in_vec1, uchar8 in_vec2);
swcSIMDMin8U8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin4U8
; uchar4 swcSIMDMin4U8(uchar4 in_vec1, uchar4 in_vec2);
swcSIMDMin4U8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMin2U8
; uchar2 swcSIMDMin2U8(uchar2 in_vec1, uchar2 in_vec2);
swcSIMDMin2U8:
	BRU.JMP i30
	CMU.MIN.I8 v23 v23 v22
	nop 4

; MAX for all types supported by MoviCompiler

.code .text.swcSIMDMax4F32
; float4 swcSIMDMax4F32(float4 in_vec1, float4 in_vec2);
swcSIMDMax4F32:
	BRU.JMP i30
	CMU.MAX.F32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2F32
; float2 swcSIMDMax2F32(float2 in_vec1,float2 in_vec2);
swcSIMDMax2F32:
	BRU.JMP i30
	CMU.MAX.F32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4I32
; int4 swcSIMDMax4I32(int4 in_vec1, int4 in_vec2);
swcSIMDMax4I32:
	BRU.JMP i30
	CMU.MAX.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax3I32
; int3 swcSIMDMax3I32(int3 in_vec1, int3 in_vec2);
swcSIMDMax3I32:
	BRU.JMP i30
	CMU.MAX.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2I32
; int2 swcSIMDMax2I32(int2 in_vec1, int2 in_vec2);
swcSIMDMax2I32:
	BRU.JMP i30
	CMU.MAX.I32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4U32
; uint4 swcSIMDMax4U32(uint4 in_vec1, uint4 in_vec2);
swcSIMDMax4U32:
	BRU.JMP i30
	CMU.MAX.U32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax3U32
; uint3 swcSIMDMax3U32(uint3 in_vec1, uint3 in_vec2);
swcSIMDMax3U32:
	BRU.JMP i30
	CMU.MAX.U32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2U32
; uint2 swcSIMDMax2U32(uint2 in_vec1, uint2 in_vec2);
swcSIMDMax2U32:
	BRU.JMP i30
	CMU.MAX.U32 v23 v23 v22
	nop 4

.code .text.swcSIMDMax8F16
; half8 swcSIMDMax8F16(half8 in_vec1, half8 in_vec2);
swcSIMDMax8F16:
	BRU.JMP i30
	CMU.MAX.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4F16
; half4 swcSIMDMax4F16(half4 in_vec1, half4 in_vec2);
swcSIMDMax4F16:
	BRU.JMP i30
	CMU.MAX.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2F16
; half2 swcSIMDMax2F16(half2 in_vec1, half2 in_vec2);
swcSIMDMax2F16:
	BRU.JMP i30
	CMU.MAX.F16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax8I16
; short8 swcSIMDMax8I16(short8 in_vec1, short8 in_vec2);
swcSIMDMax8I16:
	BRU.JMP i30
	CMU.MAX.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4I16
; short4 swcSIMDMax4I16(short4 in_vec1, short4 in_vec2);
swcSIMDMax4I16:
	BRU.JMP i30
	CMU.MAX.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2I16
; short2 swcSIMDMax2I16(short2 in_vec1, short2 in_vec2);
swcSIMDMax2I16:
	BRU.JMP i30
	CMU.MAX.I16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax8U16
; ushort8 swcSIMDMax8U16(ushort8 in_vec1, ushort8 in_vec2);
swcSIMDMax8U16:
	BRU.JMP i30
	CMU.MAX.U16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4U16
; ushort4 swcSIMDMax4U16(ushort4 in_vec1, ushort4 in_vec2);
swcSIMDMax4U16:
	BRU.JMP i30
	CMU.MAX.U16 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2U16
; ushort2 swcSIMDMax2U16(ushort2 in_vec1, ushort2 in_vec2);
swcSIMDMax2U16:
	BRU.JMP i30
	CMU.MAX.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax16I8
; char16 swcSIMDMax16I8(char16 in_vec1, char16 in_vec2);
swcSIMDMax16I8:
	BRU.JMP i30
	CMU.MAX.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax8I8
; char8 swcSIMDMax8I8(char8 in_vec1, char8 in_vec2);
swcSIMDMax8I8:
	BRU.JMP i30
	CMU.MAX.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4I8
; char4 swcSIMDMax4I8(char4 in_vec1, char4 in_vec2);
swcSIMDMax4I8:
	BRU.JMP i30
	CMU.MAX.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2I8
; char2 swcSIMDMax2I8(char2 in_vec1, char2 in_vec2);
swcSIMDMax2I8:
	BRU.JMP i30
	CMU.MAX.I8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax16U8
; uchar16 swcSIMDMax16U8(char16 in_vec1, char16 in_vec2);
swcSIMDMax16U8:
	BRU.JMP i30
	CMU.MAX.U8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax8U8
; uchar8 swcSIMDMax8U8(char8 in_vec1, char8 in_vec2);
swcSIMDMax8U8:
	BRU.JMP i30
	CMU.MAX.U8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax4U8
; uchar4 swcSIMDMax4U8(char4 in_vec1, char4 in_vec2);
swcSIMDMax4U8:
	BRU.JMP i30
	CMU.MAX.U8 v23 v23 v22
	nop 4

.code .text.swcSIMDMax2U8
;uchar2 swcSIMDMax2U8(char2 in_vec1, char2 in_vec2);
swcSIMDMax2U8:
	BRU.JMP i30
	CMU.MAX.U8 v23 v23 v22
	nop 4

; SAU.SUM for vector types

.code .text.swcSIMDSum4F32
;fp32 swcSIMDSum4F32(float4 in_vec);
swcSIMDSum4F32:
	BRU.JMP i30
	SAU.SUM.F32 s23 v23 0xf
	nop 4

.code .text.swcSIMDSumAbs4F32
;fp32 swcSIMDSumAbs4F32(float4 in_vec);
swcSIMDSumAbs4F32:
	BRU.JMP i30
	SAU.SUMABS.F32 s23 v23 0xf
	nop 4


.code .text.swcSIMDSum8F16
;fp16 swcSIMDSum8F16(half8 in_vec);
swcSIMDSum8F16:
	BRU.JMP i30
	SAU.SUM.F16 s23 v23 0xf
	nop 4

.code .text.swcSIMDSumAbs8F16
;fp16 swcSIMDSumAbs8F16(half8 in_vec);
swcSIMDSumAbs8F16:
	BRU.JMP i30
	SAU.SUMABS.F16 s23 v23 0xf
	nop 4

.code .text.swcSIMDSum4U32
;u32 swcSIMDSum4U32(uint4 in_vec);
swcSIMDSum4U32:
	BRU.JMP i30
	SAU.SUM.U32 i18 v23 0xf
	nop 4

.code .text.swcSIMDSum4I32
;s32 swcSIMDSum4I32(int4 in_vec);
swcSIMDSum4I32:
	BRU.JMP i30
	SAU.SUM.I32 i18 v23 0xf
	nop 4

.code .text.swcSIMDSum8U16
;u32 swcSIMDSum8U16(ushort8 in_vec);
swcSIMDSum8U16:
	BRU.JMP i30
	SAU.SUM.U16 i18 v23 0xf
	nop 4

.code .text.swcSIMDSum8I16
;s32 swcSIMDSum8I16(short8 in_vec);
swcSIMDSum8I16:
	BRU.JMP i30
	SAU.SUM.I16 i18 v23 0xf
	nop 4

.code .text.swcSIMDSum16U8
;u32 swcSIMDSum16U8(uchar16 in_vec);
swcSIMDSum16U8:
	BRU.JMP i30
	SAU.SUM.U8 i18 v23 0xf
	nop 4

.code .text.swcSIMDSum16I8
;s32 swcSIMDSum16I8(char16 in_vec);
swcSIMDSum16I8:
	BRU.JMP i30
	SAU.SUM.I8 i18 v23 0xf
	nop 4
