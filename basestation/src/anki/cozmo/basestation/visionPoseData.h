/**
 * File: visionPoseData.h
 *
 * Author: Andrew Stein
 * Date:   (various)
 *
 * Description: Container for passing around pose/state information from a certain
 *              timestamp, useful for vision processing.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef __Anki_Cozmo_Basestation_VisionPoseData_H__
#define __Anki_Cozmo_Basestation_VisionPoseData_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/matrix.h"

#include "anki/cozmo/basestation/groundPlaneROI.h"
#include "anki/cozmo/basestation/robotPoseHistory.h"
#include "anki/cozmo/basestation/rollingShutterCorrector.h"

namespace Anki {
namespace Cozmo {

struct VisionPoseData
{
  TimeStamp_t           timeStamp;
  RobotPoseStamp        poseStamp;  // contains historical head/lift/pose info
  Pose3d                cameraPose; // w.r.t. pose in poseStamp
  bool                  groundPlaneVisible;
  Matrix_3x3f           groundPlaneHomography;
  GroundPlaneROI        groundPlaneROI;
  bool                  isBodyMoving = false;
  bool                  isHeadMoving = false;
  bool                  isLiftMoving = false;
  ImuDataHistory        imuDataHistory;
  
  VisionPoseData() = default;
  
  // ---------- Begin Custom copy implementation ------- //
  template<typename T1, typename T2>
  friend void swap(T1&& first, T2&& second);
  
  template<typename T>
  VisionPoseData(T&& other);
  
  VisionPoseData& operator=(VisionPoseData other);
  
}; // struct VisionPoseData
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
VisionPoseData::VisionPoseData(T&& other)
{
  swap(*this, std::forward<T>(other));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline VisionPoseData& VisionPoseData::operator=(VisionPoseData other)
{
  swap(*this, other);
  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T1, typename T2>
void swap(T1&& first, T2&& second)
{
  // This enables ADL
  using std::swap;
  
  swap(first.timeStamp, second.timeStamp);
  swap(first.poseStamp, second.poseStamp);
  swap(first.cameraPose, second.cameraPose);
  swap(first.groundPlaneVisible, second.groundPlaneVisible);
  swap(first.groundPlaneHomography, second.groundPlaneHomography);
  swap(first.groundPlaneROI, second.groundPlaneROI);
  swap(first.isBodyMoving, second.isBodyMoving);
  swap(first.isHeadMoving, second.isHeadMoving);
  swap(first.isLiftMoving, second.isLiftMoving);
  swap(first.imuDataHistory, second.imuDataHistory);
  
  // Because the cameraPose is wrt the pose contained in poseStamp, set it explicitly
  first.cameraPose.SetParent(&(first.poseStamp.GetPose()));
  second.cameraPose.SetParent(&(second.poseStamp.GetPose()));
}


} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_VisionPoseData_H__ */
