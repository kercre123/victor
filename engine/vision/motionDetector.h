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

#include "engine/debugImageList.h"

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

// Class for detecting motion in various areas of the image.
// There's two main components: one that detects motion on the ground plane, and one that detects motion in the
// peripheral areas (top, left and right).
class MotionDetector
{
public:
  
  MotionDetector(const Vision::Camera &camera, VizManager *vizManager, const Json::Value &config);

  // Will use Color data if available in ImageCache, otherwise grayscale only
  Result Detect(Vision::ImageCache&     imageCache,
                const VisionPoseData&   crntPoseData,
                const VisionPoseData&   prevPoseData,
                std::list<ExternalInterface::RobotObservedMotion>& observedMotions,
                DebugImageList<Vision::ImageRGB>& debugImageRGBs);

  ~MotionDetector();

private:

  template<class ImageType>
  Result DetectHelper(const ImageType &resizedImage,
                      s32 origNumRows, s32 origNumCols, f32 scaleMultiplier,
                      const VisionPoseData &crntPoseData,
                      const VisionPoseData &prevPoseData,
                      std::list<ExternalInterface::RobotObservedMotion> &observedMotions,
                      DebugImageList<Vision::ImageRGB> &debugImageRGBs);

  // To detect peripheral motion, a simple impulse-decay model is used. The longer motion is detected in a
  // specific area, the higher its activation will be. When it reaches a max value motion is activated in
  // that specific area.
  bool DetectPeripheralMotionHelper(Vision::Image &ratioImage,
                                    DebugImageList<Anki::Vision::ImageRGB> &debugImageRGBs,
                                    ExternalInterface::RobotObservedMotion &msg, f32 scaleMultiplier);

  bool DetectGroundAndImageHelper(Vision::Image &foregroundMotion, int numAboveThresh, s32 origNumRows,
                                  s32 origNumCols, f32 scaleMultiplier,
                                  const VisionPoseData &crntPoseData,
                                  const VisionPoseData &prevPoseData,
                                  std::list<ExternalInterface::RobotObservedMotion> &observedMotions,
                                  DebugImageList<Anki::Vision::ImageRGB> &debugImageRGBs,
                                  ExternalInterface::RobotObservedMotion &msg);

  template <class ImageType>
  void FilterImageAndPrevImages(const ImageType& image);

  void ExtractGroundPlaneMotion(s32 origNumRows, s32 origNumCols, f32 scaleMultiplier,
                                const VisionPoseData &crntPoseData,
                                const Vision::Image &foregroundMotion,
                                const Point2f &centroid,
                                Point2f &groundPlaneCentroid,
                                f32 &groundRegionArea) const;

  // Returns the number of times the ratio between the pixels in image and the pixels
  // in the previous image is above a threshold. The corresponding pixels in ratio12
  // will be set to 255
  s32 RatioTest(const Vision::Image& image,    Vision::Image& ratio12);
  s32 RatioTest(const Vision::ImageRGB& image, Vision::Image& ratio12);
  
  void SetPrevImage(const Vision::Image &image, bool wasBlurred);
  void SetPrevImage(const Vision::ImageRGB &image, bool wasBlurred);
  
  template<class ImageType>
  bool HavePrevImage() const;
  
  // Computes "centroid" at specified percentiles in X and Y
  static size_t GetCentroid(const Vision::Image& motionImg,
                            Anki::Point2f& centroid,
                            f32 xPercentile, f32 yPercentile);
  // The joy of pimpl :)
  class ImageRegionSelector;
  std::unique_ptr<ImageRegionSelector> _regionSelector;

  const Vision::Camera& _camera;
  
  Vision::ImageRGB _prevImageRGB;
  Vision::Image    _prevImageGray;
  bool _wasPrevImageRGBBlurred = false;
  bool _wasPrevImageGrayBlurred = false;
  
  TimeStamp_t   _lastMotionTime = 0;
  
  VizManager*   _vizManager = nullptr;

  const Json::Value& _config;
};
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_MotionDetector_H__ */

