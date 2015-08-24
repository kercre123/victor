#include "proceduralFace.h"

#include <opencv2/highgui/highgui.hpp>

namespace Anki {
namespace Cozmo {

  ProceduralFace::ProceduralFace()
  {
    Reset();
  }
  
  void ProceduralFace::Reset()
  {
    _faceAngle_deg = 0;
    
    for(int whichEye=0; whichEye<2; ++whichEye) {
      _eyeParams[whichEye].fill(0);
      _eyeParams[whichEye][EyeHeight] = NominalEyeHeight;
      _eyeParams[whichEye][PupilHeight] = NominalPupilHeight;
    }
  }
  
  cv::Mat_<u8> ProceduralFace::GetFace() const
  {
    cv::Mat_<u8> faceImg(HEIGHT, WIDTH);
    
    // Draw left eyebrow
    {
      const float cosAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Left][BrowAngle])));
      const float sinAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Left][BrowAngle])));
      const Value x = NominalLeftEyeCenX + _eyeParams[Left][BrowShiftX];
      const Value y = NominalEyebrowHeight + _eyeParams[Left][BrowShiftY];
      const cv::Point leftPoint(x-EyebrowHalfLength*cosAngle, y-EyebrowHalfLength*sinAngle);
      const cv::Point rightPoint(x+EyebrowHalfLength*cosAngle, y+EyebrowHalfLength*sinAngle);
      cv::line(faceImg, leftPoint, rightPoint, 255);
    }
    
    // Draw right eyebrow
    {
      const float cosAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Right][BrowAngle])));
      const float sinAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Right][BrowAngle])));
      const Value x = NominalRightEyeCenX + _eyeParams[Right][BrowShiftX];
      const Value y = NominalEyebrowHeight + _eyeParams[Right][BrowShiftY];
      const cv::Point leftPoint(x-EyebrowHalfLength*cosAngle, y-EyebrowHalfLength*sinAngle);
      const cv::Point rightPoint(x+EyebrowHalfLength*cosAngle, y+EyebrowHalfLength*sinAngle);
      cv::line(faceImg, leftPoint, rightPoint, 255);
    }
    
    // Draw left eye
    {
      const Value x = NominalLeftEyeCenX-EyeWidth/2;
      const Value y = NominalEyeCenY - _eyeParams[Left][EyeHeight]/2;
      const cv::Rect eyeRect(x, y, EyeWidth, _eyeParams[Left][EyeHeight]);
      cv::rectangle(faceImg, eyeRect, 255);

      // Set every other row to 0 to get interlaced appearance
      cv::Mat roi = faceImg(eyeRect);
      for(int i=0; i<roi.rows; i+=2) {
        roi[i].setTo(0);
      }
    }
    
    // Draw right eye
    {
      const Value x = NominalRightEyeCenX-EyeWidth/2;
      const Value y = NominalEyeCenY - _eyeParams[Right][EyeHeight]/2;
      const cv::Rect eyeRect(x, y, EyeWidth, _eyeParams[Right][EyeHeight]);
      cv::rectangle(faceImg, eyeRect, 255);
      
      // Set every other row to 0 to get interlaced appearance
      cv::Mat roi = faceImg(eyeRect);
      for(int i=0; i<roi.rows; i+=2) {
        roi[i].setTo(0);
      }
    }
    
    // Draw left pupil
    {
      const x = NominalLeftEyeCenX + _eyeParams[Left][PupilShiftX] - PupilWidth/2;
      const y = NominalEyeCenY + _eyeParams[Left][PupilShiftY] - _eyeParams[Left][PupilHeight]/2;
      const cv::Rect pupilRect(x,y,PupilWidth,_eyeParams[Left][PupilHeight]);
      cv::rectangle(faceImg, eyeRect, 0);
    }
    
    // Draw right pupil
    {
      const x = NominalRightEyeCenX + _eyeParams[Right][PupilShiftX] - PupilWidth/2;
      const y = NominalEyeCenY + _eyeParams[Right][PupilShiftY] - _eyeParams[Right][PupilHeight]/2;
      const cv::Rect pupilRect(x,y,PupilWidth,_eyeParams[Right][PupilHeight]);
      cv::rectangle(faceImg, eyeRect, 0);
    }
    
    // Rotate entire face
    if(_faceAngle_deg != 0) {
      int len = std::max(src.cols, src.rows);
      cv::Point2f pt(len/2., len/2.);
      cv::Mat R = cv::getRotationMatrix2D(cv::Point2f(HEIGHT/2, WIDTH/2), _faceAngle_deg, 1.0);
      cv::warpAffine(faceImg, faceImg, R, cv::Size(WIDTH, HEIGHT));
    }
    
    return faceImg;
  } // DrawFace()
  
  
  template<typename T>
  inline static T LinearBlendHelper(const T value1, const T value2, const float blendFraction)
  {
    if(value1 == value2) {
      // Special case, no math needed
      return value1;
    }
    
    T blendValue = static_cast<T>((1.f - blendFraction)*static_cast<float>(value1) +
                                  blendFraction*static_cast<float>(value2));
    return blendValue;
  }
  
  inline static ProceduralFace::Value BlendAngleHelper(const ProceduralFace::Value angle1,
                                                       const ProceduralFace::Value angle2,
                                                       const float blendFraction)
  {
    if(angle1 == angle2) {
      // Special case, no math needed
      return angle1;
    }
    
    const float angle1_rad = DEG_TO_RAD(static_cast<float>(angle1));
    const float angle2_rad = DEG_TO_RAD(static_cast<float>(angle2));
    
    const float x = LinearBlendHelper(std::cos(angle1_rad), std::cos(angle2_rad), blendFraction);
    const float y = LinearBlendHelper(std::sin(angle1_rad), std::sin(angle2_rad), blendFraction);
    
    return static_cast<ProceduralFace::Value>(RAD_TO_DEG(std::atan2(y,x)));
  }
  
  
  void ProceduralFace::Interpolate(const ProceduralFace& face1, const ProceduralFace& face2,
                                   const float blendFraction)
  {
    assert(blendFraction >= 0.f && blendFraction <= 1.f);
    
    // Special cases, no blending required:
    if(blendFraction == 0.f) {
      *this = face1;
      return;
    } else if(blendFraction == 1.f) {
      *this = face2;
      return;
    }
    
    for(int whichEye=0; whichEye < 2; ++whichEye)
    {
      for(int iParam=0; iParam < NumParameters; ++iParam)
      {
        if(iParam == BrowAngle) {
          // Special case: angle blending
          SetParameter(whichEye, iParam, BlendAngleHelper(face1.GetParameter(whichEye, iParam),
                                                          face2.GetParameter(whichEye, iParam),
                                                          blendFraction));
        } else {
          // Regular linear interpolation
          SetParameter(whichEye, iParam, LinearBlendHelper(face1.GetParameter(whichEye, iParam),
                                                           face2.GetParameter(whichEye, iParam),
                                                           blendFraction));
        }
      } // for each parameter
    } // for each eye
    
    SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(), face2.GetFaceAngle(), blendFraction));
    
  } // Interpolate()

} // namespace Cozmo
} // namespace Anki