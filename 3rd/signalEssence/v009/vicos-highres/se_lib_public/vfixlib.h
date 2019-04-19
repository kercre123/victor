#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2009 Signal Essence; All Rights Reserved

  Module Name  - vfixlib.c

  Author: Hugh McLaughlin

  Description:

      Signal Essence vector fixed point library in C definitions.

      Naming conventions:

      V -- has one or more vector inputs
      S -- has a scalar input

      i16 -- shorthand data type for int16   (signed short)
      q15 -- shorthand data type for int16 fractional Q15
      q14 -- Q14, etc.
      u16 -- shorthand data type for uint16  (unsigned short)
      i32 -- shorthand data type for int32   (long)

      if output is not obvious the the data type is denoted by adding a suffix with _

      Nameing:
      <V><S><operation><operand_<data type operand>_<data type operand>_<data type result>

      V -- if vector operation
      S -- if 2nd operand is a scalar, otherwise it is implied the 2nd operand is V

      If all the operands are the same type then only one data type need be listed.
      If two operands are the same and the result is different, then list the operand type
      and then the result type.
      If the operands are different types, then list all types in order of usage.

   History:    hjm - Hugh McLaughlin

      Mar11,09    hjm  created

  Machine/Compiler: ANSI C
==========================================================================*/

#ifndef ___sevfixlib_h
#define ___sevfixlib_h

#include "se_types.h"
#include "se_platform.h"
#include "frmath.h"
#include "optimized_routines.h"
#include "ext_interface.h"


/*-----------------------------------------------------------------------
  Function Name:   VAdd_i16

  Description:  Vector Add to a Vector.  Computes the element by element
                sum of two vectors.
-------------------------------------------------------------------------*/
typedef void (*VAdd_i16_t)( const int16 *src1, const int16 *src2, int16 *out, uint16 N );
extern VAdd_i16_t VAdd_i16;

/*----------------------------------------------------------------------=
  Function Name:   VAddVS32

  Description:  Vector Add.  Adds two longword vectors together, element by element.

  Notes: Can write over a source vector?
-----------------------------------------------------------------------*/
void VAdd_i32( const int32 *src1, const int32 *src2, int32 *dest, uint16 N );

void VAddScalar_i32(const int32 *px, int32 k, int32 *py, uint16 N);

/*-----------------------------------------------------------------------
  Function Name:   VAvePowerRMS_i16

  Description:  Vector compute the average power and take the RMS value of
  the power and return the result in a uint16.
-------------------------------------------------------------------------*/
uint16 VAvePowerRMS_i16( const int16 *yptr, uint16 N );

/*=======================================================================
  Function Name:   VComplexPoweri16_i32()

  Description:
      For a complex vector where each element is x+iy
      Computes
         power[i] = x[i]*x[i] + y[i]*y[i]

  Returns: void
=======================================================================*/
void VComplexPoweri16_i32(
        const int16  *inptr,          // complex vector
        int32  *outptr,         // long word output vector
        uint16 numOut );        // same as number of complex elements


/* -----------------------------------------------------------------------
  Function Name:   VCMul_16()

  Description:
      Multiplies two complex vector where each element is x+iy
      Computes
         x[i] = x0[i]*x1[i] - y0[i]*y1[i]
         y[i] = x0[i]*y1[i] + x1[i]*y0[i]

	 data is stored real, imag, real, imag.....

  Returns: void
---------------------------------------------------------------------------*/

void VCMul_i16_Win32(
        const int16  *xptr,           // complex vector
        const int16  *yptr,           // complex vector
        int16  *outptr,         // complex output vector
        uint16 numOut );        // same as number of complex elements

/*----------------------------------------------------------------------
    Function Name:   VCRMulAdd_i16_i32

    Description:

      Multiplies each 16-bit integer element of a complex vector by a Q15 vector,
      complex element by real element to produce a 32-bit result and add that
      to an existing 32-bit result.

    Returns: void
-------------------------------------------------------------------------*/
typedef void (*VCRMulAdd_i16_i32_t)( const int16 *cPtr,          // complex input
                              const int16 *rPtr,          // real coefficient vector
                              const int32 *int32src,      // add-to input
                              int32 *destPtr,       // result output, allowed to be in-place
                              uint16 N );           // number of complex outputs
extern VCRMulAdd_i16_i32_t VCRMulAdd_i16_i32;

// for testing
void VCRMulAdd_i16_i32_ansi( const int16 *cPtr,          // complex input
                             const int16 *rPtr,          // real coefficient vector
                             const int32 *int32src,      // add-to input
                             int32 *destPtr,       // result output, allowed to be in-place
                             uint16 N );            // number of complex outputs

/*=======================================================================
  Function Name:   VGenComplexTone_i16()

  Description:
      Geneates a complex tone from a phasor and initial state.
	  This is useful for creating linear delay in the frequency domain, or for
	  creating a complex tone in the time domain.
	  
  	 data is stored real, imag, real, imag.....

  Returns: void
=======================================================================*/

typedef void (*VGenComplexTone_i16_t)(
    int16        statePhasor[2],    // real and imaginary
    const int16  deltaPhasor[2],    // real and imaginary
    int16       *destPtr,
    uint16       numComplexOut );
extern VGenComplexTone_i16_t VGenComplexTone_i16;

void VGenComplexTone_i16_float_ansi(
    int16        statePhasor[2],    // real and imaginary
    const int16  deltaPhasor[2],    // real and imaginary
    int16       *destPtr,
    uint16       numComplexOut );

/*----------------------------------------------------------------------
    Function Name:   VGen_Exp_Ramp_i16

    Description:

      Generates an exponential ramp of values by doing a single pole
      filter with a target and current state.

    Returns: int16
-------------------------------------------------------------------------*/
void VGen_Exp_Ramp_i16( int16 *gainStatePtr,     // gain state -- scalar value
                        int16 alpha,             // averaging coefficient
                        int16 target,            // target
                        int16 *gptr,             // output vector
                        int N );               // num in block

void VGenRamp_i16( int16 *px, int len, int16 initVal, int16 incr);

/*----------------------------------------------------------------------
    Function Name:   VGetIndex*

    Get index of max/min value in vector

    See vgetindex.c
-------------------------------------------------------------------------*/

uint16 VGetIndexMax_i16( const int16 *inPtr, uint16 N );
uint16 VGetIndexMaxAbs_i16( const int16 *inPtr, uint16 N );
uint16 VGetIndexMin_i16( const int16 *inPtr, uint16 N );
uint16 VGetIndexMax_i32( const int32 *pIn, uint16 N );
uint16 VGetIndexMaxAbs_i32( const int32 *inPtr, uint16 N );


//
// get max/min value in vector
int16 VGetValueMin_i16( const int16 *inPtr, uint16 N );
int16 VGetValueMax_i16( const int16 *inPtr, uint16 N );
uint16 VGetValueMaxAbs_i16( const int16 *inPtr, int32 N );
int32 VGetValueMax_i32( const int32 *pIn, uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VLimitMin_i32 and VLimitMin_i16

    Description:
       If an element is below the minLimitValue then it is set to the minLimitValue

    Returns: void
-------------------------------------------------------------------------*/

void VLimitMin_i32( const int32 *src,
                    int32 minLimitValue,
                    int32 *dest,
                    uint16 N );

void VLimitMin_i16( const int16 *src,
                    int16 minLimitValue,
                    int16 *dest,
                    uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VMax_i16

    Description:

      Takes the maximum value between each element in two vectors of 16 bit words.
      output = max( src1, srcr2 );

    Returns: void
-------------------------------------------------------------------------*/
void VMax_i16( const int16 *src1Ptr,
               const int16 *src2Ptr,
               int16 *outPtr,
               uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VMax_i32

    Description:

      Takes the maximum value between each element in two vectors.
      output = max( src1, srcr2 );

    Returns: void
-------------------------------------------------------------------------*/

void VMax_i32( const int32 *src1Ptr,
               const int32 *src2Ptr,
               int32 *outPtr,
               uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VMin_i16

    Description:

      Takes the minimum value between each element in two vectors.
      output = max( src1, srcr2 );

    Returns: void
-------------------------------------------------------------------------*/

void VMin_i16( const int16 *src1Ptr,
               const int16 *src2Ptr,
               int16 *outPtr,
               uint16 N );

void VMinWithIndex_i16(const int16* pV0,
                       const int16* pV1,
                       int16* pMinIndex,
                       int16* pMinVal,
                       uint16 N);

void VMin3WayWithIndex_i16(const int16* pV0,
                       const int16* pV1,
                       const int16* pV2,
                       int16* pMinIndex,
                       int16* pMinVal,
                       uint16 N);

/*----------------------------------------------------------------------
    Function Name:   VMin_i32

    Description:

      Takes the minimum value between each element in two vectors.
      output = max( src1, srcr2 );

    Returns: void
-------------------------------------------------------------------------*/

void VMin_i32( const int32 *src1Ptr,
               const int32 *src2Ptr,
               int32 *outPtr,
               uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VMMQ15_Add_i32

    Description:
        Multiples a vector by a scaler and multiplies another vector by a scalar and
        adds them to get a 32 bit result.
        C = A*a + B*b
        The scalars are Q15 values.

    Returns: void
-------------------------------------------------------------------------*/
void VMMQ15_Add_i32( const int32 *xptr,
                     int16 a,
                     const int32 *yptr,
                     int16 b,
                     int32 *outPtr,
                     uint16 N );

/*----------------------------------------------------------------------
    Function Name:   VMul_i32_q15_i32, VMul_i32_q12_i32, VMul_i32_q10_i32

    Description:

      Multiplies each 32-bit integer by a Q15 vector, element by element to
      produce a 32-bit result.

    Returns: void
-------------------------------------------------------------------------*/
void VMul_i32_q15_i32( const int32 *src, const int16 *q15Ptr, int32 *dest, int32 N );
void VMul_i32_q12_i32( const int32 *src, const int16 *q12Ptr, int32 *dest, uint16 N );
void VMul_i32_q10_i32( const int32 *src, const int16 *q12Ptr, int32 *dest, uint16 N );


/*---------------------------------------------------------------------
    Description:

      Sums all the elements of the input vector containing shorts.
      Shift result, if desired, so that it fits in a short.

        The right shift operator (>>) is arithmetic, i.e., negative
        numbers stay that way.

    Returns: short
-------------------------------------------------------------------------*/

int16 VSumRtShift_i16( const int16 *src, uint16 N, uint16 rt_shift );

/*----------------------------------------------------------------------
    Function Name:   VSum_i32

    Description:

      Vector Sum signed longwords.  Sums the elements in a longword
      (32 bit signed) vector.

    Returns: long

    Notes:
------------------------------------------------------------------------*/

int32 VSum_i32( const int32 *src1, int32 N );


/*----------------------------------------------------------------------
    Function Name:   VSumRtShifts_i32

    Description:

      Vector Sum signed longwords.  Sums the elements in a longword
      (32 bit signed) vector.
      Shift each element first to avoid saturation issues. ASM code
      assumes there are guard bits and the vector is less than 128
      elements.

    Returns: long

    Notes:
------------------------------------------------------------------------*/

int32 VSumRtShifts_i32( const int32 *src1, uint16 N, uint16 rt_shifts );

/*----------------------------------------------------------------------
    Function Name:   VSubi16

    Description:

      Vector subtract with saturation.  Computes the element by element difference of
      of a two vectors.

    Returns: void

    Notes:
-----------------------------------------------------------------------*/

typedef void (*VSub_i16_t)( const int16 *aptr, const int16 *bptr, int16 *dest, uint16 N );
extern VSub_i16_t VSub_i16;


/*-----------------------------------------------------------------------

    Function Name:   VSub_i32

    Description:

      Vector Sub.  Subtracts one longword vector from another longword
                   vector, element by element.

    Returns: void
-------------------------------------------------------------------------*/
void VSub_i32( const int32 *src1, const int32 *src2, int32 *dest, uint16 N );

/*-------------------------------------------------------------------------
    Function Name:   VTrackUpAveDown

    Description:

      Tracks the input in the upward direction and does a leaky average
      in the down direction.

    Returns: void
-------------------------------------------------------------------------*/
void VTrackUpAveDown_i32(int32  *statePtr,
                         const int32  *inPtr,
                         int16  alpha,
                         int N );

/*-------------------------------------------------------------------------
    Function Name:   VFlip_i16

    Description:

      Vector invert the order of a vector of int16 words.  Moves the elements
      in a vector so that the first is last, second is second to last, ...
      last is first.

    Returns: void

    Notes:
-------------------------------------------------------------------------*/
void VFlip_i16( const int16 *src, int16 *out, uint16 N );

/*-------------------------------------------------------------------------
    Function Name:   VAvePower_i16_i32

    Description:

        Computes the average power of a fixed point (integer)
        vector.   S32 = SUM( S[0]..S[N-1] ) / N

    Returns: long

-------------------------------------------------------------------------*/
int32 VAvePower_i16_i32( const int16 *xptr, uint16 N );

/*-------------------------------------------------------------------------
    Function Name:   VDelaySave_i16

    Description:

        Moves a vector to the right by moving element by element starting with the last
        element.  Then retrieves data from a "previous" buffer to fill the area in front.
        Finally, it saves

    Returns: void
-------------------------------------------------------------------------*/

void VDelaySave_i16( const int16 *src, int16 *dest, int16 *delayBufPtr, int16 blockSize, int16 delaySize );


/*-------------------------------------------------------------------------

    Function Name:   VPower_i16_i32

    Description:

        Computes the  power of a fixed point (integer)
        vector.   S32 = SUM( S[0]..S[N-1] )

    Returns: long

    Notes:

-------------------------------------------------------------------------*/
typedef uint32 (*VPower_i16_i32_t)( const int16 *inptr, uint16 rshifts, uint16 N );
extern VPower_i16_i32_t VPower_i16_i32;

/*-------------------------------------------------------------------------

    Function Name:   VPowerWithExponent_i16_i32

    Description:

        Computes the  power of a fixed point (integer)
        vector.   S32 = SUM( S[0]..S[N-1] )
        Tries to maximize the precision and determines the exponent, i.e.
        the number of right shifts taken.

        The full result would be:
             retValue*(2^rightShiftsTaken)

    Returns: long

    Notes:

-------------------------------------------------------------------------*/

typedef uint32 (*VPowerWithExponent_i16_i32_t)( const int16 *inptr, uint16 *expPtr, uint16 N );
extern VPowerWithExponent_i16_i32_t VPowerWithExponent_i16_i32;

/*-------------------------------------------------------------------------

    Function Name:   VFill_i32

    Description:

      Vector Fill Long Words.

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
void VFill_i32(int32* dest, int32 fillWord, int32 N );

/*-------------------------------------------------------------------------

    Function Name:   VFill_i16

    Description:

      Vector Fill int16 Words.

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
typedef void (*VFill_i16_t)( int16 *dest, int16 fillWord, uint32 N );
extern VFill_i16_t VFill_i16;

void VFill_ui16(uint16 *pDest, uint16 val, int N);

/*-------------------------------------------------------------------------

    Function Name:   VLeftShifti16

    Description:

      Vector Left Shifts.  Shifts each element of a vector left
      by the numLefts argument.
      Works for right shifts too (specify negative leftshift)

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
typedef void (*VLeftShifts_i16_t)( const int16 *src, int16 numLefts, int16 *dest, uint16 N );
extern VLeftShifts_i16_t VLeftShifts_i16;

/*-------------------------------------------------------------------------

    Function Name:   VMoveS32

    Description:

      Vector Move.  Moves a vector of S32 words from one location to another.

    Returns: void

    Notes:

-------------------------------------------------------------------------*/

typedef void (*VMove_i32_t)(const int32 *src, int32 *dest, uint32 N );
extern VMove_i32_t VMove_i32;

/*-------------------------------------------------------------------------

    Function Name:   VMovei16

    Description:

      Vector Move.  Moves a vector from one location to another.

    Returns: void

    Notes:
    VMove_i16_C55 is optimized for C55 platform.  See function comments
    for constraints on usage.

    The Win32 version contains diagnostic code to identify when
    non-optimal transfers occur.
    To identify non-optimal VMove operations on the C55,
    re-define VMove_i16 to the Win32 version and place breakpoints
    in the diagnostic code.

-------------------------------------------------------------------------*/
typedef void (*VMove_i16_t)( const int16 *src, int16 *dest, uint32 N );
extern VMove_i16_t VMove_i16;

/*-------------------------------------------------------------------------

    Function Name:   VMovei16SourceStride

    Description:

      Vector Move.  Moves a vector of i16 words with a stride between
      each source word.  The destination words are stored in order.

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
void VMoveSourceStride_i16( const int16 *src, uint16 stride, int16 *dest, int N );


/*-------------------------------------------------------------------------

    Function Name:   VMoveDestStride_i16

    Description:

      Vector Move.  Moves a vector of i16 words with a stride between
      each destination word.

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
void VMoveDestStride_i16( const int16 *src, int stride, int16 *dest, int N );

/*-------------------------------------------------------------------------
    Function Name:   VShiftRtRnd_i32_i16

    Description:

      Vector Shift Long Word, Round 32 bit word and save 16-bit bits.

    Returns: void

    Notes:
-------------------------------------------------------------------------*/
typedef void (*VShiftRtRnd_i32_i16_t)( const int32 *lsrc, int16 *sdest, uint16 shifts, uint16 N );
extern VShiftRtRnd_i32_i16_t VShiftRtRnd_i32_i16;

/*-------------------------------------------------------------------------
    Function Name:   VShiftLtRnd_i16_i32

    Description:

      Vector Shift short Word left, and save 32-bit bits.

    Returns: void

    Notes:
-------------------------------------------------------------------------*/
typedef void (*VShiftLtRnd_i16_i32_t)( const int16 *ssrc, int32 *ldest, uint16 shifts, uint16 N );
extern VShiftLtRnd_i16_i32_t VShiftLtRnd_i16_i32;

/*-------------------------------------------------------------------------
    Function Name:   VMulQ15Ramp_i16

    Description:

      Vector Multiply by a ramping scalar.  Multiply all the elements of a vector
      by a single scalar that is incremented every sample

    Returns: the last value of the scalar

    Notes:
-------------------------------------------------------------------------*/
int16 VMulQ15Ramp_i16( const int16 *src, int16 q15val, int16 delta, int16 *dest, uint16 N );

/*-------------------------------------------------------------------------
    Function Name:   VMulSmoothedi16

    Description:

     Vector Multiply by Smoothed Scalar.  Multiply all the elements of a vector
     by a scalar that is ramping like an exponential function from the initial
     state converging to the target state.

    Returns: void

    Notes:
-------------------------------------------------------------------------*/
void VMulSmoothed_i16( const int16 *src,
                       int16 *statePtr,
                       const int16 *alpha,
                       int16 target,
                       int16 *dest,
                       uint16 N );

/*-------------------------------------------------------------------------

    Function Name:   VMul_i16_q15_i16, VMul_i16_q12_i16

    Description:

      Element-by-element vector multiply.  Multiply each of the elements of a vector
      by another vector value in Q15 or Q12 format

    Returns: void

    Notes:

-------------------------------------------------------------------------*/
typedef void (*VMul_i16_q15_i16_t)( const int16 *aptr, const int16 *bptr, int16 *dest, uint16 N );
extern VMul_i16_q15_i16_t VMul_i16_q15_i16;

// for testing
void VMul_i16_q15_i16_ansi( const int16 *src, const int16 *q15, int16 *dest, uint16 N );
void VMul_i16_q12_i16( const int16 *aptr, const int16 *bptr, int16 *dest, int32 N );

/*----------------------------------------------------------------------=

  Function Name:   VDotProducti16ReturnS32

  Description:

      Computes the dot product (e.g., correlation) of a fixed point
      (integer) vector.  Intended to operate on vectors which are
      lengths that are a power of 2.  The number of right shifts
      parameter specifies the divisor to compute the average.  The
      number of right shifts must be enough to avoid an overflow.

  Returns: long

  Notes:

-----------------------------------------------------------------------*/
typedef int32 (*VDotProductRtShifts_i16_i32_t)( const int16 *xptr,
						const int16 *hptr, uint16 N,
						uint16 rshifts );
extern VDotProductRtShifts_i16_i32_t VDotProductRtShifts_i16_i32;


/*----------------------------------------------------------------------=

  Function Name:   VDotProductQ15_i16_i16

  Description:

      Computes the dot product (e.g., correlation) of a fixed point
      (integer) vector.

  Returns: long

  Notes:

-----------------------------------------------------------------------*/
int16   VDotProductQ15_i16_i16_ansi( const int16 *xptr, const int16 *hptr, uint16 N );

typedef void (*VDotProductsLeftShift_q15_i16_t)( const int16 *inPtr,                       // input
                                                 const int16 *coefPtr,                     // other input
                                                 uint16 leftShifts,                  // allow upscaling
                                                 int16 *outPtr,                      // output vector
                                                 uint16 numPerDotProd,               // number of multiplies per dot product
                                                 uint16 numOut,                      // number of output elements
                                                 uint16 decimationFactor );           // number of input points to skip
extern VDotProductsLeftShift_q15_i16_t VDotProductsLeftShift_q15_i16;

//
// for testing
void VDotProductsLeftShift_q15_i16_ansi(  const int16 *inPtr,                       // input
                                          const int16 *coefPtr,                     // other input
                                          uint16 leftShifts,                  // allow upscaling
                                          int16 *outPtr,                      // output vector
                                          uint16 numPerDotProd,               // number of multiplies per dot product
                                          uint16 numOut,                      // number of output elements
                                          uint16 decimationFactor );           // number of input points to skip

void VWindowWithPreviousBlock_i16( const int16 *wfPtr,        // windowing function, size of window
                                   const int16 *newPtr,       // new input block
                                   int16 *prevPtr,      // previous block
                                   int16 *outPtr,       // windowed block, output is blocksize+overlap
                                   int16 overlap,
                                   int16 blockSize );    // input blocksize
/* ======================================================
   Scale a vector by a scalar

 =========================================================*/

typedef void (*VScale_i32_q15_i32_t)( const int32 *inPtr,
                               int16 q15Scalar,
                               int32 *outPtr,     // can be inplace
                               int N );
extern VScale_i32_q15_i32_t VScale_i32_q15_i32;

void VScale_i16_q15_i16( const int16 *src, int16 q15val, int16 *dest, int N );
void VScale_i16_q10_i16( const int16 *src, int16 q10val, int16 *dest, int N );
typedef void (*VScale_i16_q12_i16_t)( const int16 *src, int16 q12val, int16 *dest, int N );
extern VScale_i16_q12_i16_t VScale_i16_q12_i16;

typedef void (*VScale_i16_q15_i32_t)( const int16 *src, int16 q15val, int32 *dest, int N );
extern VScale_i16_q15_i32_t VScale_i16_q15_i32;

//void VMulScalar_i16_i32( int16 *src, int16 i16val, int32 *dest, uint16 N );
void VScale_i16_i16_i32( const int16 *src, int16 i16val, int32 *dest, int N );

typedef void (*VScale_q31_q15_q15_t)(const int32* px, int16 k, int16* py, int N);
extern VScale_q31_q15_q15_t VScale_q31_q15_q15;

typedef void (*VScale_q31_q14_q15_t)(const int32* px, int16 k_q14, int16* py, int N);
extern VScale_q31_q14_q15_t VScale_q31_q14_q15;

void VScale_q31_q12_q15(const int32* px, int16 k_q12, int16* py, int N);

void VScale_i32_q10_i32(const int32 *src, int16 i16val, int32 *dest, int N );

/* ======================================================

   Scale a vector by a scalar, then add another vector!

 =========================================================*/
void VScaleAdd_i16_q15_i32_i32( const int16 *src,
                               int16 q15val,
                               const int32 *i32src,
                               int32 *dest,
                               uint16 N );

void VScaleAdd_i16_q12_i32_i32( const int16 *src,
                               int16 q12val,
                               const int32 *i32src,
                               int32 *dest,
                               uint16 N );

void VScaleAdd_i16_i16_i32_i32( const int16 *src,
                                int16 i16val,
                                const int32 *i32src,
                                int32 *dest,
                                uint16 N );
void VScaleAdd_i16_q12_i16_i16( const int16 *s1p,
                                int16 i16val,   // Q12
                                const int16 *s2p,
                                int16 *dest,
                                uint16 N );

typedef void (*VScaleAdd_i16_q15_i16_i16_t)( const int16 *s1p,
                                 int16 i16val,   // Q15
                                 const int16 *s2p,
                                 int16 *dest,
                                 uint16 N );
extern VScaleAdd_i16_q15_i16_i16_t VScaleAdd_i16_q15_i16_i16;


void VScaleAdd_i16_q12_i16_i16( const int16 *s1p,
                           int16 q12val,   // Q12
                           const int16 *s2p,
                           int16 *dest,
                           uint16 N );


float32 VSumPower_i16_flt(const int16 *p, int N);

#endif // #ifdef
#ifdef __cplusplus
}
#endif


