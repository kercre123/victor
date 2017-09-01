/**
 * File: detectOverHeadEdges.h
 *
 * Author: Lorenzo Riano
 * Date:   2017-08-31
 *
 * Description: Vision system component for detecting edges in the ground plane.
 *
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Cozmo_Basestation_DetectOverheadEdges_H__
#define __Anki_Cozmo_Basestation_DetectOverheadEdges_H__

#include "overheadEdgesDetector.h"
#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "visionSystem.h"


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

class OverheadEdgesDetector {

public:
  OverheadEdgesDetector(const Vision::Camera& camera,
                      VizManager* vizManager,
                      VisionSystem* visionSystem,
                      f32 kEdgeThreshold = 50.0f,
                      u32 kMinChainLength = 3 );

  Result Detect(Anki::Vision::ImageCache &imageCache, const VisionPoseData *crntPoseData,
                  VisionProcessingResult *currentResult);

private:
  template<typename ImageTraitType>
  Result DetectHelper(const typename ImageTraitType::ImageType &image,
                      const VisionPoseData *crntPoseData,
                      VisionProcessingResult *currentResult);

  const Vision::Camera& _camera;
  VizManager* _vizManager = nullptr;
  VisionSystem* _visionSystem = nullptr;
  const f32 _kEdgeThreshold = 50.0f;
  const u32 _kMinChainLength = 3;

};

}

}

#endif