/**
 * File: DevProxSensorVisualizer.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2/20/18
 *
 * Description: Vision System Component to use prox sensor data to generate drivable and non drivable pixels
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/vision/engine/imageCache.h"
#include "engine/vision/visionPoseData.h"
#include "engine/groundPlaneROI.h"
#include "util/logging/logging.h"
#include "proxSensorImageAnalyzer.h"

namespace Anki {
namespace Cozmo {

static const char* kLogChannelName = "VisionSystem";

ProxSensorImageAnalyzer::ProxSensorImageAnalyzer(const Json::Value& config)
{

}

Result ProxSensorImageAnalyzer::Update(const Vision::ImageRGB& image,
                                       const VisionPoseData& poseData,
                                       DebugImageList<Anki::Vision::ImageRGB>& debugImageRGBs) const
{

  PRINT_CH_DEBUG(kLogChannelName, "DevProxSensorVisualizer.Update.IsAlive","");
  if (! poseData.groundPlaneVisible) {
    PRINT_CH_DEBUG(kLogChannelName, "DevProxSensorVisualizer.Update.GroundPlaneNotVisible", "");
    return RESULT_OK;
  }


  // Obtain the overhead ground plane image
  GroundPlaneROI groundPlaneROI;
  const Matrix_3x3f& H = poseData.groundPlaneHomography;
  Vision::ImageRGB imageToDisplay = groundPlaneROI.GetOverheadImage(image, H);
  
  // Get the prox sensor reading
  const HistRobotState& hist = poseData.histState;
  if (hist.WasProxSensorValid()) {
    // Draw a line where the prox indicates
    const int distance_mm = hist.GetProxSensorVal_mm();
    // draw at the border if too large
    const int col = std::min(distance_mm, imageToDisplay.GetNumCols() - 1);
    imageToDisplay.DrawLine({float(col), 0.0},
                            {float(col), float(imageToDisplay.GetNumRows()-1)},
                            NamedColors::RED);
  }

  debugImageRGBs.emplace_back("ProxSensorOnGroundPlane", imageToDisplay);

  return RESULT_OK;
}

} // namespace Anki
} // namespace Cozmo