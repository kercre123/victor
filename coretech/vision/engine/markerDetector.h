/**
 * File: markerDetector.h
 *
 * Author: Andrew Stein
 * Date:   08/22/2017
 *
 * Description: Detector for Vision Markers, which wraps the old Embedded implementation with an 
 *              engine/basestation-friendly API.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Vision_MarkerDetector_H__
#define __Anki_Vision_MarkerDetector_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/rect.h"

#include "coretech/vision/engine/visionMarker.h"

#include <list>
#include <vector>

namespace Anki {
namespace Vision {
  
// Forward declaration:
class Camera;
class Image;
  
class MarkerDetector
{
public:
  
  MarkerDetector(const Camera& camera);
  ~MarkerDetector();
  
  // TODO: Pass in Json config and set parameters from there
  Result Init(s32 numRows, s32 numCols);
  
  Result Detect(const Image& inputImage, std::list<ObservedMarker>& observedMarkers);
    
private:

  const Camera&           _camera;
  
  // Using forward declaration and unique_ptr pattern for Parameters and Memory because
  // those internally rely on old, crazy "Embedded" code and data structures and we don't
  // want to expose that outside of this class.
  
  struct Parameters;
  std::unique_ptr<Parameters> _params;
  
  class Memory;
  std::unique_ptr<Memory> _memory;
  
};
  
} // namespace Vision
} // namesapce Anki

#endif /* __Anki_Vision_MarkerDetector_H__ */
