/**
 * File: quarterBGGR10ToRGB.cpp
 *
 * Author:  Al Chaussee
 * Created: 10/4/2018
 *
 * Description: BGGR10 bayer to quarter sized RGB
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

void QuarterBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                        ImageRGB& rgb)
{
#ifdef __ARM_NEON__
  // Output is quarter the size of input
  rgb.Allocate(rows/4, cols/4);

  // Raw Bayer format from camera is 4 10 bit pixels packed into 5 bytes
  // so convert cols to bayer cols
  cols = (cols*5)/4;
  
  const u8* bayer1 = bayer;
  const u8* bayer2 = bayer + cols;
  
  u8* rgbPtr = reinterpret_cast<u8*>(rgb.GetRow(0));

  // Every other pair of rows will be processed
  const s32 kNumRowsToProcess = 4;
  for(int i = 0; i < rows-1; i += kNumRowsToProcess)
  {
    bayer1 = bayer + (i*cols);
    bayer2 = bayer + ((i+1)*cols);
    rgbPtr = reinterpret_cast<u8*>(rgb.GetRow(i/kNumRowsToProcess));

    // Process 20 bytes of bayer data at a time which will result in 4 RGB pixels
    // We only need to look at a quarter of the bayer data so we can skip
    // every other 2x2 chunk of data which also includes the 5th byte of packed LSBs
    // For the first row the data we will load looks like
    // ([r g] r g p [r g] r) g p ([r g] r g p [r g] r) g p
    // We need to process the bytes in [ ] and so will be loading everything in ( )
    // 2 bytes can be skipped each time, the second g p
    // The goal will be to make all the [r g] blocks sequential in order to unzip them
    // to get all red and all greens in separate registers
    // This will be done with a series of bit shifts and bit selects
    const s32 kNumElementsToProcess = 20;
    s32 j = 0;
    for(; j < (cols-1)-(kNumElementsToProcess-1); j += kNumElementsToProcess)
    {
      uint8x8_t row1_1 = vld1_u8(bayer1);
      bayer1 += 10;
      uint8x8_t row1_2 = vld1_u8(bayer1);
      bayer1 += 10;

      uint8x8_t row2_1 = vld1_u8(bayer2);
      bayer2 += 10;
      uint8x8_t row2_2 = vld1_u8(bayer2);
      bayer2 += 10;

      // Use a series of bit masks to pull out the [r g] chunks of data
      uint8x8_t mask1 = vcreate_u8(0xFFFF000000000000);
      uint8x8_t mask2 = vcreate_u8(0xFFFFFFFF00000000);
      uint8x8_t mask3 = vcreate_u8(0xFFFFFFFFFFFF0000);

      // Left shift the entire register 24 bits and bit select it with original row data
      uint8x8_t temp = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(row1_2), 24));
      row1_2 = vbsl_u8(mask1, row1_2, temp);

      // Similar shift and select to slowly build up sequential [r g] chunks
      temp = vreinterpret_u8_u64(vshr_n_u64(vreinterpret_u64_u8(row1_1), 32));
      row1_2 = vbsl_u8(mask2, row1_2, temp);

      temp = vreinterpret_u8_u64(vshr_n_u64(vreinterpret_u64_u8(row1_1), 8));
      row1_2 = vbsl_u8(mask3, row1_2, temp);

      // Repeat with second row, this one has [g b] chunks of data
      temp = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(row2_2), 24));
      row2_2 = vbsl_u8(mask1, row2_2, temp);

      temp = vreinterpret_u8_u64(vshr_n_u64(vreinterpret_u64_u8(row2_1), 32));
      row2_2 = vbsl_u8(mask2, row2_2, temp);

      temp = vreinterpret_u8_u64(vshr_n_u64(vreinterpret_u64_u8(row2_1), 8));
      row2_2 = vbsl_u8(mask3, row2_2, temp);

      // Saturating left shift by 2 as if the packed 5th byte was all 0
      row1_2 = vqshl_n_u8(row1_2, 2);
      row2_2 = vqshl_n_u8(row2_2, 2);

      // Unzip alternating [r g] and [g b] to get all
      // r, g, b in separate registers
      uint8x8x2_t rg = vuzp_u8(row1_2, row1_2);
      uint8x8x2_t gb = vuzp_u8(row2_2, row2_2);

      // Halving add the greens to average
      rg.val[0] = vhadd_u8(rg.val[0], gb.val[1]);

      uint8x8x3_t rgbData;
      // Combine r, g, b registers together and do an interleaving
      // store into rgb image output
      rgbData.val[0] = rg.val[1];
      rgbData.val[1] = rg.val[0];
      rgbData.val[2] = gb.val[0];
      vst3_u8(rgbPtr, rgbData);
      // From 20 bytes of bayer data we got 4 pixels
      rgbPtr += 4*3;
    }

    // Process left over data
    for(; j < cols; j+=5)
    {
      rgbPtr[0] = cv::saturate_cast<u8>(((u16)bayer1[0]) << 2);
      rgbPtr[1] = cv::saturate_cast<u8>((((u16)bayer1[1] + (u16)bayer2[0])) << 1);
      rgbPtr[2] = cv::saturate_cast<u8>(((u16)bayer2[1]) << 2);
      rgbPtr += 3;
      bayer1 += 5;
      bayer2 += 5;
    }
  }
#else
  // No neon available so halve to RGB and then resize to get quarter sized output
  // HalveBGGR10ToRGB has a non-neon implementation
  HalveBGGR10ToRGB(bayer, rows, cols, rgb);
  rgb.Resize(0.5f);
#endif
}
  
}
}
}
