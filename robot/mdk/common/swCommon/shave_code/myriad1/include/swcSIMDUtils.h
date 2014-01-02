///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief
///

#ifndef __SWC_SIMD_UTILS__H_
#define __SWC_SIMD_UTILS__H_

#ifndef __CLCTYPES_H__
#include <moviVectorUtils.h>
#endif

//Including mv_types for u32 and s32 support
#include <mv_types.h>

// FP vector types

/// This will compute the absolute value of vector
/// @param[in] in_vec float4 vector received as input
/// @return float4  vector
#ifdef __cplusplus
extern "C"
#endif
float4 swcSIMDAbs4F32(float4 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec float2 vector received as input
/// @return float2 vector
#ifdef __cplusplus
extern "C"
#endif
float2 swcSIMDAbs2F32(float2 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec half8 vector received as input
/// @return half8  vector
#ifdef __cplusplus
extern "C"
#endif
half8 swcSIMDAbs8F16(half8 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec half4 vector received as input
/// @return half4  vector
#ifdef __cplusplus
extern "C"
#endif
half4 swcSIMDAbs4F16(half4 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec half2 vector received as input
/// @return half2 vector
#ifdef __cplusplus
extern "C"
#endif
half2 swcSIMDAbs2F16(half2 in_vec);

// Integer vector types
/// This will compute the absolute value of vector
/// @param[in] in_vec int4 vector received as input
/// @return int4  vector
#ifdef __cplusplus
extern "C"
#endif
int4 swcSIMDAbs4I32(int4 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec int3 vector received as input
/// @return int3  vector
#ifdef __cplusplus
extern "C"
#endif
int3 swcSIMDAbs3I32(int3 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec int2 vector received as input
/// @return int2  vector
#ifdef __cplusplus
extern "C"
#endif
int2 swcSIMDAbs2I32(int2 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec short8 vector received as input
/// @return short8  vector
#ifdef __cplusplus
extern "C"
#endif
short8 swcSIMDAbs8I16(short8 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec short4 vector received as input
/// @return short4  vector
#ifdef __cplusplus
extern "C"
#endif
short4 swcSIMDAbs4I16(short4 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec short2 vector received as input
/// @return short2 vector
#ifdef __cplusplus
extern "C"
#endif
short2 swcSIMDAbs2I16(short2 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec char16 vector received as input
/// @return char16 vector
#ifdef __cplusplus
extern "C"
#endif
char16 swcSIMDAbs16I8(char16 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec char8 vector received as input
/// @return char8  vector
#ifdef __cplusplus
extern "C"
#endif
char8 swcSIMDAbs8I8(char8 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec char4 vector received as input
/// @return char4  vector
#ifdef __cplusplus
extern "C"
#endif
char4 swcSIMDAbs4I8(char4 in_vec);

/// This will compute the absolute value of vector
/// @param[in] in_vec - char2 vector received as input
/// @return char2  vector
#ifdef __cplusplus
extern "C"
#endif
char2 swcSIMDAbs2I8(char2 in_vec);



/// This will compute the minumum value of vector
/// @param[in] in_vec1 - float4 vector received as input
/// @param[in] in_vec2 - float4 vector received as input
/// @return float4  vector
#ifdef __cplusplus
extern "C"
#endif
float4 swcSIMDMin4F32(float4 in_vec1, float4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - float2 vector received as input
/// @param[in] in_vec2 - float2 vector received as input
/// @return float2  vector
#ifdef __cplusplus
extern "C"
#endif
float2 swcSIMDMin2F32(float2 in_vec1, float2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - int4 vector received as input
/// @param[in] in_vec2 - int4 vector received as input
/// @return int4  vector
#ifdef __cplusplus
extern "C"
#endif
int4 swcSIMDMin4I32(int4 in_vec1, int4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - int3 vector received as input
/// @param[in] in_vec2 - int3 vector received as input
/// @return int3  vector
#ifdef __cplusplus
extern "C"
#endif
int3 swcSIMDMin3I32(int3 in_vec1, int3 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - int2 vector received as input
/// @param[in] in_vec2 - int2 vector received as input
/// @return int2  vector
#ifdef __cplusplus
extern "C"
#endif
int2 swcSIMDMin2I32(int2 in_vec1, int2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uint4 vector received as input
/// @param[in] in_vec2 - uint4 vector received as input
/// @return uint4  vector
#ifdef __cplusplus
extern "C"
#endif
uint4 swcSIMDMin4U32(uint4 in_vec1, uint4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uint3 vector received as input
/// @param[in] in_vec2 - uint3 vector received as input
/// @return uint3  vector
#ifdef __cplusplus
extern "C"
#endif
uint3 swcSIMDMin3U32(uint3 in_vec1, uint3 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uint2 vector received as input
/// @param[in] in_vec2 - uint2 vector received as input
/// @return uint2  vector
#ifdef __cplusplus
extern "C"
#endif
uint2 swcSIMDMin2U32(uint2 in_vec1, uint2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - half8 vector received as input
/// @param[in] in_vec2 - half8 vector received as input
/// @return half8  vector
#ifdef __cplusplus
extern "C"
#endif
half8 swcSIMDMin8F16(half8 in_vec1, half8 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - half4 vector received as input
/// @param[in] in_vec2 - half4 vector received as input
/// @return half4  vector
#ifdef __cplusplus
extern "C"
#endif
half4 swcSIMDMin4F16(half4 in_vec1, half4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - half2 vector received as input
/// @param[in] in_vec2 - half2 vector received as input
/// @return half2  vector
#ifdef __cplusplus
extern "C"
#endif
half2 swcSIMDMin2F16(half2 in_vec1, half2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - short8 vector received as input
/// @param[in] in_vec2 - short8 vector received as input
/// @return short8  vector
#ifdef __cplusplus
extern "C"
#endif
short8 swcSIMDMin8I16(short8 in_vec1, short8 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - short4 vector received as input
/// @param[in] in_vec2 - short4 vector received as input
/// @return short4  vector
#ifdef __cplusplus
extern "C"
#endif
short4 swcSIMDMin4I16(short4 in_vec1, short4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - short2 vector received as input
/// @param[in] in_vec2 - short2 vector received as input
/// @return short2  vector
#ifdef __cplusplus
extern "C"
#endif
short2 swcSIMDMin2I16(short2 in_vec1, short2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - ushort8 vector received as input
/// @param[in] in_vec2 - ushort8 vector received as input
/// @return ushort8  vector
#ifdef __cplusplus
extern "C"
#endif
ushort8 swcSIMDMin8U16(ushort8 in_vec1, ushort8 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - ushort4 vector received as input
/// @param[in] in_vec2 - ushort4 vector received as input
/// @return ushort4  vector
#ifdef __cplusplus
extern "C"
#endif
ushort4 swcSIMDMin4U16(ushort4 in_vec1, ushort4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - ushort2 vector received as input
/// @param[in] in_vec2 - ushort2 vector received as input
/// @return ushort2  vector
#ifdef __cplusplus
extern "C"
#endif
ushort2 swcSIMDMin2U16(ushort2 in_vec1, ushort2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - char16 vector received as input
/// @param[in] in_vec2 - char16 vector received as input
/// @return char16  vector
#ifdef __cplusplus
extern "C"
#endif
char16 swcSIMDMin16I8(char16 in_vec1, char16 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - char8 vector received as input
/// @param[in] in_vec2 - char8 vector received as input
/// @return char8  vector
#ifdef __cplusplus
extern "C"
#endif
char8 swcSIMDMin8I8(char8 in_vec1, char8 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - char4 vector received as input
/// @param[in] in_vec2 - char4 vector received as input
/// @return char4  vector
#ifdef __cplusplus
extern "C"
#endif
char4 swcSIMDMin4I8(char4 in_vec1, char4 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - char2 vector received as input
/// @param[in] in_vec2 - char2 vector received as input
/// @return char2  vector
#ifdef __cplusplus
extern "C"
#endif
char2 swcSIMDMin2I8(char2 in_vec1, char2 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uchar16 vector received as input
/// @param[in] in_vec2 - uchar16 vector received as input
/// @return uchar16  vector
#ifdef __cplusplus
extern "C"
#endif
uchar16 swcSIMDMin16U8(uchar16 in_vec1, uchar16 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uchar8 vector received as input
/// @param[in] in_vec2 - uchar8 vector received as input
/// @return uchar8  vector
#ifdef __cplusplus
extern "C"
#endif
uchar8 swcSIMDMin8U8(uchar8 in_vec1, uchar8 in_vec2);

/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uchar4 vector received as input
/// @param[in] in_vec2- uchar4 vector received as input
/// @return uchar4  vector
#ifdef __cplusplus
extern "C"
#endif
uchar4 swcSIMDMin4U8(uchar4 in_vec1, uchar4 in_vec2);


/// This will compute the minumum value of vector
/// @param[in] in_vec1 - uchar2 vector received as input
/// @param[in] in_vec2 - uchar2 vector received as input
/// @return uchar2  vector
#ifdef __cplusplus
extern "C"
#endif
uchar2 swcSIMDMin2U8(uchar2 in_vec1, uchar2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - float4 vector received as input
/// @param[in] in_vec2 - float4 vector received as input
/// @return float4  vector
#ifdef __cplusplus
extern "C"
#endif
float4 swcSIMDMax4F32(float4 in_vec1, float4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - float2 vector received as input
/// @param[in] in_vec2 - float2 vector received as input
/// @return float2  vector
#ifdef __cplusplus
extern "C"
#endif
float2 swcSIMDMax2F32(float2 in_vec1, float2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - int4 vector received as input
/// @param[in] in_vec2 - int4 vector received as input
/// @return int4  vector
#ifdef __cplusplus
extern "C"
#endif
int4 swcSIMDMax4I32(int4 in_vec1, int4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - int3 vector received as input
/// @param[in] in_vec2 - int3 vector received as input
/// @return int3  vector
#ifdef __cplusplus
extern "C"
#endif
int3 swcSIMDMax3I32(int3 in_vec1, int3 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - int2 vector received as input
/// @param[in] in_vec2 - int2 vector received as input
/// @return int2  vector
#ifdef __cplusplus
extern "C"
#endif
int2 swcSIMDMax2I32(int2 in_vec1, int2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uint4 vector received as input
/// @param[in] in_vec2 - uint4 vector received as input
/// @return uint4  vector
#ifdef __cplusplus
extern "C"
#endif
uint4 swcSIMDMax4U32(uint4 in_vec1, uint4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uint3 vector received as input
/// @param[in] in_vec2 - uint3 vector received as input
/// @return uint3  vector
#ifdef __cplusplus
extern "C"
#endif
uint3 swcSIMDMax3U32(uint3 in_vec1, uint3 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uint2 vector received as input
/// @param[in] in_vec2 - uint2 vector received as input
/// @return uint2  vector
#ifdef __cplusplus
extern "C"
#endif
uint2 swcSIMDMax2U32(uint2 in_vec1, uint2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - half8 vector received as input
/// @param[in] in_vec2 - half8 vector received as input
/// @return half8  vector
#ifdef __cplusplus
extern "C"
#endif
half8 swcSIMDMax8F16(half8 in_vec1, half8 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - half4 vector received as input
/// @param[in] in_vec2 - half4 vector received as input
/// @return half4  vector
#ifdef __cplusplus
extern "C"
#endif
half4 swcSIMDMax4F16(half4 in_vec1, half4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - half2 vector received as input
/// @param[in] in_vec2 - half2 vector received as input
/// @return half2  vector
#ifdef __cplusplus
extern "C"
#endif
half2 swcSIMDMax2F16(half2 in_vec1, half2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - short8 vector received as input
/// @param[in] in_vec2 - short8 vector received as input
/// @return short8  vector
#ifdef __cplusplus
extern "C"
#endif
short8 swcSIMDMax8I16(short8 in_vec1, short8 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - short4 vector received as input
/// @param[in] in_vec2 - short4 vector received as input
/// @return short4  vector
#ifdef __cplusplus
extern "C"
#endif
short4 swcSIMDMax4I16(short4 in_vec1, short4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - short2 vector received as input
/// @param[in] in_vec2 - short2 vector received as input
/// @return short2  vector
#ifdef __cplusplus
extern "C"
#endif
short2 swcSIMDMax2I16(short2 in_vec1, short2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - ushort8 vector received as input
/// @param[in] in_vec2 - ushort8 vector received as input
/// @return ushort8  vector
#ifdef __cplusplus
extern "C"
#endif
ushort8 swcSIMDMax8U16(ushort8 in_vec1, ushort8 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - ushort4 vector received as input
/// @param[in] in_vec2 - ushort4 vector received as input
/// @return ushort8  vector
#ifdef __cplusplus
extern "C"
#endif
ushort4 swcSIMDMax4U16(ushort4 in_vec1, ushort4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - ushort2 vector received as input
/// @param[in] in_vec2 - ushort2 vector received as input
/// @return ushort2  vector
#ifdef __cplusplus
extern "C"
#endif
ushort2 swcSIMDMax2U16(ushort2 in_vec1, ushort2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - char16 vector received as input
/// @param[in] in_vec2 - char16 vector received as input
/// @return char16  vector
#ifdef __cplusplus
extern "C"
#endif
char16 swcSIMDMax16I8(char16 in_vec1, char16 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - char8 vector received as input
/// @param[in] in_vec2 - char8 vector received as input
/// @return char8  vector
#ifdef __cplusplus
extern "C"
#endif
char8 swcSIMDMax8I8(char8 in_vec1, char8 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - char4 vector received as input
/// @param[in] in_vec2 - char4 vector received as input
/// @return char4  vector
#ifdef __cplusplus
extern "C"
#endif
char4 swcSIMDMax4I8(char4 in_vec1, char4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - char2 vector received as input
/// @param[in] in_vec2 - char2 vector received as input
/// @return char2  vector
#ifdef __cplusplus
extern "C"
#endif
char2 swcSIMDMax2I8(char2 in_vec1, char2 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uchar16 vector received as input
/// @param[in] in_vec2 - uchar16 vector received as input
/// @return uchar16  vector
#ifdef __cplusplus
extern "C"
#endif
uchar16 swcSIMDMax16U8(uchar16 in_vec1, uchar16 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uchar8 vector received as input
/// @param[in] in_vec2 - uchar8 vector received as input
/// @return uchar8  vector
#ifdef __cplusplus
extern "C"
#endif
uchar8 swcSIMDMax8U8(uchar8 in_vec1, uchar8 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uchar4 vector received as input
/// @param[in] in_vec2 - uchar4 vector received as input
/// @return uchar4  vector
#ifdef __cplusplus
extern "C"
#endif
uchar4 swcSIMDMax4U8(uchar4 in_vec1, uchar4 in_vec2);

/// This will compute the maximum value of vector
/// @param[in] in_vec1 - uchar2 vector received as input
/// @param[in] in_vec2 - uchar2 vector received as input
/// @return uchar2  vector
#ifdef __cplusplus
extern "C"
#endif
uchar2 swcSIMDMax2U8(uchar2 in_vec1, uchar2 in_vec2);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return float vector  as output

#ifdef __cplusplus
extern "C"
#endif
float swcSIMDSum4F32(float4 in_vec);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return float vector  as output

#ifdef __cplusplus
extern "C"
#endif
float swcSIMDSumAbs4F32(float4 in_vec);

///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return half vector  as output

#ifdef __cplusplus
extern "C"
#endif
half swcSIMDSum8F16(half8 in_vec);

///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return half vector  as output

#ifdef __cplusplus
extern "C"
#endif
half swcSIMDSumAbs8F16(half8 in_vec);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return u32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
u32 swcSIMDSum4U32(uint4 in_vec);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return s32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
s32 swcSIMDSum4I32(int4 in_vec);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return u32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
u32 swcSIMDSum8U16(ushort8 in_vec);

///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return s32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
s32 swcSIMDSum8I16(short8 in_vec);

///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return u32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
u32 swcSIMDSum16U8(uchar16 in_vec);


///Computes the Horizontal vector sum to scalar.
///@param[in] in_vec vector received as input
///@return s32 vector  as output

#ifdef __cplusplus
extern "C"
#endif
s32 swcSIMDSum16I8(char16 in_vec);

#endif //__SWC_SIMD_UTILS__H_
