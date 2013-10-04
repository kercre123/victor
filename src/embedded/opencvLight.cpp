/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "anki/embeddedCommon/opencvLight.h"
#include "anki/embeddedCommon/errorHandling.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#include <malloc.h>
#endif

#ifdef __cplusplus
}
#endif

// Movidius compiler is missing alloca()
/* ! DO NOT make it an inline function */
//#define cvStackAlloc(size) cvAlignPtr( alloca((size) + CV_MALLOC_ALIGN), CV_MALLOC_ALIGN )

#define CV_SWAP(a,b,t) ((t) = (a), (a) = (b), (b) = (t))

#define MAX_ITERS  30

/* the alignment of all the allocated buffers */
#define  CV_MALLOC_ALIGN    32

#define icvGivens_32f( n, x, y, c, s ) \
{                                      \
  int _i;                              \
  float* _x = (x);                     \
  float* _y = (y);                     \
  for( _i = 0; _i < n; _i++ )          \
{                                  \
  double t0 = _x[_i];                \
  double t1 = _y[_i];                \
  _x[_i] = (float)(t0*c + t1*s);     \
  _y[_i] = (float)(-t0*s + t1*c);    \
}                                  \
}

#define icvGivens_64f( n, x, y, c, s ) \
{                                      \
  int _i;                              \
  double* _x = (x);                    \
  double* _y = (y);                    \
  for( _i = 0; _i < n; _i++ )          \
{                                  \
  double t0 = _x[_i];                \
  double t1 = _y[_i];                \
  _x[_i] = t0*c + t1*s;              \
  _y[_i] = -t0*s + t1*c;             \
}                                  \
}

namespace Anki
{
  namespace Embedded
  {
    inline void* cvAlignPtr( const void* ptr, int align=32 )
    {
      assert( (align & (align-1)) == 0 );
      return (void*)( ((size_t)ptr + align - 1) & ~(size_t)(align-1) );
    }

    IN_DDR static void icvMatrAXPY3_32f( int m, int n, const float* x, int l, float* y, double h )
    {
      int i, j;

      for( i = 1; i < m; i++ )
      {
        double s = 0;
        y += l;

        for( j = 0; j <= n - 4; j += 4 )
          s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = (float)(s*x[-1]);

        for( j = 0; j <= n - 4; j += 4 )
        {
          double t0 = y[j]   + s*x[j];
          double t1 = y[j+1] + s*x[j+1];
          y[j]   = (float)t0;
          y[j+1] = (float)t1;
          t0 = y[j+2] + s*x[j+2];
          t1 = y[j+3] + s*x[j+3];
          y[j+2] = (float)t0;
          y[j+3] = (float)t1;
        }

        for( ; j < n; j++ ) y[j] = (float)(y[j] + s*x[j]);
      }
    }

    /* y[1:m,-1] = h*y[1:m,0:n]*x[0:1,0:n]'*x[-1]  (this is used for U&V reconstruction)
    y[1:m,0:n] += h*y[1:m,0:n]*x[0:1,0:n]'*x[0:1,0:n] */
    IN_DDR static void icvMatrAXPY3_64f( int m, int n, const double* x, int l, double* y, double h )
    {
      int i, j;

      for( i = 1; i < m; i++ )
      {
        double s = 0;

        y += l;

        for( j = 0; j <= n - 4; j += 4 )
          s += x[j]*y[j] + x[j+1]*y[j+1] + x[j+2]*y[j+2] + x[j+3]*y[j+3];

        for( ; j < n; j++ )  s += x[j]*y[j];

        s *= h;
        y[-1] = s*x[-1];

        for( j = 0; j <= n - 4; j += 4 )
        {
          double t0 = y[j]   + s*x[j];
          double t1 = y[j+1] + s*x[j+1];
          y[j]   = t0;
          y[j+1] = t1;
          t0 = y[j+2] + s*x[j+2];
          t1 = y[j+3] + s*x[j+3];
          y[j+2] = t0;
          y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
      }
    }

    /* accurate hypotenuse calculation */
    IN_DDR static double pythag( double a, double b )
    {
      a = fabs( a );
      b = fabs( b );
      if( a > b )
      {
        b /= a;
        a *= sqrt( 1. + b * b );
      }
      else if( b != 0 )
      {
        a /= b;
        a = b * sqrt( 1. + a * a );
      }

      return a;
    }

    IN_DDR static void icvMatrAXPY_32f( int m, int n, const float* x, int dx,
      const float* a, float* y, int dy )
    {
      int i, j;

      for( i = 0; i < m; i++, x += dx, y += dy )
      {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
          double t0 = y[j]   + s*x[j];
          double t1 = y[j+1] + s*x[j+1];
          y[j]   = (float)t0;
          y[j+1] = (float)t1;
          t0 = y[j+2] + s*x[j+2];
          t1 = y[j+3] + s*x[j+3];
          y[j+2] = (float)t0;
          y[j+3] = (float)t1;
        }

        for( ; j < n; j++ )
          y[j] = (float)(y[j] + s*x[j]);
      }
    }

    /* y[0:m,0:n] += diag(a[0:1,0:m]) * x[0:m,0:n] */
    IN_DDR static void icvMatrAXPY_64f( int m, int n, const double* x, int dx,
      const double* a, double* y, int dy )
    {
      int i, j;

      for( i = 0; i < m; i++, x += dx, y += dy )
      {
        double s = a[i];

        for( j = 0; j <= n - 4; j += 4 )
        {
          double t0 = y[j]   + s*x[j];
          double t1 = y[j+1] + s*x[j+1];
          y[j]   = t0;
          y[j+1] = t1;
          t0 = y[j+2] + s*x[j+2];
          t1 = y[j+3] + s*x[j+3];
          y[j+2] = t0;
          y[j+3] = t1;
        }

        for( ; j < n; j++ ) y[j] += s*x[j];
      }
    }

    /*! Performs an Singular Value Decomposition on the mXn, float32 input array. [u^t,w,v^t] = SVD(a); */
    IN_DDR Result svd_f32(
      Array<f32> &a, //!< Input array mXn
      Array<f32> &w, //!< W array 1Xm
      Array<f32> &uT, //!< U-transpose array mXm
      Array<f32> &vT, //!< V-transpose array nXn
      void * scratch //!< A scratch buffer, with at least "sizeof(float)*(n*2 + m*2 + 64)" bytes
      )
    {
      const s32 m = a.get_size(0); // m
      const s32 n = a.get_size(1); // n

      AnkiConditionalErrorAndReturnValue(a.IsValid(),
        RESULT_FAIL, "svd_f32", "a is not valid");

      AnkiConditionalErrorAndReturnValue(w.IsValid(),
        RESULT_FAIL, "svd_f32", "w is not valid");

      AnkiConditionalErrorAndReturnValue(uT.IsValid(),
        RESULT_FAIL, "svd_f32", "uT is not valid");

      AnkiConditionalErrorAndReturnValue(vT.IsValid(),
        RESULT_FAIL, "svd_f32", "vT is not valid");

      AnkiConditionalErrorAndReturnValue(scratch,
        RESULT_FAIL, "svd_f32", "scratch is null");

      AnkiConditionalErrorAndReturnValue(w.get_size(0) == 1 && w.get_size(1) == n,
        RESULT_FAIL, "svd_f32", "w is not mXn");

      AnkiConditionalErrorAndReturnValue(uT.get_size(0) == m && uT.get_size(1) == m,
        RESULT_FAIL, "svd_f32", "uT is not mXm");

      AnkiConditionalErrorAndReturnValue(vT.get_size(0) == n && vT.get_size(1) == n,
        RESULT_FAIL, "svd_f32", "vT is not nXn");

      icvLightSVD_32f(
        a.Pointer(0,0),
        a.get_stride() / sizeof(f32),
        m,
        n,
        w.Pointer(0,0),
        uT.Pointer(0,0),
        uT.get_stride() / sizeof(f32),
        uT.get_size(1),
        vT.Pointer(0,0),
        vT.get_stride() / sizeof(f32),
        reinterpret_cast<f32*>(scratch));

      AnkiConditionalErrorAndReturnValue(a.IsValid(),
        RESULT_FAIL, "svd_f32", "After call: a is not valid");

      AnkiConditionalErrorAndReturnValue(w.IsValid(),
        RESULT_FAIL, "svd_f32", "After call: w is not valid");

      AnkiConditionalErrorAndReturnValue(uT.IsValid(),
        RESULT_FAIL, "svd_f32", "After call: uT is not valid");

      AnkiConditionalErrorAndReturnValue(vT.IsValid(),
        RESULT_FAIL, "svd_f32", "After call: vT is not valid");

      return RESULT_OK;
    }

    /*! Performs an Singular Value Decomposition on the mXn, float64 input array. [u^t,w,v^t] = SVD(a); */
    IN_DDR Result svd_f64(
      Array<f64> &a,  //!< Input array mXn
      Array<f64> &w,  //!< W array 1xm
      Array<f64> &uT, //!< U-transpose array mXm
      Array<f64> &vT, //!< V-transpose array nXn
      void * scratch //!< A scratch buffer, with at least "sizeof(f64)*(n*2 + m*2 + 64)" bytes
      )
    {
      const s32 m = a.get_size(0); // m
      const s32 n = a.get_size(1); // n

      AnkiConditionalErrorAndReturnValue(a.IsValid(),
        RESULT_FAIL, "svd_f64", "a is not valid");

      AnkiConditionalErrorAndReturnValue(w.IsValid(),
        RESULT_FAIL, "svd_f64", "w is not valid");

      AnkiConditionalErrorAndReturnValue(uT.IsValid(),
        RESULT_FAIL, "svd_f64", "uT is not valid");

      AnkiConditionalErrorAndReturnValue(vT.IsValid(),
        RESULT_FAIL, "svd_f64", "vT is not valid");

      AnkiConditionalErrorAndReturnValue(scratch,
        RESULT_FAIL, "svd_f64", "scratch is null");

      AnkiConditionalErrorAndReturnValue(w.get_size(0) == 1 && w.get_size(1) == n,
        RESULT_FAIL, "svd_f64", "w is not mXn");

      AnkiConditionalErrorAndReturnValue(uT.get_size(0) == m && uT.get_size(1) == m,
        RESULT_FAIL, "svd_f64", "uT is not mXm");

      AnkiConditionalErrorAndReturnValue(vT.get_size(0) == n && vT.get_size(1) == n,
        RESULT_FAIL, "svd_f64", "vT is not nXn");

      icvLightSVD_64f(
        a.Pointer(0,0),
        a.get_stride() / sizeof(f64),
        m,
        n,
        w.Pointer(0,0),
        uT.Pointer(0,0),
        uT.get_stride() / sizeof(f64),
        uT.get_size(1),
        vT.Pointer(0,0),
        vT.get_stride() / sizeof(f64),
        reinterpret_cast<f64*>(scratch));

      AnkiConditionalErrorAndReturnValue(a.IsValid(),
        RESULT_FAIL, "svd_f64", "After call: a is not valid");

      AnkiConditionalErrorAndReturnValue(w.IsValid(),
        RESULT_FAIL, "svd_f64", "After call: w is not valid");

      AnkiConditionalErrorAndReturnValue(uT.IsValid(),
        RESULT_FAIL, "svd_f64", "After call: uT is not valid");

      AnkiConditionalErrorAndReturnValue(vT.IsValid(),
        RESULT_FAIL, "svd_f64", "After call: vT is not valid");

      return RESULT_OK;
    }

    /*!
    Performs an Singular Value Decomposition on the mXn, float32 input array. [w,u^t,v^t] = SVD(a);
    WARNING: I think that if any of the input arrays have stride padding,
    the stride padding must be set to zero before calling.
    */
    IN_DDR void icvLightSVD_32f(
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
      f32* buffer //!< A scratch buffer, with at least "sizeof(double)*(2n + 2m + 64)" bytes
      )
    {
      float* e;
      float* temp;
      float *w1, *e1;
      float *hv;
      double ku0 = 0, kv0 = 0;
      double anorm = 0;
      float *a1, *u0 = uT, *v0 = vT;
      double scale, h;
      int i, j, k, l;
      int nm, m1, n1;
      int nv = n;
      int iters = 0;

      // Movidius compiler is missing alloca()
      //float* hv0 = (float*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1;

      float* hv0 = (float*) cvAlignPtr( buffer, CV_MALLOC_ALIGN ) + 1;
      buffer = hv0;
      buffer += (m+2)*sizeof(hv0[0]) + 32;

      e = buffer;

      w1 = w;
      e1 = e + 1;
      nm = n;

      temp = buffer + nm;

      memset( w, 0, nm * sizeof( w[0] ));
      memset( e, 0, nm * sizeof( e[0] ));

      m1 = m;
      n1 = n;

      /* transform a to bi-diagonal form */
      for( ;; )
      {
        int update_u;
        int update_v;

        if( m1 == 0 )
          break;

        scale = h = 0;

        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
          double t = a1[0];
          scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
          double f = 1./scale, g, s = 0;

          for( j = 0; j < m1; j++ )
          {
            double t = (hv[j] = (float)(hv[j]*f));
            s += t * t;
          }

          g = sqrt( s );
          f = hv[0];
          if( f >= 0 )
            g = -g;
          hv[0] = (float)(f - g);
          h = 1. / (f * g - s);

          memset( temp, 0, n1 * sizeof( temp[0] ));

          /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
          icvMatrAXPY_32f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );

          for( k = 1; k < n1; k++ ) temp[k] = (float)(temp[k]*h);

          /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
          icvMatrAXPY_32f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
          *w1 = (float)(g*scale);
        }
        w1++;

        /* store -2/(hv'*hv) */
        if( update_u )
        {
          if( m1 == m )
            ku0 = h;
          else
            hv[-1] = (float)h;
        }

        a++;
        n1--;
        if( vT )
          vT += ldvT + 1;

        if( n1 == 0 )
          break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;
        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
          double t = a[j];
          scale += fabs( hv[j] = (float)t );
        }

        if( scale != 0 )
        {
          double f = 1./scale, g, s = 0;

          for( j = 0; j < n1; j++ )
          {
            double t = (hv[j] = (float)(hv[j]*f));
            s += t * t;
          }

          g = sqrt( s );
          f = hv[0];
          if( f >= 0 )
            g = -g;
          hv[0] = (float)(f - g);
          h = 1. / (f * g - s);
          hv[-1] = 0.f;

          /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
          icvMatrAXPY3_32f( m1, n1, hv, lda, a, h );

          *e1 = (float)(g*scale);
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
          if( n1 == n )
            kv0 = h;
          else
            hv[-1] = (float)h;
        }

        a += lda;
        m1--;
        if( uT )
          uT += lduT + 1;
      }

      m1 -= m1 != 0;
      n1 -= n1 != 0;

      /* accumulate left transformations */
      if( uT )
      {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
          memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
          uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
          double s;
          int lh = nu - i;

          l = m - i;

          hv = u0 + (lduT + 1) * i;
          h = i == 0 ? ku0 : hv[-1];

          assert( h <= 0 );

          if( h != 0 )
          {
            uT = hv;
            icvMatrAXPY3_32f( lh, l-1, hv+1, lduT, uT+1, h );

            s = hv[0] * h;
            for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
            hv[0] += 1;
          }
          else
          {
            for( j = 1; j < l; j++ )
              hv[j] = 0;
            for( j = 1; j < lh; j++ )
              hv[j * lduT] = 0;
            hv[0] = 1;
          }
        }
        uT = u0;
      }

      /* accumulate right transformations */
      if( vT )
      {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
          memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
          vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
          double s;
          int lh = nv - i;

          l = n - i;
          hv = v0 + (ldvT + 1) * i;
          h = i == 0 ? kv0 : hv[-1];

          assert( h <= 0 );

          if( h != 0 )
          {
            vT = hv;
            icvMatrAXPY3_32f( lh, l-1, hv+1, ldvT, vT+1, h );

            s = hv[0] * h;
            for( k = 0; k < l; k++ ) hv[k] = (float)(hv[k]*s);
            hv[0] += 1;
          }
          else
          {
            for( j = 1; j < l; j++ )
              hv[j] = 0;
            for( j = 1; j < lh; j++ )
              hv[j * ldvT] = 0;
            hv[0] = 1;
          }
        }
        vT = v0;
      }

      for( i = 0; i < nm; i++ )
      {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
          anorm = tnorm;
      }

      anorm *= FLT_EPSILON;

      /* diagonalization of the bidiagonal form */
      for( k = nm - 1; k >= 0; k-- )
      {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
          double c, s, f, g, x, y;
          int flag = 0;

          /* test for splitting */
          for( l = k; l >= 0; l-- )
          {
            if( fabs( e[l] ) <= anorm )
            {
              flag = 1;
              break;
            }
            assert( l > 0 );
            if( fabs( w[l - 1] ) <= anorm )
              break;
          }

          if( !flag )
          {
            c = 0;
            s = 1;

            for( i = l; i <= k; i++ )
            {
              f = s * e[i];
              e[i] = (float)(e[i]*c);

              if( anorm + fabs( f ) == anorm )
                break;

              g = w[i];
              h = pythag( f, g );
              w[i] = (float)h;
              c = g / h;
              s = -f / h;

              if( uT )
                icvGivens_32f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
            }
          }

          z = w[k];
          if( l == k || iters++ == MAX_ITERS )
            break;

          /* shift from bottom 2x2 minor */
          x = w[l];
          y = w[k - 1];
          g = e[k - 1];
          h = e[k];
          f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
          g = pythag( f, 1 );
          if( f < 0 )
            g = -g;
          f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
          /* next QR transformation */
          c = s = 1;

          for( i = l + 1; i <= k; i++ )
          {
            g = e[i];
            y = w[i];
            h = s * g;
            g *= c;
            z = pythag( f, h );
            e[i - 1] = (float)z;
            c = f / z;
            s = h / z;
            f = x * c + g * s;
            g = -x * s + g * c;
            h = y * s;
            y *= c;

            if( vT )
              icvGivens_32f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

            z = pythag( f, h );
            w[i - 1] = (float)z;

            /* rotation can be arbitrary if z == 0 */
            if( z != 0 )
            {
              c = f / z;
              s = h / z;
            }
            f = c * g + s * y;
            x = -s * g + c * y;

            if( uT )
              icvGivens_32f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
          }

          e[l] = 0;
          e[k] = (float)f;
          w[k] = (float)x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
          break;

        if( z < 0 )
        {
          w[k] = (float)(-z);
          if( vT )
          {
            for( j = 0; j < n; j++ )
              vT[j + k * ldvT] = -vT[j + k * ldvT];
          }
        }
      }                           /* end of diagonalization loop */

      /* sort singular values and corresponding vectors */
      for( i = 0; i < nm; i++ )
      {
        k = i;
        for( j = i + 1; j < nm; j++ )
          if( w[k] < w[j] )
            k = j;

        if( k != i )
        {
          float t;
          CV_SWAP( w[i], w[k], t );

          if( vT )
            for( j = 0; j < n; j++ )
              CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

          if( uT )
            for( j = 0; j < m; j++ )
              CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
      }
    }

    /*!
    Performs an Singular Value Decomposition on the mXn, float32 input array. [w,u^t,v^t] = SVD(a);
    WARNING: I think that if any of the input arrays have stride padding,
    the stride padding must be set to zero before calling.
    */
    IN_DDR void icvLightSVD_64f(
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
      f64* buffer //!< A scratch buffer, with at least "sizeof(double)*(2n + 2m + 64)" bytes
      )
    {
      double* e;
      double* temp;
      double *w1, *e1;
      double *hv;
      double ku0 = 0, kv0 = 0;
      double anorm = 0;
      double *a1, *u0 = uT, *v0 = vT;
      double scale, h;
      int i, j, k, l;
      int nm, m1, n1;
      int nv = n;
      int iters = 0;

      // Movidius compiler is missing alloca()
      // double* hv0 = (double*)cvStackAlloc( (m+2)*sizeof(hv0[0])) + 1;

      double* hv0 = (double*) cvAlignPtr( buffer, CV_MALLOC_ALIGN ) + 1;
      //memset(hv0, 0, (m+2)*sizeof(hv0[0]) + 32);
      buffer = hv0;
      buffer += (m+2)*sizeof(hv0[0]) + 32;

      e = buffer;
      w1 = w;
      e1 = e + 1;
      nm = n;

      temp = buffer + nm;

      memset( w, 0, nm * sizeof( w[0] ));
      memset( e, 0, nm * sizeof( e[0] ));

      m1 = m;
      n1 = n;

      /* transform a to bi-diagonal form */
      for( ;; )
      {
        int update_u;
        int update_v;

        if( m1 == 0 )
          break;

        scale = h = 0;
        update_u = uT && m1 > m - nu;
        hv = update_u ? uT : hv0;

        for( j = 0, a1 = a; j < m1; j++, a1 += lda )
        {
          double t = a1[0];
          scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
          double f = 1./scale, g, s = 0;

          for( j = 0; j < m1; j++ )
          {
            double t = (hv[j] *= f);
            s += t * t;
          }

          g = sqrt( s );
          f = hv[0];
          if( f >= 0 )
            g = -g;
          hv[0] = f - g;
          h = 1. / (f * g - s);

          memset( temp, 0, n1 * sizeof( temp[0] ));

          /* calc temp[0:n-i] = a[i:m,i:n]'*hv[0:m-i] */
          icvMatrAXPY_64f( m1, n1 - 1, a + 1, lda, hv, temp + 1, 0 );
          for( k = 1; k < n1; k++ ) temp[k] *= h;

          /* modify a: a[i:m,i:n] = a[i:m,i:n] + hv[0:m-i]*temp[0:n-i]' */
          icvMatrAXPY_64f( m1, n1 - 1, temp + 1, 0, hv, a + 1, lda );
          *w1 = g*scale;
        }
        w1++;

        /* store -2/(hv'*hv) */
        if( update_u )
        {
          if( m1 == m )
            ku0 = h;
          else
            hv[-1] = h;
        }

        a++;
        n1--;
        if( vT )
          vT += ldvT + 1;

        if( n1 == 0 )
          break;

        scale = h = 0;
        update_v = vT && n1 > n - nv;

        hv = update_v ? vT : hv0;

        for( j = 0; j < n1; j++ )
        {
          double t = a[j];
          scale += fabs( hv[j] = t );
        }

        if( scale != 0 )
        {
          double f = 1./scale, g, s = 0;

          for( j = 0; j < n1; j++ )
          {
            double t = (hv[j] *= f);
            s += t * t;
          }

          g = sqrt( s );
          f = hv[0];
          if( f >= 0 )
            g = -g;
          hv[0] = f - g;
          h = 1. / (f * g - s);
          hv[-1] = 0.;

          /* update a[i:m:i+1:n] = a[i:m,i+1:n] + (a[i:m,i+1:n]*hv[0:m-i])*... */
          icvMatrAXPY3_64f( m1, n1, hv, lda, a, h );

          *e1 = g*scale;
        }
        e1++;

        /* store -2/(hv'*hv) */
        if( update_v )
        {
          if( n1 == n )
            kv0 = h;
          else
            hv[-1] = h;
        }

        a += lda;
        m1--;
        if( uT )
          uT += lduT + 1;
      }

      m1 -= m1 != 0;
      n1 -= n1 != 0;

      /* accumulate left transformations */
      if( uT )
      {
        m1 = m - m1;
        uT = u0 + m1 * lduT;
        for( i = m1; i < nu; i++, uT += lduT )
        {
          memset( uT + m1, 0, (m - m1) * sizeof( uT[0] ));
          uT[i] = 1.;
        }

        for( i = m1 - 1; i >= 0; i-- )
        {
          double s;
          int lh = nu - i;

          l = m - i;

          hv = u0 + (lduT + 1) * i;
          h = i == 0 ? ku0 : hv[-1];

          assert( h <= 0 );

          if( h != 0 )
          {
            uT = hv;
            icvMatrAXPY3_64f( lh, l-1, hv+1, lduT, uT+1, h );

            s = hv[0] * h;
            for( k = 0; k < l; k++ ) hv[k] *= s;
            hv[0] += 1;
          }
          else
          {
            for( j = 1; j < l; j++ )
              hv[j] = 0;
            for( j = 1; j < lh; j++ )
              hv[j * lduT] = 0;
            hv[0] = 1;
          }
        }
        uT = u0;
      }

      /* accumulate right transformations */
      if( vT )
      {
        n1 = n - n1;
        vT = v0 + n1 * ldvT;
        for( i = n1; i < nv; i++, vT += ldvT )
        {
          memset( vT + n1, 0, (n - n1) * sizeof( vT[0] ));
          vT[i] = 1.;
        }

        for( i = n1 - 1; i >= 0; i-- )
        {
          double s;
          int lh = nv - i;

          l = n - i;
          hv = v0 + (ldvT + 1) * i;
          h = i == 0 ? kv0 : hv[-1];

          assert( h <= 0 );

          if( h != 0 )
          {
            vT = hv;
            icvMatrAXPY3_64f( lh, l-1, hv+1, ldvT, vT+1, h );

            s = hv[0] * h;
            for( k = 0; k < l; k++ ) hv[k] *= s;
            hv[0] += 1;
          }
          else
          {
            for( j = 1; j < l; j++ )
              hv[j] = 0;
            for( j = 1; j < lh; j++ )
              hv[j * ldvT] = 0;
            hv[0] = 1;
          }
        }
        vT = v0;
      }

      for( i = 0; i < nm; i++ )
      {
        double tnorm = fabs( w[i] );
        tnorm += fabs( e[i] );

        if( anorm < tnorm )
          anorm = tnorm;
      }

      anorm *= DBL_EPSILON;

      /* diagonalization of the bidiagonal form */
      for( k = nm - 1; k >= 0; k-- )
      {
        double z = 0;
        iters = 0;

        for( ;; )               /* do iterations */
        {
          double c, s, f, g, x, y;
          int flag = 0;

          /* test for splitting */
          for( l = k; l >= 0; l-- )
          {
            if( fabs(e[l]) <= anorm )
            {
              flag = 1;
              break;
            }
            assert( l > 0 );
            if( fabs(w[l - 1]) <= anorm )
              break;
          }

          if( !flag )
          {
            c = 0;
            s = 1;

            for( i = l; i <= k; i++ )
            {
              f = s * e[i];

              e[i] *= c;

              if( anorm + fabs( f ) == anorm )
                break;

              g = w[i];
              h = pythag( f, g );
              w[i] = h;
              c = g / h;
              s = -f / h;

              if( uT )
                icvGivens_64f( m, uT + lduT * (l - 1), uT + lduT * i, c, s );
            }
          }

          z = w[k];
          if( l == k || iters++ == MAX_ITERS )
            break;

          /* shift from bottom 2x2 minor */
          x = w[l];
          y = w[k - 1];
          g = e[k - 1];
          h = e[k];
          f = 0.5 * (((g + z) / h) * ((g - z) / y) + y / h - h / y);
          g = pythag( f, 1 );
          if( f < 0 )
            g = -g;
          f = x - (z / x) * z + (h / x) * (y / (f + g) - h);
          /* next QR transformation */
          c = s = 1;

          for( i = l + 1; i <= k; i++ )
          {
            g = e[i];
            y = w[i];
            h = s * g;
            g *= c;
            z = pythag( f, h );
            e[i - 1] = z;
            c = f / z;
            s = h / z;
            f = x * c + g * s;
            g = -x * s + g * c;
            h = y * s;
            y *= c;

            if( vT )
              icvGivens_64f( n, vT + ldvT * (i - 1), vT + ldvT * i, c, s );

            z = pythag( f, h );
            w[i - 1] = z;

            /* rotation can be arbitrary if z == 0 */
            if( z != 0 )
            {
              c = f / z;
              s = h / z;
            }
            f = c * g + s * y;
            x = -s * g + c * y;

            if( uT )
              icvGivens_64f( m, uT + lduT * (i - 1), uT + lduT * i, c, s );
          }

          e[l] = 0;
          e[k] = f;
          w[k] = x;
        }                       /* end of iteration loop */

        if( iters > MAX_ITERS )
          break;

        if( z < 0 )
        {
          w[k] = -z;
          if( vT )
          {
            for( j = 0; j < n; j++ )
              vT[j + k * ldvT] = -vT[j + k * ldvT];
          }
        }
      }                           /* end of diagonalization loop */

      /* sort singular values and corresponding values */
      for( i = 0; i < nm; i++ )
      {
        k = i;
        for( j = i + 1; j < nm; j++ )
          if( w[k] < w[j] )
            k = j;

        if( k != i )
        {
          double t;
          CV_SWAP( w[i], w[k], t );

          if( vT )
            for( j = 0; j < n; j++ )
              CV_SWAP( vT[j + ldvT*k], vT[j + ldvT*i], t );

          if( uT )
            for( j = 0; j < m; j++ )
              CV_SWAP( uT[j + lduT*k], uT[j + lduT*i], t );
        }
      }
    }

    /*! Compute the homography such that "transformedPoints = homography * originalPoints" */
    IN_DDR Result EstimateHomography(
      const FixedLengthList<Point<f64> > &originalPoints,    //!<
      const FixedLengthList<Point<f64> > &transformedPoints, //!<
      Array<f64> &homography, //!<
      MemoryStack &scratch //!<
      )
    {
      const s32 count = originalPoints.get_size();
      const Point<f64> * M = originalPoints.Pointer(0);
      const Point<f64> * m = transformedPoints.Pointer(0);

      Array<f64> _LtL = Array<f64>(9, 9, scratch);
      Array<f64> _W = Array<f64>(1, 9, scratch); // Swapper
      Array<f64> _V = Array<f64>(9, 9, scratch);
      Array<f64> _homography0 = Array<f64>(3, 3, scratch);
      Array<f64> _homographyTemp = Array<f64>(3, 3, scratch);

      Point<f64> cM(0,0), cm(0,0), sM(0,0), sm(0,0);

      for(s32 iPoint = 0; iPoint < count; iPoint++) {
        cm.x += m[iPoint].x; cm.y += m[iPoint].y;
        cM.x += M[iPoint].x; cM.y += M[iPoint].y;
      }

      cm.x /= count; cm.y /= count;
      cM.x /= count; cM.y /= count;

      for(s32 iPoint = 0; iPoint < count; iPoint++) {
        sm.x += fabs(m[iPoint].x - cm.x);
        sm.y += fabs(m[iPoint].y - cm.y);
        sM.x += fabs(M[iPoint].x - cM.x);
        sM.y += fabs(M[iPoint].y - cM.y);
      }

      if( fabs(sm.x) < DBL_EPSILON || fabs(sm.y) < DBL_EPSILON || fabs(sM.x) < DBL_EPSILON || fabs(sM.y) < DBL_EPSILON )
        return RESULT_FAIL;

      sm.x = count/sm.x; sm.y = count/sm.y;
      sM.x = count/sM.x; sM.y = count/sM.y;

      Array<f64> _invHomographyNorm = Array<f64>(3, 3, scratch);
      Array<f64> _homographyNorm2 = Array<f64>(3, 3, scratch);

      *_invHomographyNorm.Pointer(0,0) = 1./sm.x;  *_invHomographyNorm.Pointer(0,1) = 0;       *_invHomographyNorm.Pointer(0,2) = cm.x;
      *_invHomographyNorm.Pointer(1,0) = 0;        *_invHomographyNorm.Pointer(1,1) = 1./sm.y; *_invHomographyNorm.Pointer(1,2) = cm.y;
      *_invHomographyNorm.Pointer(2,0) = 0;        *_invHomographyNorm.Pointer(2,1) = 0;       *_invHomographyNorm.Pointer(2,2) = 1;

      *_homographyNorm2.Pointer(0,0) = sM.x;  *_homographyNorm2.Pointer(0,1) = 0;     *_homographyNorm2.Pointer(0,2) = -cM.x*sM.x;
      *_homographyNorm2.Pointer(1,0) = 0;     *_homographyNorm2.Pointer(1,1) = sM.y;  *_homographyNorm2.Pointer(1,2) = -cM.y*sM.y;
      *_homographyNorm2.Pointer(2,0) = 0;     *_homographyNorm2.Pointer(2,1) = 0;     *_homographyNorm2.Pointer(2,2) = 1;

      _LtL.Set(0.0);

      for(s32 iPoint = 0; iPoint < count; iPoint++) {
        double x = (m[iPoint].x - cm.x)*sm.x, y = (m[iPoint].y - cm.y)*sm.y;
        double X = (M[iPoint].x - cM.x)*sM.x, Y = (M[iPoint].y - cM.y)*sM.y;
        double Lx[] = { X, Y, 1, 0, 0, 0, -x*X, -x*Y, -x };
        double Ly[] = { 0, 0, 0, X, Y, 1, -y*X, -y*Y, -y };

        for(s32 j = 0; j < 9; j++){
          for(s32 k = j; k < 9; k++) {
            *_LtL.Pointer(j,k) += Lx[j]*Lx[k] + Ly[j]*Ly[k];
          }
        }
      }
      MakeArraySymmetric(_LtL, false);

      Result result;
      {
        // Push the current state of the scratch buffer onto the system stack
        const MemoryStack scratch_tmp = scratch;
        MemoryStack scratch(scratch_tmp);

        Array<f64> uT(_LtL.get_size(0), _LtL.get_size(0), scratch);
        void * svdScratchBuffer = scratch.Allocate(sizeof(f64)*(_LtL.get_size(1)*2 + _LtL.get_size(0)*2 + 64));

        result = svd_f64(_LtL, _W, uT, _V, svdScratchBuffer);

        AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
          RESULT_FAIL, "EstimateHomography", "After call: scratch is not valid");
      }

      AnkiConditionalErrorAndReturnValue(result == RESULT_OK,
        RESULT_FAIL, "EstimateHomography", "svd_f64 failed");

      {
        s32 ci = 0;
        const f64 * const V_rowPointer = _V.Pointer(8,0);
        for(s32 y=0; y<3; y++) {
          for(s32 x=0; x<3; x++) {
            (*_homography0.Pointer(y,x)) = V_rowPointer[ci++];
          }
        }
      }

      MultiplyMatrices<Array<f64>,f64>(_invHomographyNorm, _homography0, _homographyTemp);
      MultiplyMatrices<Array<f64>,f64>(_homographyTemp, _homographyNorm2, homography);

      {
        const f64 inverseHomogeneousScale = 1.0 / (*homography.Pointer(2,2));
        for(s32 y=0; y<3; y++) {
          for(s32 x=0; x<3; x++) {
            (*homography.Pointer(y,x)) *= inverseHomogeneousScale;
          }
        }
      }

      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki