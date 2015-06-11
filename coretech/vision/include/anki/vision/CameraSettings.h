// CameraSettings.h
//
// Description: Types and constants pertaining to generic camera settings
//
//  Created by Kevin Yoon on 4/21/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//

#ifndef ANKI_CAMERA_SETTINGS_H
#define ANKI_CAMERA_SETTINGS_H

#include "anki/common/types.h"

namespace Anki {
  namespace Vision {
    typedef enum
    {
      CAMERA_RES_QUXGA = 0, // 3200 x 2400
      CAMERA_RES_QXGA,      // 2048 x 1536
      CAMERA_RES_UXGA,      // 1600 x 1200
      CAMERA_RES_SXGA,      // 1280 x 960, technically SXGA-
      CAMERA_RES_XGA,       // 1024 x 768
      CAMERA_RES_SVGA,      // 800 x 600
      CAMERA_RES_VGA,       // 640 x 480
      CAMERA_RES_CVGA,      // 400 x 296
      CAMERA_RES_QVGA,      // 320 x 240
      CAMERA_RES_QQVGA,     // 160 x 120
      CAMERA_RES_QQQVGA,    // 80 x 60
      CAMERA_RES_QQQQVGA,   // 40 x 30
      CAMERA_RES_VERIFICATION_SNAPSHOT, // 16 x 16
      CAMERA_RES_COUNT,
      CAMERA_RES_NONE = CAMERA_RES_COUNT
    } CameraResolution;

    typedef struct
    {
      u16 width;
      u16 height;
    } ImageDims;

    const ImageDims CameraResInfo[CAMERA_RES_COUNT] =
    {
      {3200, 2400},
      {2048, 1536},
      {1600, 1200},
      {1280, 960},
      {1024, 768},
      {800,  600},
      {640,  480},
      {400,  296},
      {320,  240},
      {160,  120},
      {80,   60},
      {40,   30},
      {16,   16}
    };

    typedef enum {
      IE_NONE,
      IE_RAW_GRAY, // no compression
      IE_RAW_RGB,  // no compression, just [RGBRGBRG...]
      IE_YUYV,
      IE_BAYER,
      IE_JPEG_GRAY,
      IE_JPEG_COLOR,
      IE_JPEG_CHW, // Color half width
      IE_MINIPEG_GRAY   // Minimized grayscale JPEG - no header, no footer, no byte stuffing
    } ImageEncoding_t;

  } // namespace Vision
} // namespace Anki

#endif // ANKI_CAMERA_TYPES_H
