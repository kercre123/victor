/**
 * File: halveBGGR10ToRGB.cpp
 *
 * Author:  Al Chaussee
 * Created: 10/4/2018
 *
 * Description: Halve BGGR10 bayer to RGB
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/vision/engine/imageBuffer/conversions/imageConversions.h"

#include "coretech/vision/engine/imageBuffer/conversions/debayer.h"
#include "coretech/vision/engine/image_impl.h"
#include "coretech/vision/engine/neonMacros.h"

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"

#include <unistd.h>

namespace Anki {
namespace Vision {
namespace ImageConversions {

void HalveBGGR10ToRGB(const u8* bayer_in, s32 rows, s32 cols, ImageRGB& rgb)
{
  // Output image is half the resolution of the bayer image
  rgb.Allocate(rows / 2, cols / 2);

  // Multiply cols by (10/8) as cols is the resolution of the bayer image
  // but this functions expects the width and height in bytes
  const s32 bayer_sx = (cols*10)/8;
  const s32 bayer_sy = rows;
  bayer_mipi_bggr10_downsample(bayer_in, reinterpret_cast<u8*>(rgb.GetRow(0)),
                               bayer_sx, bayer_sy, 10);
}
  
}
}
}
