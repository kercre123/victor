/**
 * File: imageConversions.h
 *
 * Author:  Al Chaussee
 * Created: 10/4/2018
 *
 * Description: Various optimized image conversions
 *              Implementations are in individual cpp files
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Coretech_Vision_Engine_ImageConversions_H__
#define __Anki_Coretech_Vision_Engine_ImageConversions_H__

#include "coretech/common/shared/types.h"

namespace Anki {
namespace Vision {

class Image;
class ImageRGB;

namespace ImageConversions {

  // Converts YUV420sp formatted data of an image of size rows x cols
  // to RGB
  // NEON optimized
  void ConvertYUV420spToRGB(const u8* yuv, s32 rows, s32 cols,
                            ImageRGB& rgb);

  // Converts a Bayer BGGR 10bit image to RGB
  // Halves so output is half the resolution of the bayer image
  void HalveBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                        ImageRGB& rgb);
  
  // Converts a Bayer BGGR 10bit image to Gray
  // Halves so output is half the resolution of the bayer image
  void HalveBGGR10ToGray(const u8* bayer, s32 rows, s32 cols,
                         Image& gray);
  
  // Converts a Bayer BGGR 10bit image to RGB
  // Demosaics so output is same resolution as bayer image
  void DemosaicBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                           ImageRGB& rgb);

  // Converts a Bayer BGGR 10bit image to RGB
  // Output image is quarter the size of the bayer image
  void QuarterBGGR10ToRGB(const u8* bayer, s32 rows, s32 cols,
                         ImageRGB& rgb);
}
}
}

#endif
