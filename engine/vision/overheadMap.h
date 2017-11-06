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

class CozmoContext;

/*
 * This class maintains an overhead map of the robot's surrounding. The map has images taken
 * from the ground plane in front of the robot. Every time the robot covers a distance the map
 * is updated/overwritten. The area underneath the robot can be extracted using GetImageCenteredOnRobot()
 */
class OverheadMap
{
public:
  OverheadMap(int numrows, int numcols, const CozmoContext *context);
  OverheadMap(const Json::Value& config, const CozmoContext *context);

  Result Update(const Vision::ImageRGB& image, const VisionPoseData& poseData,
                DebugImageList <Vision::ImageRGB>& debugImageRGBs);

  const Vision::ImageRGB& GetOverheadMap() const;
  const Vision::Image& GetFootprintMask() const;

  // Save the generated overhead map plus a list of drivable and non drivable pixels.
  // Used for offline training and testing
  void SaveMaskedOverheadPixels(const std::string& positiveExamplesFilename,
                                const std::string& negativeExamplesFileName,
                                const std::string& overheadMapFileName) const;

  // Returns a pair of vectors:
  //  * all the pixels in the overhead map that are non-black in the footprint mask, i.e. areas that the robot traversed
  //  * all the pixels in the overhead map that the robot mapped but didn't traverse, making them potential obstacles
  void GetDrivableNonDrivablePixels(std::vector<Vision::PixelRGB>& drivablePixels,
                                    std::vector<Vision::PixelRGB>& nonDrivablePixels) const;

private:

  // Set the overhead map to be all black
  void ResetMaps();
  // Paint all the _footprintMask pixels underneath the robot footprint with white
  void UpdateFootprintMask(const Pose3d& robotPose, DebugImageList <Anki::Vision::ImageRGB>& debugImageRGBs);

  Vision::ImageRGB _overheadMap;
  Vision::Image _footprintMask; // this image has 0 (black) if the robot has never been to that position, 1 otherwise
  const CozmoContext*  _context;

  // Calculate the footprint as a cv::RotatedRect. To get the correct size the footprint is aligned
  // with the axis
  cv::RotatedRect GetFootprintRotatedRect(const Pose3d& robotPose) const;

  Vision::ImageRGB GetImageCenteredOnRobot(const Pose3d& robotPose,
                                           DebugImageList<Anki::Vision::ImageRGB>& debugImageRGBs) const;
};

} // namespace Cozmo
} // namespace Anki


#endif //__Anki_Cozmo_OverheadMap_H__
