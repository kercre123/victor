/**
 * File: motionDetector.h
 *
 * Author: Andrew Stein
 * Date:   2017-04-25
 *
 * Description: Vision system component for detecting motion in images and/or on the ground plane.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Cozmo_Basestation_MotionDetector_H__
#define __Anki_Cozmo_Basestation_MotionDetector_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"

#include "anki/vision/basestation/image.h"

#include "anki/cozmo/basestation/debugImageList.h"

#include "clad/externalInterface/messageEngineToGame.h"

#include <list>
#include <string>

namespace Anki {
 
// Forward declaration
namespace Vision {
  class Camera;
  class ImageCache;
}
  
namespace Cozmo {
    
// Forward declaration:
struct VisionPoseData;
class VizManager;

class MotionDetector
{
public:
  
  MotionDetector(const Vision::Camera& camera, VizManager* vizManager);

  // Will use Color data if available in ImageCache, otherwise grayscale only
  Result Detect(Vision::ImageCache&     imageCache,
                const VisionPoseData&   crntPoseData,
                const VisionPoseData&   prevPoseData,
                std::list<ExternalInterface::RobotObservedMotion>& observedMotions,
                DebugImageList<Vision::ImageRGB>& debugImageRGBs);
  
private:
  
  template<class ImageType>
  Result DetectHelper(const ImageType&        resizedImage,
                      s32 origNumRows, s32 origNumCols, f32 scaleMultiplier,
                      const VisionPoseData&   crntPoseData,
                      const VisionPoseData&   prevPoseData,
                      std::list<ExternalInterface::RobotObservedMotion>& observedMotions,
                      DebugImageList<Vision::ImageRGB>& debugImageRGBs);
  
  s32 RatioTest(const Vision::Image& image,    Vision::Image& ratio12);
  s32 RatioTest(const Vision::ImageRGB& image, Vision::Image& ratio12);
  
  void SetPrevImage(const Vision::Image& image);
  void SetPrevImage(const Vision::ImageRGB& image);
  
  template<class ImageType>
  bool HavePrevImage() const;
  
  // Computes "centroid" at specified percentiles in X and Y
  static size_t GetCentroid(const Vision::Image& motionImg,
                            Anki::Point2f& centroid,
                            f32 xPercentile, f32 yPercentile);

  const Vision::Camera& _camera;
  
  Vision::ImageRGB _prevImageRGB;
  Vision::Image    _prevImageGray;
  
  TimeStamp_t   _lastMotionTime = 0;
  
  VizManager*   _vizManager = nullptr;
  
};
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_MotionDetector_H__ */

