#ifndef _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_

#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    /*! Compute the homography such that "transformedPoints = homography * originalPoints" */
    Result EstimateHomography(
      const FixedLengthList<Point<f64> > &originalPoints,    //!<
      const FixedLengthList<Point<f64> > &transformedPoints, //!<
      Array<f64> &homography, //!<
      MemoryStack &scratch //!<
      );

    /*! Performs an Singular Value Decomposition on the mXn, float32 input array. [u^t,w,v^t] = SVD(a); */
    Result svd_f32(
      Array<f32> &a,  //!< Input array mXn
      Array<f32> &w,  //!< W array 1Xm
      Array<f32> &uT, //!< U-transpose array mXm
      Array<f32> &vT, //!< V-transpose array nXn
      void * scratch //!< A scratch buffer, with at least "sizeof(float)*(n*2 + m)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, float64 input array. [u^t,w,v^t] = SVD(a); */
    Result svd_f64(
      Array<f64> &a,  //!< Input array mXn
      Array<f64> &w,  //!< W array 1xm
      Array<f64> &uT, //!< U-transpose array mXm
      Array<f64> &vT, //!< V-transpose array nXn
      void * scratch //!< A scratch buffer, with at least "sizeof(f64)*(n*2 + m)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, float32 input array. [w,u^t,v^t] = SVD(a); */
    void icvLightSVD_32f(
      f32* a,   //!< Pointer to the upper-left of the input array. Warning: this array will be modified.
      s32 lda,  //!< A_stride_in_bytes / sizeof(float)
      s32 m,    //!< Number of rows of A
      s32 n,    //!< Number of columns of A
      f32* w,   //!< Pointer to the upper-left of the W array
      f32* uT,  //!< Pointer to the upper-left of the U-transpose array
      s32 lduT, //!< U_stride_in_bytes / sizeof(float)
      s32 nu,   //!< Number of columns of U
      f32* vT,  //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT, //!< V_stride_in_bytes / sizeof(float)
      f32* buffer //!< A scratch buffer, with at least "sizeof(f32)*(n*2 + m)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, float64 input array. [w,u^t,v^t] = SVD(a); */
    void icvLightSVD_64f(
      f64* a,   //!< Pointer to the upper-left of the input array. Warning: this array will be modified.
      s32 lda,  //!< A_stride_in_bytes / sizeof(float)
      s32 m,    //!< Number of rows of A
      s32 n,    //!< Number of columns of A
      f64* w,   //!< Pointer to the upper-left of the W array
      f64* uT,  //!< Pointer to the upper-left of the U-transpose array
      s32 lduT, //!< U_stride_in_bytes / sizeof(float)
      s32 nu,   //!< Number of columns of U
      f64* vT,  //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT, //!< V_stride_in_bytes / sizeof(float)
      f64* buffer //!< A scratch buffer, with at least "sizeof(float)*(n*2 + m)" bytes
      );
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
