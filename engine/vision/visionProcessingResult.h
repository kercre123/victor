/**
 * File: visionProcessingResult.h
 *
 * Author: Andrew Stein
 * Date:   12/10/2018
 *
 * Description: Everything that can be generated from one image in one big package.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Vector_VisionProcessingRestul_H__
#define __Anki_Vector_VisionProcessingRestul_H__

#include "coretech/common/engine/robotTimeStamp.h"

#include "coretech/vision/engine/compressedImage.h"
#include "coretech/vision/engine/trackedFace.h"
#include "coretech/vision/engine/trackedPet.h"
#include "coretech/vision/engine/visionMarker.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/cameraParams.h"
#include "clad/types/imageTypes.h"

#include "engine/debugImageList.h"
#include "engine/overheadEdge.h"
#include "engine/vision/visionModeSet.h"

#include <list>

namespace Anki {
namespace Vector {

struct VisionProcessingResult
{
  RobotTimeStamp_t timestamp; // Always set, even if all the lists below are empty (e.g. nothing is found)
  VisionModeSet modesProcessed;
  
  Vision::ImageQuality imageQuality;
  Vision::CameraParams cameraParams;
  u8 imageMean;
  
  std::list<ExternalInterface::RobotObservedMotion>     observedMotions;
  std::list<Vision::ObservedMarker>                     observedMarkers;
  std::list<Vision::TrackedFace>                        faces;
  std::list<Vision::TrackedPet>                         pets;
  std::list<OverheadEdgeFrame>                          overheadEdges;
  std::list<Vision::UpdatedFaceID>                      updatedFaceIDs;
  std::list<ExternalInterface::RobotObservedLaserPoint> laserPoints;
  std::list<Vision::CameraCalibration>                  cameraCalibration;
  std::list<OverheadEdgeFrame>                          visualObstacles;
  std::list<Vision::SalientPoint>                       salientPoints;
  ExternalInterface::RobotObservedIllumination          illumination;
  
  Vision::CompressedImage compressedDisplayImg;
  Vision::ImageRGB565 mirrorModeImg;
  
  // Used to pass debug images back to main thread for display:
  DebugImageList<Vision::CompressedImage> debugImages;
  
  // Returns timestamp of the first detection for the corresponding mode in this result, zero if none.
  // If atTimestamp != 0, the detection(s) must match the given timestamp as well (since in some cases,
  //  such as salientPoints, a detections' timestamp may not match the VisionProcessingResult's timestamp).
  //  In this case, the returned timestamp, if any, will of course match atTimestamp.
  // Note that this method may not be true even if the mode was processed, as indicated by 'modesProcessed'.
  TimeStamp_t ContainsDetectionsForMode(const VisionMode mode, const TimeStamp_t atTimestamp) const;
  
};

} // namespace Vector
} // namespace Anki

#endif /* __Anki_Vector_VisionProcessingRestul_H__ */
