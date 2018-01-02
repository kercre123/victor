/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
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
//   * The name of the copyright holders may not be used to endorse or promote products
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

#include "coretech/vision/robot/opencvLight_vision.h"

#include "coretech/common/robot/errorHandling.h"
#include "coretech/common/robot/matrix.h"
#include "coretech/common/robot/comparisons.h"

#include "coretech/vision/robot/imageProcessing.h"

namespace Anki
{
  namespace Embedded
  {
    Result CannyEdgeDetection(
      const Array<u8> &src, Array<u8> &dst,
      const s32 low_thresh, const s32 high_thresh,
      const s32 aperture_size,
      MemoryStack scratch)
    {
      BeginBenchmark("Canny_init");

      const s32 imageHeight = src.get_size(0);
      const s32 imageWidth = src.get_size(1);

      AnkiConditionalErrorAndReturnValue(src.IsValid() && dst.IsValid() && scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "CannyEdgeDetection", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(AreEqualSize(src, dst),
        RESULT_FAIL_INVALID_SIZE, "CannyEdgeDetection", "src and dst must be the same size");

      if ((aperture_size & 1) == 0 || (aperture_size != -1 && (aperture_size < 3 || aperture_size > 7))) {
        AnkiError("CannyEdgeDetection", "invalid aperture_size");
        return RESULT_FAIL_INVALID_PARAMETER;
      }

      AnkiConditionalErrorAndReturnValue(low_thresh <= high_thresh,
        RESULT_FAIL_INVALID_PARAMETER, "CannyEdgeDetection", "Invalid threshold");

      Array<s16> dx(imageHeight, imageWidth, scratch, Flags::Buffer(false, false, false));
      Array<s16> dy(imageHeight, imageWidth, scratch, Flags::Buffer(false, false, false));

      //Sobel(src, dx, CV_16S, 1, 0, aperture_size, 1, 0, cv::BORDER_REPLICATE);
      //Sobel(src, dy, CV_16S, 0, 1, aperture_size, 1, 0, cv::BORDER_REPLICATE);

      BeginBenchmark("Canny_init_gradient");

      ImageProcessing::ComputeXGradient<u8,s16,s16>(src, dx);
      ImageProcessing::ComputeYGradient<u8,s16,s16>(src, dy);

      EndBenchmark("Canny_init_gradient");

      s32 mapstep = imageWidth + 2;
      s32 numBufferBytes;
      u8 * buffer = reinterpret_cast<u8*>( scratch.Allocate((imageWidth+2)*(imageHeight+2) + mapstep * 3 * sizeof(int), false, numBufferBytes) );

      int* mag_buf[3];
      mag_buf[0] = (int*)(u8*)buffer;
      mag_buf[1] = mag_buf[0] + mapstep;
      mag_buf[2] = mag_buf[1] + mapstep;
      memset(mag_buf[0], 0, /* cn* */mapstep*sizeof(int));

      u8* map = (u8*)(mag_buf[2] + mapstep);
      memset(map, 1, mapstep);
      memset(map + mapstep*(imageHeight + 1), 1, mapstep);

      //int maxsize = MAX(1 << 10, imageWidth * imageHeight / 10);
      //std::vector<u8*> stack(maxsize);

      const int maxsize = MAX(1 << 10, imageWidth * imageHeight / 3);
      FixedLengthList<u8*> stack(maxsize, scratch, Flags::Buffer(false, false, true));

      u8 **stack_top = &stack[0];
      u8 **stack_bottom = &stack[0];

      EndBenchmark("Canny_init");

      BeginBenchmark("Canny_candidateLoop");

      /* sector numbers
      *  (Top-Left Origin)
      *
      *   1   2   3
      *    *  *  *
      *     * * *
      *   0*******0
      *     * * *
      *    *  *  *
      *   3   2   1
      */

#define CANNY_PUSH(d)    *(d) = u8(2), *stack_top++ = (d)
#define CANNY_POP(d)     (d) = *--stack_top

      // calculate magnitude and angle of gradient, perform non-maxima supression.
      // fill the map with one of the following values:
      //   0 - the pixel might belong to an edge
      //   1 - the pixel can not belong to an edge
      //   2 - the pixel does belong to an edge
      for (int i = 0; i <= imageHeight; i++)
      {
        int* _norm = mag_buf[(i > 0) + 1] + 1;
        if (i < imageHeight)
        {
          short* _dx = dx.Pointer(i, 0);
          short* _dy = dy.Pointer(i, 0);

          for (int j = 0; j < imageWidth; j++)
            _norm[j] = int(_dx[j])*_dx[j] + int(_dy[j])*_dy[j];

          _norm[-1] = _norm[imageWidth] = 0;
        }
        else
          memset(_norm-1, 0, /* cn* */mapstep*sizeof(int));

        // at the very beginning we do not have a complete ring
        // buffer of 3 magnitude rows for non-maxima suppression
        if (i == 0)
          continue;

        u8* _map = map + mapstep*i + 1;
        _map[-1] = _map[imageWidth] = 1;

        int* _mag = mag_buf[1] + 1; // take the central row
        size_t magstep1 = mag_buf[2] - mag_buf[1];
        size_t magstep2 = mag_buf[0] - mag_buf[1];

        const short* _x = dx.Pointer(i-1, 0);
        const short* _y = dy.Pointer(i-1, 0);

        if ((stack_top - stack_bottom) + imageWidth > maxsize)
        {
          //int sz = (int)(stack_top - stack_bottom);
          //maxsize = maxsize * 3/2;
          //stack.resize(maxsize);
          //stack_bottom = &stack[0];
          //stack_top = stack_bottom + sz;

          AnkiError("CannyEdgeDetection", "Out of Canny stack space");
          return RESULT_FAIL_OUT_OF_MEMORY;
        }

        int prev_flag = 0;
        for (int j = 0; j < imageWidth; j++)
        {
#define CANNY_SHIFT 15
          const int TG22 = (int)(0.4142135623730950488016887242097*(1<<CANNY_SHIFT) + 0.5);

          int m = _mag[j];

          if (m > low_thresh)
          {
            int xs = _x[j];
            int ys = _y[j];
            int x = ABS(xs);
            int y = ABS(ys) << CANNY_SHIFT;

            int tg22x = x * TG22;

            if (y < tg22x)
            {
              if (m > _mag[j-1] && m >= _mag[j+1]) goto __ocv_canny_push;
            }
            else
            {
              int tg67x = tg22x + (x << (CANNY_SHIFT+1));
              if (y > tg67x)
              {
                if (m > _mag[j+magstep2] && m >= _mag[j+magstep1]) goto __ocv_canny_push;
              }
              else
              {
                int s = (xs ^ ys) < 0 ? -1 : 1;
                if (m > _mag[j+magstep2-s] && m > _mag[j+magstep1+s]) goto __ocv_canny_push;
              }
            }
          }
          prev_flag = 0;
          _map[j] = u8(1);
          continue;
__ocv_canny_push:
          if (!prev_flag && m > high_thresh && _map[j-mapstep] != 2)
          {
            CANNY_PUSH(_map + j);
            prev_flag = 1;
          }
          else
            _map[j] = 0;
        }

        // scroll the ring buffer
        _mag = mag_buf[0];
        mag_buf[0] = mag_buf[1];
        mag_buf[1] = mag_buf[2];
        mag_buf[2] = _mag;
      }

      EndBenchmark("Canny_candidateLoop");

      BeginBenchmark("Canny_hysteresisLoop");

      // now track the edges (hysteresis thresholding)
      while (stack_top > stack_bottom)
      {
        u8* m;
        if ((stack_top - stack_bottom) + 8 > maxsize)
        {
          //int sz = (int)(stack_top - stack_bottom);
          //maxsize = maxsize * 3/2;
          //stack.resize(maxsize);
          //stack_bottom = &stack[0];
          //stack_top = stack_bottom + sz;

          AnkiError("CannyEdgeDetection", "Out of Canny stack space");
          return RESULT_FAIL_OUT_OF_MEMORY;
        }

        CANNY_POP(m);

        if (!m[-1])         CANNY_PUSH(m - 1);
        if (!m[1])          CANNY_PUSH(m + 1);
        if (!m[-mapstep-1]) CANNY_PUSH(m - mapstep - 1);
        if (!m[-mapstep])   CANNY_PUSH(m - mapstep);
        if (!m[-mapstep+1]) CANNY_PUSH(m - mapstep + 1);
        if (!m[mapstep-1])  CANNY_PUSH(m + mapstep - 1);
        if (!m[mapstep])    CANNY_PUSH(m + mapstep);
        if (!m[mapstep+1])  CANNY_PUSH(m + mapstep + 1);
      }

      EndBenchmark("Canny_hysteresisLoop");

      BeginBenchmark("Canny_finalize");

      // the final pass, form the final image
      const u8* pmap = map + mapstep + 1;
      u8* pdst = dst.Pointer(0, 0);
      const s32 dstStride = dst.get_stride();
      for (int i = 0; i < imageHeight; i++, pmap += mapstep, pdst += dstStride)
      {
        for (int j = 0; j < imageWidth; j++)
          pdst[j] = (u8)-(pmap[j] >> 1);
      }

      EndBenchmark("Canny_finalize");

      return RESULT_OK;
    } // CannyEdgeDetection

    typedef struct CvWSNode
    {
      struct CvWSNode* next;
      int mask_ofs;
      int img_ofs;
    }
    CvWSNode;

    typedef struct CvWSQueue
    {
      CvWSNode* first;
      CvWSNode* last;
    }
    CvWSQueue;

    static CvWSNode*
      icvAllocWSNodes( MemoryStack &storage )
    {
      CvWSNode* n = 0;

      int i;
      //int count = (block_size - sizeof(CvMemBlock))/sizeof(*n) - 1;
      const int count = 100;

      n = (CvWSNode*) storage.Allocate(count*sizeof(*n) );

      for( i = 0; i < count-1; i++ ) {
        n[i].next = n + i + 1;
      }

      n[count-1].next = 0;

      return n;
    }

    Result cvWatershed(const Array<u8> &src, Array<s32> &dst, MemoryStack scratch)
    {
      const int IN_QUEUE = -2;
      const int WSHED = -1;
      const int NQ = 256;

      const s32 imageHeight = src.get_size(0);
      const s32 imageWidth = src.get_size(1);

      //CvMat sstub;
      //CvMat dstub;
      //CvSize size;
      CvWSNode* free_node = 0, *node;
      CvWSQueue q[NQ];
      int active_queue;
      int i, j;
      int db, dg, dr;
      //int* mask;
      //u8* img;
      int mstep, istep;
      int subs_tab[513];

      // MAX(a,b) = b + MAX(a-b,0)
#define ws_max(a,b) ((b) + subs_tab[(a)-(b)+NQ])
      // MIN(a,b) = a - MAX(a-b,0)
#define ws_min(a,b) ((a) - subs_tab[(a)-(b)+NQ])

#define ws_push(idx,mofs,iofs,storage)  \
      {                               \
      if( !free_node )            \
      free_node = icvAllocWSNodes( storage );\
      node = free_node;           \
      free_node = free_node->next;\
      node->next = 0;             \
      node->mask_ofs = mofs;      \
      node->img_ofs = iofs;       \
      if( q[idx].last )           \
      q[idx].last->next=node; \
        else                        \
        q[idx].first = node;    \
        q[idx].last = node;         \
      }

#define ws_pop(idx,mofs,iofs)   \
      {                               \
      node = q[idx].first;        \
      q[idx].first = node->next;  \
      if( !node->next )           \
      q[idx].last = 0;        \
      node->next = free_node;     \
      free_node = node;           \
      mofs = node->mask_ofs;      \
      iofs = node->img_ofs;       \
      }

#define c_diff(ptr1,ptr2,diff)      \
      {                                   \
      db = abs((ptr1)[0] - (ptr2)[0]);\
      dg = abs((ptr1)[1] - (ptr2)[1]);\
      dr = abs((ptr1)[2] - (ptr2)[2]);\
      diff = ws_max(db,dg);           \
      diff = ws_max(diff,dr);         \
      assert( 0 <= diff && diff <= 255 ); \
      }

      AnkiConditionalErrorAndReturnValue(src.IsValid() && dst.IsValid() && scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "CannyEdgeDetection", "Invalid objects");

      //AnkiConditionalErrorAndReturnValue(AreEqualSize(src, dst),
      AnkiConditionalErrorAndReturnValue(src.get_size(0) == dst.get_size(0) && src.get_size(1) == 3*dst.get_size(1),
        RESULT_FAIL_INVALID_SIZE, "CannyEdgeDetection", "src and dst must be the same size");

      //size = cvGetMatSize(src);
      //storage = cvCreateMemStorage();

      //img = src.data.ptr;
      //mask = dst.data.i;

      //istep = src.step;
      //mstep = dst.step / sizeof(mask[0]);
      istep = src.get_stride();
      mstep = dst.get_stride() / sizeof(s32);

      memset( q, 0, NQ*sizeof(q[0]) );

      for( i = 0; i < 256; i++ )
        subs_tab[i] = 0;
      for( i = 256; i <= 512; i++ )
        subs_tab[i] = i - 256;

      // draw a pixel-wide border of dummy "watershed" (i.e. boundary) pixels
      //for( j = 0; j < imageWidth; j++ )
      //  mask[j] = mask[j + mstep*(imageHeight-1)] = WSHED;
      dst(0,0,0,-1).Set(WSHED);
      dst(-1,-1,0,-1).Set(WSHED);
      dst(0,-1,0,0).Set(WSHED);
      dst(0,-1,-1,-1).Set(WSHED);

      // initial phase: put all the neighbor pixels of each marker to the ordered queue -
      // determine the initial boundaries of the basins
      for( i = 1; i < imageHeight-1; i++ )
      {
        //img += istep;
        //mask += mstep;
        const u8 * restrict pSrc = src.Pointer(i, 0);
        s32 * restrict pDst = dst.Pointer(i, 0);

        pDst[0] = pDst[imageWidth-1] = WSHED;

        for( j = 1; j < imageWidth-1; j++ )
        {
          int* m = pDst + j;
          if( m[0] < 0 ) m[0] = 0;
          if( m[0] == 0 && (m[-1] > 0 || m[1] > 0 || m[-mstep] > 0 || m[mstep] > 0) )
          {
            const u8* ptr = pSrc + j*3;
            int idx = 256, t;
            if( m[-1] > 0 )
              c_diff( ptr, ptr - 3, idx );
            if( m[1] > 0 )
            {
              c_diff( ptr, ptr + 3, t );
              idx = ws_min( idx, t );
            }
            if( m[-mstep] > 0 )
            {
              c_diff( ptr, ptr - istep, t );
              idx = ws_min( idx, t );
            }
            if( m[mstep] > 0 )
            {
              c_diff( ptr, ptr + istep, t );
              idx = ws_min( idx, t );
            }
            AnkiAssert( 0 <= idx && idx <= 255 );
            ws_push( idx, i*mstep + j, i*istep + j*3, scratch );
            m[0] = IN_QUEUE;
          }
        }
      }

      // find the first non-empty queue
      for( i = 0; i < NQ; i++ )
        if( q[i].first )
          break;

      // if there is no markers, exit immediately
      if( i == NQ )
        return RESULT_OK;

      active_queue = i;

      //img = src.data.ptr;
      //mask = dst.data.i;
      const u8 * restrict pSrc = src.Pointer(0, 0);
      s32 * restrict pDst = dst.Pointer(0, 0);

      // recursively fill the basins
      for(;;)
      {
        int mofs, iofs;
        int lab = 0, t;
        int* m;
        const u8* ptr;

        if( q[active_queue].first == 0 )
        {
          for( i = active_queue+1; i < NQ; i++ )
            if( q[i].first )
              break;
          if( i == NQ )
            break;
          active_queue = i;
        }

        ws_pop( active_queue, mofs, iofs );

        m = pDst + mofs;
        ptr = pSrc + iofs;
        t = m[-1];
        if( t > 0 ) lab = t;
        t = m[1];
        if( t > 0 )
        {
          if( lab == 0 ) lab = t;
          else if( t != lab ) lab = WSHED;
        }
        t = m[-mstep];
        if( t > 0 )
        {
          if( lab == 0 ) lab = t;
          else if( t != lab ) lab = WSHED;
        }
        t = m[mstep];
        if( t > 0 )
        {
          if( lab == 0 ) lab = t;
          else if( t != lab ) lab = WSHED;
        }
        assert( lab != 0 );
        m[0] = lab;
        if( lab == WSHED )
          continue;

        if( m[-1] == 0 )
        {
          c_diff( ptr, ptr - 3, t );
          ws_push( t, mofs - 1, iofs - 3, scratch );
          active_queue = ws_min( active_queue, t );
          m[-1] = IN_QUEUE;
        }
        if( m[1] == 0 )
        {
          c_diff( ptr, ptr + 3, t );
          ws_push( t, mofs + 1, iofs + 3, scratch );
          active_queue = ws_min( active_queue, t );
          m[1] = IN_QUEUE;
        }
        if( m[-mstep] == 0 )
        {
          c_diff( ptr, ptr - istep, t );
          ws_push( t, mofs - mstep, iofs - istep, scratch );
          active_queue = ws_min( active_queue, t );
          m[-mstep] = IN_QUEUE;
        }
        if( m[mstep] == 0 )
        {
          c_diff( ptr, ptr + istep, t );
          ws_push( t, mofs + mstep, iofs + istep, scratch );
          active_queue = ws_min( active_queue, t );
          m[mstep] = IN_QUEUE;
        }
      }

      return RESULT_OK;
    } // cvWatershed
  } // namespace Embedded
} // namespace Anki
