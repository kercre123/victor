/**
 * File: DevProxSensorVisualizer.h
 *
 * Author: Lorenzo Riano
 * Created: 2/20/18
 *
 * Description: Vision System Component that uses prox sensor data to generate drivable and non drivable pixels
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_DevProxSensorVisualizer_H__
#define __Anki_Cozmo_DevProxSensorVisualizer_H__

#include "coretech/common/shared/types.h"
#include "engine/debugImageList.h"

namespace Anki {

// Forward declaration
namespace Vision {
class Camera;
class ImageCache;
class ImageRGB;
}
namespace Cozmo {

// Forward declaration:
struct VisionPoseData;

class ProxSensorImageAnalyzer
{
public:

  explicit ProxSensorImageAnalyzer(const Json::Value& config);
  Result Update(const Vision::ImageRGB& image, const VisionPoseData& crntPoseData,
                  const VisionPoseData& prevPoseData,
                  DebugImageList <Anki::Vision::ImageRGB>& debugImageRGBs) const;

private:
  std::pair<Anki::Array2d<float>, Anki::Array2d<float>>
  GetDrivableNonDrivable(const Vision::ImageRGB& image, const float minRow, const float maxRow,
                           const float col, Vision::ImageRGB& debugImage,
                           DebugImageList <Anki::Vision::ImageRGB>& debugImageRGBs) const;
};

} // namespace Cozmo
} // namespace Anki


#endif //__Anki_Cozmo_DevProxSensorVisualizer_H__
