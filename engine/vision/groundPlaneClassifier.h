/**
 * File: GroundPlaneClassifier.h
 *
 * Author: Lorenzo Riano
 * Created: 11/29/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_GroundplaneClassifier_h
#define __Anki_Cozmo_Basestation_GroundplaneClassifier_h

#include "anki/common/basestation/math/polygon.h"
#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "engine/debugImageList.h"
#include "engine/vision/drivingSurfaceClassifier.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {
namespace Cozmo {

// Forward declaration
class CozmoContext;
struct OverheadEdgeFrame;
  
class GroundPlaneClassifier
{
public:
  GroundPlaneClassifier(const Json::Value& config, const CozmoContext *context);

  Result Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                DebugImageList <Vision::ImageRGB>& debugImageRGBs,
                std::list<OverheadEdgeFrame>& outEdges);

  bool IsInitizialized() const {
    return _initialized;
  }

  const DrivingSurfaceClassifier& GetClassifier() const {
    return *(_classifier.get());
  }

  static Vision::Image processClassifiedImage(const Vision::Image& binaryImage); //TODO remove static and put const

protected:
  std::unique_ptr<DrivingSurfaceClassifier> _classifier;
  const CozmoContext* _context;
  bool _initialized = false;

  void trainClassifier(const std::string& path);
  bool loadClassifier(const std::string& filename);

};

} // namespace Anki
} // namespace Cozmo


#endif //__Anki_Cozmo_Basestation_GroundplaneClassifier_h
