/**
 * File: demosaicBGGR10ToRGB.cpp
 *
 * Author:  Al Chaussee
 * Created: 10/4/2018
 *
 * Description: Demosaic BGGR10 bayer data to RGB
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/vision/engine/imageBuffer/conversions/imageConversions.h"

#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/neonMacros.h"

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

#include <unistd.h>

namespace Anki {
namespace Vision {
namespace ImageConversions {

void DemosaicBGGR10ToRGB(const u8* bayer_in, s32 rows, s32 cols, ImageRGB& rgb)
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
  // Adopted from code in HalveBGGR10ToRGB
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
}
