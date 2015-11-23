/**
 * File: groundPlaneROI.cpp
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

#include "anki/cozmo/basestation/groundPlaneROI.h"

#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"
#include "anki/common/basestation/math/matrix_impl.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace Anki {
namespace Cozmo {

 
const Vision::Image& GroundPlaneROI::GetOverheadMask() const
{
  if(_overheadMask.IsEmpty())
  {
    _overheadMask = Vision::Image(_widthFar, _length);
    const s32 w = std::round(0.5f*(_widthFar - _widthClose));
    _overheadMask.FillWith(0);
    cv::fillConvexPoly(_overheadMask.get_CvMat_(), std::vector<cv::Point>{
      cv::Point(0, w),
      cv::Point(_length-1,0),
      cv::Point(_length-1,_widthFar-1),
      cv::Point(0, w + _widthClose)
    }, 255);
  }
  return _overheadMask;
}

Quad3f GroundPlaneROI::GetGroundQuad(f32 zHeight) const
{
  return Quad3f{
    {_dist + _length   ,  0.5f*_widthFar   , zHeight},
    {_dist             ,  0.5f*_widthClose , zHeight},
    {_dist + _length   , -0.5f*_widthFar   , zHeight},
    {_dist             , -0.5f*_widthClose , zHeight}
  };
}

Quad2f GroundPlaneROI::GetImageQuad(const Matrix_3x3f& H) const
{
  // Note that the z coordinate is actually 0, but in the mapping to the
  // image plane below, we are actually doing K[R t]* [Px Py Pz 1]',
  // and Pz == 0 and we thus drop out the third column, making it
  // K[R t] * [Px Py 0 1]' or H * [Px Py 1]', so for convenience, we just
  // go ahead and fill in that 1 here as if it were the "z" coordinate:
  const Quad3f groundQuad = GetGroundQuad(1.f);
  
  // Project ground quad in camera image
  // (This could be done by Camera::ProjectPoints, but that would duplicate
  //  the computation of H we did above, which here we need to use below)
  Quad2f imgGroundQuad;
  for(Quad::CornerName iCorner = Quad::CornerName::FirstCorner;
      iCorner != Quad::CornerName::NumCorners; ++iCorner)
  {
    Point3f temp = H * groundQuad[iCorner];
    ASSERT_NAMED(temp.z() > 0.f, "Projected ground quad points should have z > 0.");
    const f32 divisor = 1.f / temp.z();
    imgGroundQuad[iCorner].x() = temp.x() * divisor;
    imgGroundQuad[iCorner].y() = temp.y() * divisor;
  }
  
  return imgGroundQuad;
} // GetImageQuad()

template<class PixelType>
void GroundPlaneROI::GetOverheadImageHelper(const Vision::ImageBase<PixelType>& image, const Matrix_3x3f& H,
                                            Vision::ImageBase<PixelType>& overheadImg) const
{
  // Need to apply a shift after the homography to put things in image
  // coordinates with (0,0) at the upper left (since groundQuad's origin
  // is not upper left). Also mirror Y coordinates since we are looking
  // from above, not below
  Matrix_3x3f InvShift{
    1.f, 0.f, _dist, // Negated b/c we're using inv(Shift)
    0.f,-1.f, _widthFar*0.5f,
    0.f, 0.f, 1.f};
  
  // Note that we're applying the inverse homography, so we're doing
  //  inv(Shift * inv(H)), which is the same as  (H * inv(Shift))
  cv::warpPerspective(image.get_CvMat_(), overheadImg.get_CvMat_(), (H*InvShift).get_CvMatx_(),
                      cv::Size(_length, _widthFar), cv::INTER_LINEAR | cv::WARP_INVERSE_MAP);
  
  const Vision::Image& mask = GetOverheadMask();
  
  ASSERT_NAMED(overheadImg.IsContinuous() && mask.IsContinuous(),
               "Overhead image and mask should be continous.");
  
  // Zero out masked regions
  PixelType* imgData = overheadImg.GetDataPointer();
  const u8* maskData = mask.GetDataPointer();
  for(s32 i=0; i<_overheadMask.GetNumElements(); ++i) {
    if(maskData[i] == 0) {
      imgData[i] = PixelType(0);
    }
  }
  
} // GetOverheadImage()

Vision::ImageRGB GroundPlaneROI::GetOverheadImage(const Vision::ImageRGB &image, const Matrix_3x3f &H) const
{
  Vision::ImageRGB overheadImg(_overheadMask.GetNumRows(), _overheadMask.GetNumCols());
  GetOverheadImageHelper(image, H, overheadImg);
  return overheadImg;
}

Vision::Image GroundPlaneROI::GetOverheadImage(const Vision::Image &image, const Matrix_3x3f &H) const
{
  Vision::Image overheadImg(_overheadMask.GetNumRows(), _overheadMask.GetNumCols());
  GetOverheadImageHelper(image, H, overheadImg);
  return overheadImg;
}

  
} // namespace Cozmo
} // namespace Anki
