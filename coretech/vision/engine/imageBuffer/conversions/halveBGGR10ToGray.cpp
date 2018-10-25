/**
 * File: halveBGGR10ToGray.cpp
 *
 * Author:  Al Chaussee
 * Created: 10/4/2018
 *
 * Description: Halves BGGR10 bayer to Gray
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

void HalveBGGR10ToGray(const u8* bayer, s32 rows, s32 cols,
                       Image& gray)
{
  // Whether or not to average the 2 green values in each 2x2 block of bayer data
  // If 0, only use green values from every other row of bayer data
#define AVERAGE_GREEN 0

// Outer most ifdef for whether or not neon is supported
#ifdef __ARM_NEON__
  // Output is half the input size
  gray.Allocate(rows/2, cols/2);

  // Raw Bayer format from camera is 4 10 bit pixels packed into 5 bytes
  // so convert cols to bayer cols
  cols = (cols*5)/4;

  // If we aren't averaging greens then we only need to looks at every other row
  const u8* bayer1 = bayer;
  
#if AVERAGE_GREEN
  // Otherwise we will look at pairs of rows
  const u8* bayer2 = bayer + cols;
#endif
  
  u8* grayPtr = gray.GetRawDataPointer();

  // The idea for this conversion is that we just want the green bytes from the bayer
  // data. We need to make all the green bytes sequential so we can write them into the output
  // image. This will be accomplished with a series of bit shifts that operate on varying chunks
  // of data. We can reinterpret the input data as a varying number of elements of different data
  // types and shift each element individually. We will be loading 16 bytes at a time but will
  // only be operating on 15 of this bytes. This is due to the 5 byte packing of bayer data
  //        r g r g p r g r  g p  r  g  r  g  p  r
  // Input: 1 2 3 4 5 6 7 8  9 10 11 12 13 14 15 16  ->  Output: 2 4 7 9 12 14 _ _ (all the greens)
  // Steps: [1 2] [3 4] [5 6] [7 8] [9 10] [11 12] [13 14] [15 16] (treat u8x16 data as u16x8
  //          0    <8     0     8>    8>     <8      <8       0
  //         [1 2 4 0] [5 6 0 7] [0 9 12 0] [14 0 15 16] (treat as u32x4)
  //             8>       <24        8>          0
  //         [0 1 2 4 7 0 0 0] [0 0 9 12 14 0 15 16] (treat as u64x2)
  //                <16                 8>
  //          [2 4 7 0 0 0 0 0] [0 0 0 9 12 14 0 15]
  //                  Bitwise OR these two
  //                  [2 4 7 9 12 14 0 15] (treat as u8x8)
  // From 16 bytes of input we got 6 bytes of output in sequential order to be written to output
  // A very similar method with different shifts can be used on the gb rows
  
  // Construct all the shifts. Endianness, neon register ordering, and vcombine all
  // seem to play a factor into how the shifts need to be constructed
  // This one ends up being the 0, <8, 0, 8>, 8>, <8, <8, 0 shifts
  // It appears that the individual vcreates need to be mirrored element-wise
  // and that negative and positive are reversed (negative values are supposed to shift right)
  // So while this looks like it would create 8, 0, -8, 0, 0, -8, -8, 8 it ends up creating
  // 0, 8, 0, -8, 8, -8, -8, 0
  int16x8_t shift1_1 = vcombine_s16(vcreate_s16(0x00080000FFF80000),
                                    vcreate_s16(0x0000FFF8FFF80008));

  int32x4_t shift2_1 = vcombine_s32(vcreate_s32(0xFFFFFFE800000008),
                                    vcreate_s32(0x0000000000000008));

  int64x2_t shift3_1 = vcombine_s64(vcreate_s64(0xFFFFFFFFFFFFFFF0),
                                    vcreate_s64(0x0000000000000008));

#if AVERAGE_GREEN
  // Can use a similar set of shifts for the gb rows
  int16x8_t shift1_2 = vcombine_s16(vcreate_s16(0xFFF8000000000008),
                                    vcreate_s16(0x0000000000080008));

  int32x4_t shift2_2 = vcombine_s32(vcreate_s32(0xFFFFFFF800000008),
                                    vcreate_s32(0x0000000000000000));

  int64x2_t shift3_2 = vcombine_s64(vcreate_s64(0xFFFFFFFFFFFFFFF0),
                                    vcreate_s64(0x0000000000000008));

  // The output of the above shifts for the gb rows results in
  // [1 3 6 8 0 0 0 0] [0 0 9 0 11 13 14 5]
  // The desired output is [1 3 6 8 11 13 _ _] so in order to OR these together
  // we need to mask out the first 4 bytes of the second register
  uint64x1_t lowAnd = vcreate_u64(0xFFFFFFFF00000000);
#endif

  // Will be processing two rows at a time
  for(int i = 0; i < rows-1; i += 2)
  {
    bayer1 = bayer + (i*cols);

#if AVERAGE_GREEN
    // Only need to consider second row if averaging greens
    bayer2 = bayer + ((i+1)*cols);
#endif

    const s32 kNumElementsProcessed = 15;
    s32 j = 0;
    for(; j < cols-(kNumElementsProcessed-1); j += kNumElementsProcessed)
    {
      // Load 16 elements
      uint8x16_t row1 = vld1q_u8(bayer1);
      // Will only be processing 15 of them
      bayer1 += 15;

      // Perform the series of bit shifts, treaing the data as the appropriate types
      uint16x8_t t1 = vshlq_u16(vreinterpretq_u16_u8(row1), shift1_1);
      uint32x4_t t2 = vshlq_u32(vreinterpretq_u32_u8(t1),   shift2_1);
      uint64x2_t t3 = vshlq_u64(vreinterpretq_u64_u32(t2),  shift3_1);

      // Bitwise OR the two halves
      uint64x1_t high = vget_high_u64(t3);
      uint64x1_t low = vget_low_u64(t3);
      high = vorr_u64(high, low);

      uint8x8_t out = vreinterpret_u8_u64(high);
      // Saturating left shift by 2 as if the packed 5th byte
      // was all 0
      out = vqshl_n_u8(out, 2);

#if AVERAGE_GREEN
      // Repeat the same process with gb row
      row1 = vld1q_u8(bayer2);
      bayer2 += 15;

      t1 = vshlq_u16(vreinterpretq_u16_u8(row1), shift1_2);
      t2 = vshlq_u32(vreinterpretq_u32_u8(t1),   shift2_2);
      t3 = vshlq_u64(vreinterpretq_u64_u32(t2),  shift3_2);
      
      high = vget_high_u64(t3);
      low = vget_low_u64(t3);

      high = vand_u64(high, lowAnd);
      high = vorr_u64(high, low);

      uint8x8_t out2 = vreinterpret_u8_u64(high);
      out2 = vqshl_n_u8(out2, 2);
    
      out = vhadd_u8(out, out2);
#endif

      // Write the 6 pixels to output
      vst1_u8(grayPtr, out);
      grayPtr += 6;
    }

    // Process whatever is left over
    for(; j < cols; j += 5)
    {
#if AVERAGE_GREEN
      grayPtr[0] = cv::saturate_cast<u8>(((u16)bayer1[1] + (u16)bayer2[0]) << 2);
      grayPtr[1] = cv::saturate_cast<u8>(((u16)bayer1[3] + (u16)bayer2[2]) << 2);
      bayer2 += 5;
#else
      grayPtr[0] = cv::saturate_cast<u8>(((u16)bayer1[1]) << 2);
      grayPtr[1] = cv::saturate_cast<u8>(((u16)bayer1[3]) << 2);
#endif
      grayPtr += 2;
      bayer1 += 5;
    }
  }
#else // No neon support
  // Halve to RGB and then convert to gray
  // HalveBGGR10ToRGB has a non-neon implementation
  ImageRGB rgb;
  HalveBGGR10ToRGB(bayer, rows, cols, rgb);
  rgb.FillGray(gray);
#endif
}

  
}
}
}
