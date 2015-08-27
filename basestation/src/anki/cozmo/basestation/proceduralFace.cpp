#include "proceduralFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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
  
  inline static void DrawEye(cv::Mat_<u8>& faceImg, const cv::Rect& eyeRect, const cv::Rect& pupilRect, bool interlace)
  {
    static const cv::Rect imgRect(0,0,ProceduralFace::WIDTH, ProceduralFace::HEIGHT);
    
    // Fill eye
    cv::Mat_<u8> roi = faceImg(eyeRect & imgRect);
    roi.setTo(255);
    
    // Remove a few pixels from the four corners
    const s32 NumCornerPixels = 3; // TODO: Make this a static const parameter?
    u8* topLine = roi.ptr(0);
    u8* btmLine = roi.ptr(roi.rows-1);
    for(s32 j=0; j<NumCornerPixels; ++j) {
      topLine[j] = 0;
      btmLine[j] = 0;
      topLine[roi.cols-1-j] = 0;
      btmLine[roi.cols-1-j] = 0;
    }
    
    if(interlace) {
      // Set every other row to 0 to get interlaced appearance
      for(int i=0; i<roi.rows; i+=2) {
        roi.row(i).setTo(0);
      }
    }
    
    // Black out pupil
    roi = faceImg(pupilRect & imgRect);
    roi.setTo(0);
  }
  
  cv::Mat_<u8> ProceduralFace::GetFace() const
  {
    cv::Mat_<u8> faceImg(HEIGHT, WIDTH);
    faceImg.setTo(0);
    
    // Draw left eyebrow
    {
      const float cosAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Left][BrowAngle])));
      const float sinAngle = std::sin(DEG_TO_RAD(static_cast<float>(_eyeParams[Left][BrowAngle])));
      const Value x = NominalLeftEyeCenX   + _eyeParams[Left][BrowShiftX];
      const Value y = NominalEyebrowHeight + _eyeParams[Left][BrowShiftY];
      const cv::Point leftPoint(x-EyebrowHalfLength*cosAngle, y-EyebrowHalfLength*sinAngle);
      const cv::Point rightPoint(x+EyebrowHalfLength*cosAngle, y+EyebrowHalfLength*sinAngle);
      cv::line(faceImg, leftPoint, rightPoint, 255, EyebrowThickness, 4);
    }
    
    // Draw right eyebrow
    {
      const float cosAngle = std::cos(DEG_TO_RAD(static_cast<float>(_eyeParams[Right][BrowAngle])));
      const float sinAngle = std::sin(DEG_TO_RAD(static_cast<float>(_eyeParams[Right][BrowAngle])));
      const Value x = NominalRightEyeCenX  + _eyeParams[Right][BrowShiftX];
      const Value y = NominalEyebrowHeight + _eyeParams[Right][BrowShiftY];
      const cv::Point leftPoint(x-EyebrowHalfLength*cosAngle, y-EyebrowHalfLength*sinAngle);
      const cv::Point rightPoint(x+EyebrowHalfLength*cosAngle, y+EyebrowHalfLength*sinAngle);
      cv::line(faceImg, leftPoint, rightPoint, 255, EyebrowThickness, 4);
    }
    
    // Draw left eye
    {
      const Value xEye = NominalLeftEyeCenX-EyeWidth/2;
      const Value yEye = NominalEyeCenY - _eyeParams[Left][EyeHeight]/2;
      const cv::Rect eyeRect(xEye, yEye, EyeWidth, _eyeParams[Left][EyeHeight]);

      const Value xPupil = NominalLeftEyeCenX + _eyeParams[Left][PupilShiftX] - PupilWidth/2;
      const Value yPupil = NominalEyeCenY + _eyeParams[Left][PupilShiftY] - _eyeParams[Left][PupilHeight]/2;
      const cv::Rect pupilRect(xPupil,yPupil,PupilWidth,_eyeParams[Left][PupilHeight]);
      
      DrawEye(faceImg, eyeRect, pupilRect, !ScanlinesAsPostProcess);
    }
    
    // Draw right eye
    {
      const Value xEye = NominalRightEyeCenX-EyeWidth/2;
      const Value yEye = NominalEyeCenY - _eyeParams[Right][EyeHeight]/2;
      const cv::Rect eyeRect(xEye, yEye, EyeWidth, _eyeParams[Right][EyeHeight]);
      
      const Value xPupil = NominalRightEyeCenX + _eyeParams[Right][PupilShiftX] - PupilWidth/2;
      const Value yPupil = NominalEyeCenY + _eyeParams[Right][PupilShiftY] - _eyeParams[Right][PupilHeight]/2;
      const cv::Rect pupilRect(xPupil,yPupil,PupilWidth,_eyeParams[Right][PupilHeight]);
      
      DrawEye(faceImg, eyeRect, pupilRect, !ScanlinesAsPostProcess);
    }
    
    // Rotate entire face
    if(_faceAngle_deg != 0) {
      // Note negative angle to get mirroring
      cv::Mat R = cv::getRotationMatrix2D(cv::Point2f(WIDTH/2,HEIGHT/2), _faceAngle_deg, 1.0);
      cv::warpAffine(faceImg, faceImg, R, cv::Size(WIDTH, HEIGHT), cv::INTER_NEAREST);
    }
    
    if(ScanlinesAsPostProcess) {
      // Apply interlacing / scanlines at the end
      // TODO: Switch odd/even periodically to avoid burn-in?
      for(s32 i=0; i<HEIGHT; i+=2) {
        faceImg.row(i).setTo(0);
      }
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
    
    for(int iWhichEye=0; iWhichEye < 2; ++iWhichEye)
    {
      WhichEye whichEye = static_cast<WhichEye>(iWhichEye);
      
      for(int iParam=0; iParam < NumParameters; ++iParam)
      {
        Parameter param = static_cast<Parameter>(iParam);
        
        if(param == BrowAngle) {
          // Special case: angle blending
          SetParameter(whichEye, param, BlendAngleHelper(face1.GetParameter(whichEye, param),
                                                          face2.GetParameter(whichEye, param),
                                                          blendFraction));
        } else {
          // Regular linear interpolation
          SetParameter(whichEye, param, LinearBlendHelper(face1.GetParameter(whichEye, param),
                                                           face2.GetParameter(whichEye, param),
                                                           blendFraction));
        }
      } // for each parameter
    } // for each eye
    
    SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(), face2.GetFaceAngle(), blendFraction));
    
  } // Interpolate()

} // namespace Cozmo
} // namespace Anki