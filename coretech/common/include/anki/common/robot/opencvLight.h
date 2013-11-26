// Opencv functions and wrappers

#ifndef _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
#define _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"

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

    /*! Performs an Singular Value Decomposition on the mXn, f32 input array. [u^t,w,v^t] = SVD(a); */
    Result svd_f32(
      Array<f32> &a,  //!< Input array mXn
      Array<f32> &w,  //!< w-vector 1Xm
      Array<f32> &uT, //!< U-transpose array mXm
      Array<f32> &vT, //!< V-transpose array nXn
      void * scratch  //!< A scratch buffer, with at least "sizeof(f32)*(n_stride*2 + m_stride)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, f64 input array. [u^t,w,v^t] = SVD(a); */
    Result svd_f64(
      Array<f64> &a,  //!< Input array mXn
      Array<f64> &w,  //!< w-vector 1Xm
      Array<f64> &uT, //!< U-transpose array mXm
      Array<f64> &vT, //!< V-transpose array nXn
      void * scratch  //!< A scratch buffer, with at least "sizeof(f64)*(n_stride*2 + m_stride)" bytes
      );

    /*! Performs Singular Value Back Substitution (solves A*X = B) */
    Result svdBackSubstitute_f32(
      const Array<f32> &w, //!< w-vector 1Xm
      const Array<f32> &Ut,//!< U-array mXm
      const Array<f32> &Vt,//!< V-array nXn
      Array<f32> &b,       //!< b-vector 1Xm
      Array<f32> &x,       //!< x-vector 1Xn
      void * scratch       //!< A scratch buffer, with at least "sizeof(f32) * (MAX(m_stride, n_stride) + o_stride)" bytes
      );

    /*! Performs Singular Value Back Substitution (solves A*X = B) */
    Result svdBackSubstitute_f64(
      const Array<f64> &w, //!< w-vector 1Xm
      const Array<f64> &Ut,//!< U-array mXm
      const Array<f64> &Vt,//!< V-array nXn
      Array<f64> &b,       //!< b-vector 1Xm
      Array<f64> &x,       //!< x-vector 1Xn
      void * scratch       //!< A scratch buffer, with at least "sizeof(f64) * (MAX(m_stride, n_stride) + o_stride)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, f32 input array. [w,u^t,v^t] = SVD(a); */
    void icvLightSVD_32f(
      f32* a,   //!< Pointer to the upper-left of the input array. Warning: this array will be modified.
      s32 lda,  //!< A_stride_in_bytes / sizeof(f32)
      s32 m,    //!< Number of rows of A
      s32 n,    //!< Number of columns of A
      f32* w,   //!< Pointer to start of the W vector
      f32* uT,  //!< Pointer to the upper-left of the U-transpose array
      s32 lduT, //!< U_stride_in_bytes / sizeof(f32)
      s32 nu,   //!< Number of columns of U
      f32* vT,  //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT, //!< V_stride_in_bytes / sizeof(f32)
      f32* buffer //!< A scratch buffer, with at least "sizeof(f32)*(n_stride*2 + m_stride)" bytes
      );

    /*! Performs an Singular Value Decomposition on the mXn, f64 input array. [w,u^t,v^t] = SVD(a); */
    void icvLightSVD_64f(
      f64* a,   //!< Pointer to the upper-left of the input array. Warning: this array will be modified.
      s32 lda,  //!< A_stride_in_bytes / sizeof(f32)
      s32 m,    //!< Number of rows of A
      s32 n,    //!< Number of columns of A
      f64* w,   //!< Pointer to the start of the W vector
      f64* uT,  //!< Pointer to the upper-left of the U-transpose array
      s32 lduT, //!< U_stride_in_bytes / sizeof(f32)
      s32 nu,   //!< Number of columns of U
      f64* vT,  //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT, //!< V_stride_in_bytes / sizeof(f32)
      f64* buffer //!< A scratch buffer, with at least "sizeof(f32)*(n_stride*2 + m_stride)" bytes
      );

    void icvSVBkSb_32f(
      s32 m,         //!< Number of rows in u
      s32 n,         //!< Number of rows in v
      const f32* w,  //!< Pointer to start of the W vector
      const f32* uT, //!< Pointer to the upper-left of the U-transpose array
      s32 lduT,      //!< U_stride_in_bytes / sizeof(f32)
      const f32* vT, //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT,      //!< V_stride_in_bytes / sizeof(f32)
      const f32* b,  //!< Pointer to the upper-left of the b array
      s32 ldb,       //!< b_stride_in_bytes / sizeof(f32)
      s32 nb,        //!< Number of columns in B
      f32* x,        //!< Pointer to the start of the x vector (I think x must always be Sx1, for S either m or n?)
      s32 ldx,       //!< x_stride_in_bytes / sizeof(f32)
      f32* buffer    //!< A scratch buffer, with at least "sizeof(f32)*b_stride" bytes
      );

    void icvSVBkSb_64f(
      s32 m,         //!< Number of rows in u
      s32 n,         //!< Number of rows in v
      const f64* w,  //!< Pointer to start of the W vector
      const f64* uT, //!< Pointer to the upper-left of the U-transpose array
      s32 lduT,      //!< U_stride_in_bytes / sizeof(f32)
      const f64* vT, //!< Pointer to the upper-left of the V-transpose array
      s32 ldvT,      //!< V_stride_in_bytes / sizeof(f32)
      const f64* b,  //!< Pointer to the upper-left of the b array
      s32 ldb,       //!< b_stride_in_bytes / sizeof(f32)
      s32 nb,        //!< Number of columns in B
      f64* x,        //!< Pointer to the start of the x vector (I think x must always be Sx1, for S either m or n?)
      s32 ldx,       //!< x_stride_in_bytes / sizeof(f32)
      f64* buffer    //!< A scratch buffer, with at least "sizeof(f32)*b_stride" bytes
      );
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_OPENCVLIGHT_H_
