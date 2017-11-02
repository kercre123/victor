/**
 * File: overHeadMap.h
 *
 * Author: Lorenzo Riano
 * Created: 10/9/17
 *
 * Description: This class maintains an overhead map of the robot's surrounding. The map has images taken
 * from the ground plane in front of the robot.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_OverheadMap_H__
#define __Anki_Cozmo_OverheadMap_H__

#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "engine/debugImageList.h"
#include "engine/vision/visionPoseData.h"

namespace Anki {
namespace Cozmo {

/*
 * This class maintains an overhead map of the robot's surrounding. The map has images taken
 * from the ground plane in front of the robot. Every time the robot covers a distance the map
 * is updated/overwritten. The area underneath the robot can be extracted using GetImageCenteredOnRobot()
 */
class OverheadMap
{
public:
  OverheadMap(int numrows, int numcols);
  OverheadMap(const Json::Value &config);

  Result Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                DebugImageList <Vision::ImageRGB>& debugImageRGBs);
  const Vision::ImageRGB& GetOverheadMap() const;

  Vision::ImageRGB GetImageCenteredOnRobot(const Pose3d& robotPose,
                                           DebugImageList<Anki::Vision::ImageRGB>& debugImageRGBs) const;

private:

  Vision::ImageRGB _overheadMap;

};

} // namespace Cozmo
} // namespace Anki


#endif //__Anki_Cozmo_OverheadMap_H__
