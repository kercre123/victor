/**
 * File: scanlineDistorter.h
 *
 * Author: Andrew Stein
 * Created: 5/15/2017
 *
 * Description: Responsible for holding distortion parameters for a ProceduralFace
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_ScanlineDistorter_H__
#define __Anki_Cozmo_ScanlineDistorter_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/point.h"

#include <array>
#include <vector>

namespace Anki {
  
namespace Util {
  class RandomGenerator;
}
  
namespace Vision {
  class ImageRGB;
}

namespace Cozmo {
  
class ProceduralFace;

  
class ScanlineDistorter
{
public:
  
  ScanlineDistorter(s32 maxAmount_pix, f32 noiseProb);
  ScanlineDistorter(const ScanlineDistorter& other) = default;
  
  void Update(s32 maxAmount_pix); // +ve for increase, -ve for decrease
  
  // Given the fractional vertical eye position, returns the number of pixels of horizontal
  // distortion to apply.
  s32 GetEyeDistortionAmount(f32 eyeFrac) const;
  
  // Given the "warp" matrix which positions/scales the eye in the face, draws the corresponding "off"
  // noise into the image.
  void AddOffNoise(const SmallMatrix<2,3,f32>& warpMatrix,
                   const s32 eyeHeight, const s32 eyeWidth,
                   Vision::ImageRGB& faceImg) const;
  
  // Gets sequence of distortions using the ScanlineDistorter in the given faceData.
  // Call until it returns false, which indicates there are no more distortion frames and the face is back in its
  // original state. The output "offset" indicates the desired timing since the previous state.
  static bool GetNextDistortionFrame(const f32 degree, ProceduralFace& faceData, TimeStamp_t& timeInc);
  
private:

  ScanlineDistorter() = default;
  
  enum class Shape : u8
  {
    Skew = 0,  // 2 control points, top and bottom
    Triangle,  // 3 control points, top/bottom shift one way, middle shifts other
    S_Curve,   // 4 control points, shifts in alternating directions
    Count
  };
  
  struct ControlPoint
  {
    f32 vertical_frac; // vertical position within the eye, relative to eye height
    s32 direction;
    s32 amount_pix;
    
    ControlPoint() = default;
    
    ControlPoint(f32 vFrac, s32 dir, s32 amount)
    : vertical_frac(vFrac), direction(dir), amount_pix(amount)
    {
      
    }
  };
  
  std::vector<ControlPoint> _controlPoints;
  std::vector<Point2f>      _offNoisePoints; // relative to eye center/size
  
  Shape _shape;
  
  static Util::RandomGenerator& GetRNG();
  
}; // class ScanlineDistorter
  
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_ScanlineDistorter_H__ */
