/**
 * File: groundPlaneROI.h
 *
 * Author: Andrew Stein
 * Date:   11/20/15
 *
 * Description: Defines a class for visual reasoning about a region of interest (ROI)
 *              on the ground plane immediately in front of the robot.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __Anki_Cozmo_Basestation_GroundPlaneROI_H__
#define __Anki_Cozmo_Basestation_GroundPlaneROI_H__

#include "anki/vision/basestation/image.h"
#include "anki/common/basestation/math/matrix.h"

namespace Anki {
namespace Cozmo {

class GroundPlaneROI
{
public:
  // Define ROI quad on ground plane, in robot-centric coordinates (origin is *)
  // The region is "length" mm long and starts "dist" mm from the robot origin.
  // It is "w_close" mm wide at the end close to the robot and "w_far" mm
  // wide at the opposite end
  //                              _____
  //  +---------+    _______------     |
  //  | Robot   |   |                  |
  //  |       * |   | w_close          | w_far
  //  |         |   |_______           |
  //  +---------+           ------_____|
  //
  //          |<--->|<---------------->|
  //           dist         length
  //
  
  f32 GetDist()       const { return _dist; }
  f32 GetWidthFar()   const { return _widthFar; }
  f32 GetWidthClose() const { return _widthClose; }
  f32 GetLength()     const { return _length; }
  
  Quad3f GetGroundQuad(f32 z=0.f) const;
  Quad2f GetImageQuad(const Matrix_3x3f& H) const;
  
  Vision::ImageRGB GetOverheadImage(const Vision::ImageRGB& image,
                                    const Matrix_3x3f& H) const;
  
  Vision::Image GetOverheadImage(const Vision::Image& image,
                                 const Matrix_3x3f& H) const;
  
  // Creates the mask on first request and then just returns that one from then on
  const Vision::Image& GetOverheadMask() const;
  
  Point2f GetOverheadImageOrigin() const { return Point2f{_dist, -_widthFar*0.5f}; }
  
private:
  // In mm
  f32 _dist = 50.f;
  f32 _length = 100.f;
  f32 _widthFar = 80.f;
  f32 _widthClose = 30.f;
  
  mutable Vision::Image _overheadMask;
  
  template<class PixelType>
  void GetOverheadImageHelper(const Vision::ImageBase<PixelType>& image,
                              const Matrix_3x3f& H,
                              Vision::ImageBase<PixelType>& overheadImg) const;
}; // class GroundPlaneROI

} // namespace Cozmo
} // namespace Anki

#endif // __Anki_Cozmo_Basestation_GroundPlaneROI_H__

