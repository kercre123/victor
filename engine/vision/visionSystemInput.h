/**
 * File: visionSystemInput.h
 *
 * Author: Al Chaussee
 * Date:   9/18/18
 *
 * Description: Collection of all inputs for VisionSystem to be able to process an image
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Engine_Vision_VisionSystemInput_H__
#define __Engine_Vision_VisionSystemInput_H__

#include "engine/vision/visionPoseData.h"
#include "coretech/vision/engine/imageBuffer/imageBuffer.h"
#include "coretech/vision/engine/imageCache.h"

namespace Anki {
namespace Vector {

struct VisionSystemInput
{
  // Whether or not input is locked by processor
  std::atomic<bool> locked;

  // Wrapper around raw image data
  Vision::ImageBuffer imageBuffer;

  // Pose data corresponding to imageBuffer's data
  VisionPoseData poseData;

  // Resize method for ImageCache to use
  Vision::ResizeMethod resizeMethod = Vision::ResizeMethod::Linear;
};

}
}

#endif
