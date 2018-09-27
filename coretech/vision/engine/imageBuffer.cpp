/**
 * File: imageBuffer.h
 *
 * Author:  Al Chaussee
 * Created: 09/19/2018
 *
 * Description: Wrapper around raw image data
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/vision/engine/imageBuffer.h"

#include "coretech/vision/engine/debayer.h"
#include "coretech/vision/engine/image_impl.h"

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

namespace Anki {
namespace Vision {

ImageBuffer::ImageBuffer(u8* data, s32 numRows, s32 numCols, ImageEncoding format, TimeStamp_t timestamp, s32 id)
: _rawData(data)
, _rawNumRows(numRows)
, _rawNumCols(numCols)
, _format(format)
, _imageId(id)
, _timestamp(timestamp)
{

}

void ImageBuffer::GetNumRowsCols(s32& rows, s32& cols) const
{
  switch(_format)
  {
    case ImageEncoding::BAYER:
      {
        if(_downsampleIfBayer)
        {
          // Downsampled bayer is half resolution of
          // original image
          rows = _rawNumRows/2;
          cols = _rawNumCols/2;
        }
        else
        {
          rows = _rawNumRows;
          cols = _rawNumCols;
        }
      }
      break;
    case ImageEncoding::RawRGB:
      {
        rows = _rawNumRows;
        cols = _rawNumCols;
      }
      break;
    case ImageEncoding::YUV420sp:
      {
        // YUV420sp data is has extra rows for UV
        rows = (_rawNumRows * (2.f/3.f));
        cols = _rawNumCols;
      }
      break;
    default:
      {
        DEV_ASSERT_MSG(false,
                       "ImageBuffer.GetNumRowsCols.UnsupportedFormat",
                       "%s",
                       EnumToString(_format));
      }
      break;
  }
}
  
s32 ImageBuffer::GetNumCols() const
{
  s32 rows = 0;
  s32 cols = 0;
  GetNumRowsCols(rows, cols);
  return cols;
}

s32 ImageBuffer::GetNumRows() const
{
  s32 rows = 0;
  s32 cols = 0;
  GetNumRowsCols(rows, cols);
  return rows;
}

f32 ImageBuffer::GetScaleFactorFromSensorRes() const
{
  // If we are downsampling the image then our "Sensor" resolution and
  // "Full" resolution will be the same otherwise "Sensor" resolution will
  // be actual sensor resolution and "Full" will be half that.
  // Or if the image is already RGB (sim)
  // TODO: VIC-7267 will likely remove the need for this
  return (((_format == ImageEncoding::BAYER && _downsampleIfBayer) ||
           _format == ImageEncoding::RawRGB) ?
          1.f : 0.5f);
}

bool ImageBuffer::GetRGB(ImageRGB& rgb) const
{
  DEV_ASSERT(_rawData != nullptr, "ImageBuffer.GetRGB.NullData");
  
  s32 rows = 0;
  s32 cols = 0;
  GetNumRowsCols(rows, cols);
  switch(_format)
  {
    case ImageEncoding::BAYER:
      {
        if(_downsampleIfBayer)
        {
          rgb.Allocate(rows, cols);

          DownsampleBGGR10ToRGB(_rawData,
                                _rawNumCols,
                                _rawNumRows,
                                rgb);
        }
        else
        {
          rgb.Allocate(rows, cols);

          DemosaicBGGR10ToRGB(_rawData,
                              _rawNumRows,
                              _rawNumCols,
                              rgb);
        }
      }
      break;
    case ImageEncoding::RawRGB:
      {
        rgb = ImageRGB(rows, cols, _rawData);
      }
      break;
    case ImageEncoding::YUV420sp:
      {
        rgb.Allocate(rows, cols);
        ConvertYUV420spToRGB(_rawData,
                             rows,
                             cols,
                             rgb);
      }
      break;
    default:
      {
        DEV_ASSERT_MSG(false,
                       "ImageBuffer.GetRGB.UnsupportedFormat",
                       "%s", EnumToString(_format));
        return false;
      }
      break;
  }

  rgb.SetTimestamp(_timestamp);
  rgb.SetImageId(_imageId);

  return true;
}

void ImageBuffer::ConvertYUV420spToRGB(const u8* yuv, s32 rows, s32 cols,
                                       ImageRGB& rgb)
{
  // Expecting even number of rows and colums for 2x2 subsampled YUV data
  DEV_ASSERT((rows % 2 == 0) && (cols % 2 == 0),
             "Vision.ConvertYUVtoRGB.OddNumRowsOrCols");

  rgb.Allocate(rows, cols);

#ifdef __ARM_NEON__

  // Neon implementation of opencv's YUV420sp to RGB algorithm
  // R = 1.164(Y - 16) + 1.596(V - 128)
  // G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
  // B = 1.164(Y - 16)                  + 2.018(U - 128)

  // Set up constants
  const float32x4_t kRVCoeff = vdupq_n_f32(1.596);
  const float32x4_t kGUCoeff = vdupq_n_f32(0.391);
  const float32x4_t kGVCoeff = vdupq_n_f32(0.813);
  const float32x4_t kBUCoeff = vdupq_n_f32(2.018);
  const float32x4_t kYCoeff  = vdupq_n_f32(1.164);
  const uint8x8_t   k128     = vdup_n_u8(128);
  const uint8x8_t   k16      = vdup_n_u8(16);

#endif

  // Set up input and output pointers
  const u8* yPtr = yuv;
  const u8* y2Ptr = yuv + cols;
  const u8* uvPtr = yuv + (rows*cols);
  u8* outputPtr1 = nullptr;
  u8* outputPtr2 = nullptr;

  u32 r = 0;
  for(; r < rows; r += 2)
  {
    outputPtr1 = reinterpret_cast<u8*>(rgb.GetRow(r));
    outputPtr2 = reinterpret_cast<u8*>(rgb.GetRow(r+1));

    s32 c = 0;

#ifdef __ARM_NEON__
    for(; c < cols - (8-1); c += 8)
    {
      // Load 2 rows of Y since each UV corresponds to 2x2 blocks of Y
      uint8x8_t y1 = vld1_u8(yPtr);
      yPtr += 8;
      uint8x8_t y2 = vld1_u8(y2Ptr);
      y2Ptr += 8;
      uint8x8x2_t uv = vld2_u8(uvPtr);
      uvPtr += 8; // Have to load 8 UV pairs but will only use 4
      uint8x8x3_t rgb1, rgb2;

      // Unsigned long subract Y - 16
      // Since Y is u8, if this underflows the underflow bit (9th bit) will
      // be preserved in the conversion to u16. This is neccessary when we reinterpret
      // from u16 to s16 so we end up with the correct value from Y - 16
      uint16x8_t tmp = vsubl_u8(y1, k16);
      int16x8_t ui16x8 = vreinterpretq_s16_u16(tmp);

      // Expand to float
      int16x4_t u16x4_1 = vget_low_s16(ui16x8);
      int16x4_t u16x4_2 = vget_high_s16(ui16x8);

      int32x4_t u32x4_1 = vmovl_s16(u16x4_1);
      int32x4_t u32x4_2 = vmovl_s16(u16x4_2);

      float32x4_t y1_1 = vcvtq_f32_s32(u32x4_1);
      float32x4_t y1_2 = vcvtq_f32_s32(u32x4_2);

      // Multiply (Y - 16) by YCoeff
      y1_1 = vmulq_f32(y1_1, kYCoeff);
      y1_2 = vmulq_f32(y1_2, kYCoeff);

      // Repeat for second row of Y
      tmp = vsubl_u8(y2, k16);
      ui16x8 = vreinterpretq_s16_u16(tmp);

      u16x4_1 = vget_low_s16(ui16x8);
      u16x4_2 = vget_high_s16(ui16x8);

      u32x4_1 = vmovl_s16(u16x4_1);
      u32x4_2 = vmovl_s16(u16x4_2);

      float32x4_t y2_1 = vcvtq_f32_s32(u32x4_1);
      float32x4_t y2_2 = vcvtq_f32_s32(u32x4_2);

      y2_1 = vmulq_f32(y2_1, kYCoeff);
      y2_2 = vmulq_f32(y2_2, kYCoeff);

      // We only need 4 UV pairs for 8 Y (have to load 8 pairs though) and each UV will operate
      // on 2 consecutive Ys (2x2 subsampling) so we want the U and V data to look like
      // 1 1 2 2 3 3 4 4

      // Expand and convert U from u8 to f32

      // Use same subtraction trick for U - 128
      tmp = vsubl_u8(uv.val[0], k128);
      ui16x8 = vreinterpretq_s16_u16(tmp);

      // Grab the lower 4 U and zip them with themselves to produce a vector
      // of 1 1 2 2 3 3 4 4
      int16x4_t u16x4 = vget_low_s16(ui16x8);
      int16x4x2_t u16x4x2 = vzip_s16(u16x4, u16x4); 

      // Expand the 1 1 2 2 3 3 4 4 vector and convert to f32
      u32x4_1 = vmovl_s16(u16x4x2.val[0]);
      u32x4_2 = vmovl_s16(u16x4x2.val[1]);

      float32x4_t uf_1 = vcvtq_f32_s32(u32x4_1);
      float32x4_t uf_2 = vcvtq_f32_s32(u32x4_2);

      // Do same things for Y
      // Expand and convert V from u8 to f32

      // Convert u8 to s16
      tmp = vsubl_u8(uv.val[1], k128);
      int16x8_t vi16x8 = vreinterpretq_s16_u16(tmp);

      // Grab the lower 4 V and zip them with themselves to produce a vector
      // of 1 1 2 2 3 3 4 4
      int16x4_t v16x4 = vget_low_s16(vi16x8);
      int16x4x2_t v16x4x2 = vzip_s16(v16x4, v16x4); 

      // Expand the 1 1 2 2 3 3 4 4 vector and convert to f32
      int32x4_t v32x4_1 = vmovl_s16(v16x4x2.val[0]);
      int32x4_t v32x4_2 = vmovl_s16(v16x4x2.val[1]);

      float32x4_t vf_1 = vcvtq_f32_s32(v32x4_1);
      float32x4_t vf_2 = vcvtq_f32_s32(v32x4_2);

      // Calculate R

      // Multiply V by RCoeff
      float32x4_t rf_1 = vmulq_f32(kRVCoeff, vf_1);
      float32x4_t rf_2 = vmulq_f32(kRVCoeff, vf_2);

      // Calculate R value for first row of Y
      float32x4_t r1_1 = vaddq_f32(y1_1, rf_1);
      float32x4_t r1_2 = vaddq_f32(y1_2, rf_2);

      // Saturate convert R from f32 to u8
      uint32x4_t r32x4_1 = vcvtq_u32_f32(r1_1);
      uint32x4_t r32x4_2 = vcvtq_u32_f32(r1_2);

      uint16x4_t r16x4_1 = vqmovn_u32(r32x4_1);
      uint16x4_t r16x4_2 = vqmovn_u32(r32x4_2);

      uint16x8_t r16x8 = vcombine_u16(r16x4_1, r16x4_2);

      rgb1.val[0] = vqmovn_u16(r16x8);

      // Repeat for second row of Y

      float32x4_t r2_1 = vaddq_f32(y2_1, rf_1);
      float32x4_t r2_2 = vaddq_f32(y2_2, rf_2);

      // Saturate convert R from f32 to u8
      r32x4_1 = vcvtq_u32_f32(r2_1);
      r32x4_2 = vcvtq_u32_f32(r2_2);

      r16x4_1 = vqmovn_u32(r32x4_1);
      r16x4_2 = vqmovn_u32(r32x4_2);

      r16x8 = vcombine_u16(r16x4_1, r16x4_2);

      rgb2.val[0] = vqmovn_u16(r16x8);

      // Calculate G

      // Multiply U by GUCoeff
      float32x4_t guf_1 = vmulq_f32(kGUCoeff, uf_1);
      float32x4_t guf_2 = vmulq_f32(kGUCoeff, uf_2);

      // Multiply V by GVCoeff
      float32x4_t gvf_1 = vmulq_f32(kGVCoeff, vf_1);
      float32x4_t gvf_2 = vmulq_f32(kGVCoeff, vf_2);

      // Calculate G value for first row of Y
      float32x4_t g1_1 = vsubq_f32(y1_1, gvf_1);
      g1_1 = vsubq_f32(g1_1, guf_1);
      float32x4_t g1_2 = vsubq_f32(y1_2, gvf_2);
      g1_2 = vsubq_f32(g1_2, guf_2);

      uint32x4_t g32x4_1 = vcvtq_u32_f32(g1_1);
      uint32x4_t g32x4_2 = vcvtq_u32_f32(g1_2);

      uint16x4_t g16x4_1 = vqmovn_u32(g32x4_1);
      uint16x4_t g16x4_2 = vqmovn_u32(g32x4_2);

      uint16x8_t g16x8 = vcombine_u16(g16x4_1, g16x4_2);

      rgb1.val[1] = vqmovn_u16(g16x8);

      // Repeat for second row of Y

      float32x4_t g2_1 = vsubq_f32(y2_1, gvf_1);
      g2_1 = vsubq_f32(g2_1, guf_1);
      float32x4_t g2_2 = vsubq_f32(y2_2, gvf_2);
      g2_2 = vsubq_f32(g2_2, guf_2);

      g32x4_1 = vcvtq_u32_f32(g2_1);
      g32x4_2 = vcvtq_u32_f32(g2_2);

      g16x4_1 = vqmovn_u32(g32x4_1);
      g16x4_2 = vqmovn_u32(g32x4_2);

      g16x8 = vcombine_u16(g16x4_1, g16x4_2);

      rgb2.val[1] = vqmovn_u16(g16x8);

      // Calculate B

      // Multiply U by BCoeff
      float32x4_t bf_1 = vmulq_f32(kBUCoeff, uf_1);
      float32x4_t bf_2 = vmulq_f32(kBUCoeff, uf_2);

      // Calculate B value for first row of Y
      float32x4_t b1_1 = vaddq_f32(y1_1, bf_1);
      float32x4_t b1_2 = vaddq_f32(y1_2, bf_2);

      uint32x4_t b32x4_1 = vcvtq_u32_f32(b1_1);
      uint32x4_t b32x4_2 = vcvtq_u32_f32(b1_2);

      uint16x4_t b16x4_1 = vqmovn_u32(b32x4_1);
      uint16x4_t b16x4_2 = vqmovn_u32(b32x4_2);

      uint16x8_t b16x8 = vcombine_u16(b16x4_1, b16x4_2);

      rgb1.val[2] = vqmovn_u16(b16x8);

      // Repeat for second for of Y

      float32x4_t b2_1 = vaddq_f32(y2_1, bf_1);
      float32x4_t b2_2 = vaddq_f32(y2_2, bf_2);

      b32x4_1 = vcvtq_u32_f32(b2_1);
      b32x4_2 = vcvtq_u32_f32(b2_2);

      b16x4_1 = vqmovn_u32(b32x4_1);
      b16x4_2 = vqmovn_u32(b32x4_2);

      b16x8 = vcombine_u16(b16x4_1, b16x4_2);

      rgb2.val[2] = vqmovn_u16(b16x8);

      // Store both rows of rgb values into output
      vst3_u8(outputPtr1, rgb1);
      outputPtr1 += 24;

      vst3_u8(outputPtr2, rgb2);
      outputPtr2 += 24;
    }

#endif

    // Process any extra elements one by one
    // Copied from OpenCV
    // https://github.com/opencv/opencv/blob/7dc88f26f24fa3fd564a282b2438c3ac0263cd2f/modules/imgproc/src/color_yuv.cpp
    constexpr int ITUR_BT_601_CY    = 1220542;
    constexpr int ITUR_BT_601_CUB   = 2116026;
    constexpr int ITUR_BT_601_CUG   = -409993;
    constexpr int ITUR_BT_601_CVG   = -852492;
    constexpr int ITUR_BT_601_CVR   = 1673527;
    constexpr int ITUR_BT_601_SHIFT = 20;

    for(; c < cols; c += 2)
    {
      int u = (int)(uvPtr[0]) - 128;
      int v = (int)(uvPtr[1]) - 128;

      int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
      int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
      int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

      int y00 = std::max(0, int(yPtr[0]) - 16) * ITUR_BT_601_CY;
      outputPtr1[0] = cv::saturate_cast<uchar>((y00 + ruv) >> ITUR_BT_601_SHIFT);
      outputPtr1[1] = cv::saturate_cast<uchar>((y00 + guv) >> ITUR_BT_601_SHIFT);
      outputPtr1[2] = cv::saturate_cast<uchar>((y00 + buv) >> ITUR_BT_601_SHIFT);

      int y01 = std::max(0, int(yPtr[1]) - 16) * ITUR_BT_601_CY;
      outputPtr1[3] = cv::saturate_cast<uchar>((y01 + ruv) >> ITUR_BT_601_SHIFT);
      outputPtr1[4] = cv::saturate_cast<uchar>((y01 + guv) >> ITUR_BT_601_SHIFT);
      outputPtr1[5] = cv::saturate_cast<uchar>((y01 + buv) >> ITUR_BT_601_SHIFT);

      int y10 = std::max(0, int(y2Ptr[0]) - 16) * ITUR_BT_601_CY;
      outputPtr2[0] = cv::saturate_cast<uchar>((y10 + ruv) >> ITUR_BT_601_SHIFT);
      outputPtr2[1] = cv::saturate_cast<uchar>((y10 + guv) >> ITUR_BT_601_SHIFT);
      outputPtr2[2] = cv::saturate_cast<uchar>((y10 + buv) >> ITUR_BT_601_SHIFT);

      int y11 = std::max(0, int(y2Ptr[1]) - 16) * ITUR_BT_601_CY;
      outputPtr2[3] = cv::saturate_cast<uchar>((y11 + ruv) >> ITUR_BT_601_SHIFT);
      outputPtr2[4] = cv::saturate_cast<uchar>((y11 + guv) >> ITUR_BT_601_SHIFT);
      outputPtr2[5] = cv::saturate_cast<uchar>((y11 + buv) >> ITUR_BT_601_SHIFT);

      yPtr  += 2;
      y2Ptr += 2;
      uvPtr += 2;
      outputPtr1 += 6;
      outputPtr2 += 6;
    }

    // Finished a row so increment pointers
    yPtr += cols;
    y2Ptr += cols;
  }
}  

void ImageBuffer::DownsampleBGGR10ToRGB(const u8* bayer_in, s32 bayer_sx, s32 bayer_sy, ImageRGB& rgb)
{
  // Output image is half the resolution of the bayer image
  rgb.Allocate(bayer_sy / 2, bayer_sx / 2);

  // Multiply bayer_sx by (10/8) as bayer_sx is the resolution of the bayer image
  // but this functions expects the width and height in bytes
  bayer_mipi_bggr10_downsample(bayer_in, reinterpret_cast<u8*>(rgb.GetRow(0)),
                               (bayer_sx*10)/8, bayer_sy, 10);
}

void ImageBuffer::DemosaicBGGR10ToRGB(const u8* bayer_in, s32 rows, s32 cols, ImageRGB& rgb)
{
#ifdef __ARM_NEON__
  // Create a mat to hold the bayer image
  cv::Mat bayer(rows,
                cols,
                CV_8UC1);

  const u8* bufferPtr = bayer_in;
  // Raw Bayer format from camera is 4 10 bit pixels packed into 5 bytes
  // The fifth byte is the 2 lsb from each of the 4 pixels
  // This loop strips the fifth byte and does a saturating shift on
  // the other 4 bytes as if the fifth byte was all 0
  // Adopted from code in DownsampleBGGR10ToRGB
  u8* bayerPtr = static_cast<u8*>(bayer.ptr());
  for(int i = 0; i < (cols*rows); i+=8)
  {
    uint8x8_t data = vld1_u8(bufferPtr);
    bufferPtr += 5;
    uint8x8_t data2 = vld1_u8(bufferPtr);
    bufferPtr += 5;
    data = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(data), 32));
    data = vext_u8(data, data2, 4);
    data = vqshl_n_u8(data, 2);
    vst1_u8(bayerPtr, data);
    bayerPtr += 8;
  }

  // Use opencv to convert bayer BGGR to RGB
  // The RG in BayerRG come from the second row, second and third columns
  // of the bayer image
  // B  G   B  G
  // G [R] [G] R
  // And as usual opencv has bugs in their image conversions which
  // result in swapped R and B channels in the output image compared to
  // what the conversion enum says
  cv::cvtColor(bayer, rgb.get_CvMat_(), cv::COLOR_BayerRG2BGR);
#else
  // Create a mat to hold the bayer image
  cv::Mat bayer(rows,
                cols,
                CV_8UC1);

  const u8* bufferPtr = bayer_in;
  // Raw Bayer format from camera is 4 10 bit pixels packed into 5 bytes
  // The fifth byte is the 2 lsb from each of the 4 pixels
  // This loop strips the fifth byte and does a saturating shift on
  // the other 4 bytes as if the fifth byte was all 0
  u8* bayerPtr = static_cast<u8*>(bayer.ptr());
  for(int i = 0; i < (cols*rows); i+=4)
  {
    u8 a = cv::saturate_cast<u8>(((u16)bufferPtr[0]) << 2);
    u8 b = cv::saturate_cast<u8>(((u16)bufferPtr[1]) << 2);
    u8 c = cv::saturate_cast<u8>(((u16)bufferPtr[2]) << 2);
    u8 d = cv::saturate_cast<u8>(((u16)bufferPtr[3]) << 2);
    bufferPtr += 5;
            
    bayerPtr[0] = a;
    bayerPtr[1] = b;
    bayerPtr[2] = c;
    bayerPtr[3] = d;
    bayerPtr += 4;
  }

  // Use opencv to convert bayer BGGR to RGB
  // The RG in BayerRG come from the second row, second and third columns
  // of the bayer image
  // B  G   B  G
  // G [R] [G] R
  cv::cvtColor(bayer, rgb.get_CvMat_(), cv::COLOR_BayerRG2RGB);
#endif
}
  
}
}
