#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence - Avistar; All Rights Reserved

Module Name  - vfloatlib.h

Author: Hugh McLaughlin

History

   04-23-09      hjm    created

Description

   Vector floating point library.
   The general rule is that these are primitive.  They require no
   information between calls.  They have no hidden data, no static
   data, no globals.

Notes:

    Naming:

    All these routines start with a V which stands for vector.
    The first letter of each descriptive word is capitalized.
    There are no underscores.
    The first descriptive word is a verb which describes the
    principal action.

    Parameter List Convention:

    In general the first parameter points to the source vector.
    If there are two source vectors then the 2nd source vector is
    listed next.
    Next comes the destination vector.
    Next comes any auxiliary information or pointers.
    Last is the number of things to do, e.g. numElements numQuads etc.



Machine/Compiler: ANSI C

-------------------------------------------------------------*/

#ifndef __vfloatlib_h
#define __vfloatlib_h

#include "se_types.h"
#include "optimized_routines.h"


void  VSMMAdd_flt32( const float32 *Aptr, float32 a,
		     const float32 *Bptr, float32 b,
		     float32 *Cptr, int N );

float32 VDotProduct_flt32( const float32 *xptr, const float32 *yptr, int N );
uint16 VGetIndexMax_flt32( const float32 *src, int N );

void VDivide_flt32( const float32 *pUpstairs, const float32 *pDownstairs, float32 *pDest, int N);
void VFill_flt32( float32 *dest, float32 fval, int N );
void VAdd_i16_flt32_flt32(const int16 *px_i16, const float32 *py_flt32, float *pdest_flt32, int N);
void VMove_flt32(const float32 *psrc, float32 *pdest, int N );
//void  VLimit( float32 *src, float32 *dest, float32 plusLimit, int N );
void VInvert_f32(const float32 *pSrc, float32 *pDest, int N);
void VMul_f32_q10_f32(const float32 *pSrc, int16 *pQ10, float32 *pDest, uint16 N);
void VMul_flt32(const float32 *pa, const float32 *pb, float32 *pdest, int N);
void VConjugateMul_flt32(const float32 *pX, float32 *pYPreConj, float32 *pDest, int numComplexValues);

void VAbs_flt32(const float32 *pSrc, float32 *pDest, int N);
void VSub_flt32(const float32 *psrc1, const float32 *psrc2, float32 *pdest, int N );

float32 VSumPower_flt32(const float32 *pa, int N);
float32 VSumPower_i16_f32(const int16 *pa, int N);
float32 VSumPowerComplex_f32(const float32 *pa, int numComplex);

void VLimitMin_flt32( const float32 *src,
                      float32 minLimitValue,
                      float32 *dest,
                      int N );

float32 VAvePower_i32_f32(const int32 *xptr, int N);
float32 VAvePower_f32(const float32 *xptr, int N);
float32 VAvePower_i16_f32(const int16 *xptr, int N);

void VScale_i16_f32_f32(const int16 *psrc, float32 scalar, float32 *pdest, int N);

void VMin3WayWithIndex_flt32(const float32* pV0,
                             const float32* pV1,
                             const float32* pV2,
                             int32* pMinIndex,
                             float32* pMinVal,
                             int32 N);

typedef void (*VFlipFloatToQ15_t)(const float32 *psrc, int16 *pdest, int N);
extern VFlipFloatToQ15_t VFlipFloatToQ15;

typedef void (*VComplexMulConjugate_flt32_t)(const float32 *pX, const float32 *pY, float32 *pDest, int numComplexValues);
extern VComplexMulConjugate_flt32_t VComplexMulConjugate_flt32;

typedef void (*VScale_flt32_t)(const float32 *src, float32 scalar, float32 *dest, int N );
extern VScale_flt32_t VScale_flt32;

typedef void (*VComplexRealMul_flt32_t)(const float32 *pCmplx, const float32 *pReal, float32 *pCmplxDest, int numComplexValues);
extern VComplexRealMul_flt32_t VComplexRealMul_flt32;

typedef void   (*VComplexMul_flt32_t) (const float32 *pX, const float32 *pY, float32 *pDest, int numComplexValues);
extern VComplexMul_flt32_t       VComplexMul_flt32;

typedef void  (*VAdd_flt32_t)(const float32 *pX, const float32 *pY, float32 *pDest, int N );
extern VAdd_flt32_t VAdd_flt32;

typedef void   (*VComplexMulAdd_flt32_t) (const float32 *pX, const float32 *pY, float32 *pDest, int numComplexValues);
extern VComplexMulAdd_flt32_t       VComplexMulAdd_flt32;

typedef void   (*VMove_flt32_i16_t)(const float *pSrcFlt, int16 *pDest16, int N);
extern VMove_flt32_i16_t            VMove_flt32_i16;

typedef void (*VComplexPower_flt32_t)( const float32 *src, float32 *dest, int numComplex );
extern VComplexPower_flt32_t VComplexPower_flt32;


/****************************************************************************/

void  VComplexPower(const float32 *src, float32 *dest, int N );
float32 VDotProductFlt32(const float32 *xptr, float32 *yptr, int N );
//int   VFindMax(const float32 *Aptr, int N );     // returns index of max value
float32 VSum_flt32(const float32 *pa, int N);
float32 VGetValueMax_flt32(const float32 *inPtr, int N );
float32 VGetValueMaxAbs_flt32(const float32 *inPtr, int N );
float32 VGetValueMin_flt32(const float32 *inPtr, int N );
int VGetIndexMax_f32(const float32 *inPtr, int N );
float32 VMean_flt32(const float32 *inPtr, int N);
void VVariance_flt32(const float32 *inPtr, int N, float32 *pMean, float32 *pVariance);

void VWindowWithPreviousBlock_flt32( const float32 *wfPtr,        // windowing function, size of window
                                   const float32 *newPtr,       // new input block
                                   float32 *prevPtr,      // previous block
                                   float32 *outPtr,       // windowed block, output is blocksize+overlap
                                   int32 overlap,
                                   int32 blockSize );    // input blocksize
/****************************************************************************/
//
// functions named *_ansi are ansi-c implementations, exported for testing
void VAdd_flt32_ansi(const float32 *pX, const float32 *pY, float32 *pDest, int N );
void VAddScalar_flt32(const float32 *px, float32 k, float32 *py, int N);
void VComplexMul_flt32_ansi(const float32 *pX, const float32 *pYPreConj, float32 *pDest, int numComplexValues);
void VComplexRealMul_flt32_ansi(const float32 *pCmplx, const float32 *pReal, float32 *pCmplxDest, int numComplexValues);
void VFlipFloatToQ15_ansi(const float32 *psrc, int16 *pdest, int N);
void VFlip_f32(const float32 *psrc, float32 *pdest, int N);
void VFlip_i16_f32(const int16 *psrc, float32 *pdest, int N);
void VComplexMulAdd_flt32_ansi(const float32 *pX, const float32 *pY, float32 *pDest, int numCmplx);
void VMove_flt32_i16_ansi(const float *pSrcFlt, int16 *pDest16, int N);
void VComplexMulConjugate_flt32_ansi(const float32 *pX, const float32 *pY, float32 *pDest, int numComplexValues);
void VComplexPower_flt32_ansi( const float32 *src, float32 *dest, int numComplex );

// format conversion
void VConvert_i32_f32(const int32 *pInInt32, float32 *pOutFlt32, int N);
void VConvert_i16_f32(const int16 *pInInt16, float32 *pOutFlt32, int N);
void VConvert_f32_i32(const float32 *psrc, int32 *pdest, int N);
void VConvert_f32_q15(const float32 *psrc, int16 *pdest, int N);
void VConvert_f32_i16(const float32 *psrc, int16 *pdest, int N);
void VConvert_q15_f32(const int16* psrc_q15, float32* pdest_flt32, int N);
void VConvert_i16_f32(const int16 *pSrc16, float32 *pDestFlt, int N);

void VMin_flt32( const float32 *src1Ptr,
                 const float32 *src2Ptr,
                 float32 *outPtr,
                 int N );
void VMax_flt32( const float32 *src1Ptr,
                 const float32 *src2Ptr,
                 float32 *outPtr,
                 int N );


void VTrackUpAveDown_flt32(float32  *statePtr,
                           const float32 *inPtr,   
                           float32 alpha,     
                           int N );

void VAveDown_flt32( float32 *statePtr, 
                     const float32 *pCompareInput, 
                     float32 alphaDown, 
                     float32 *pOutput,   // can be the same as statePtr
                     int   N );

void VAveUpAveDown_flt32( float32 *statePtr,     
                          const float32 *pInput,
                          float32 alphaUp,       
                          float32 alphaDown,
                          float32 *pOutput,        // can be same as statePtr
                          int     N );

void VMM_Add_flt32( const float32 *xptr, 
                    float32 a, 
                    const float32 *yptr,
                    float32 b, 
                    float32 *outPtr,
                    int N );

float32 VSquareSum_f32( const float32 *vector, int len );

void VGen_Exp_Ramp_f32( const float32 *gainStatePtr,     // gain state -- scalar value
                        float32 alpha,             // averaging coefficient
                        float32 target,            // target 
                        float32 *gptr,             // output vector
                        int32 N );               // num in block

void VGenRamp_f32( float32 *px, int len, float32 initVal, float32 incr);

void VCalcAngle_f32(const float32 *pComplex, float32 *pPhi, int len);

//void  VFloat2Short(const float32 *src, int16  *dest, int N );
//void  VMin(const  float32 *Aptr, const float32 *Bptr, float32 *dest, int N );
//void  VMove(const  float32 *src, float32 *dest, int N );
//void  VMoveComplexStride(const  float32 *src, float32 *dest, int N,
//                         int strideSrc, int strideDest );
//void  VMoveStride(const float32 *src, int srcStride, float32 *dest, int destStride, int N );
//void  VMul(const  float32 *src1, const float32 *src2, float32 *dest, int N );
//void  VShort2Float(const int16 *src, float32 *dest, int N );
//    Vector Scalar multiply, multiply and add.
//    Vector Scalar multiply, multiply and add; and add bias.
//    Same as VSMMAdd except the input has a bias value added to it.
//void  VSMMAddB(const float32 *Aptr, float32 a,
//              const float32 *Bptr, float32 b,
//              const float32 *Cptr, float32 bias,
//              int N );
//void  VSMul(const float32 *src, float32 Scalar, float32 *dest, int N );

//void  VSubInPlaceAndStore(const float32 *Aptr, const float32 *Bptr, float32 *Cptr, int N );
//void VSubMin(const float32 *Aptr, const loat32 *Bptr, const float32 *Cptr, const float32 *Dptr, int N );
//void VSubSum(const float32 *src1, const float32 *src2, const float32 *subsum, int N );
//float VSum(const float32 *src1, int N );


#endif
#ifdef __cplusplus
}
#endif

