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
      CAMERA_RES_VGA = 0,
      CAMERA_RES_QVGA,
      CAMERA_RES_QQVGA,
      CAMERA_RES_QQQVGA,
      CAMERA_RES_QQQQVGA,
      CAMERA_RES_VERIFICATION_SNAPSHOT,
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
      {640, 480},
      {320, 240},
      {160, 120},
      {80, 60},
      {40, 30},
      {16,16}
    };

  } // namespace Vision
} // namespace Anki

#endif // ANKI_CAMERA_TYPES_H

