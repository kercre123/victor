#include "anki/cozmo/basestation/proceduralFace.h"

#include "anki/vision/basestation/trackedFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Anki {
namespace Cozmo {

  //const ProceduralFace::Value ProceduralFace::MaxFaceAngle = 30.f; // Not sure why I can't set this one here
  
  const cv::Rect ProceduralFace::imgRect(0,0,ProceduralFace::WIDTH, ProceduralFace::HEIGHT);
  
  ProceduralFace::ProceduralFace()
  {
    Reset();
  }
  
  void ProceduralFace::Reset()
  {
    _faceAngle = 0;
    _sentToRobot = false;
    _timestamp = 0;
    _firstScanLine = 0;

    _eyeParams[Left].fill(0);
    _eyeParams[Right].fill(0);
  }

  

  inline const s32 GetScaledValue(ProceduralFace::Value value, s32 min, s32 max)
  {
    // Input is [-1,1]. Make [0,1]
    value += 1;
    value *= .5f;
    
    return static_cast<s32>(value * static_cast<ProceduralFace::Value>(max-min)) + min;
  }

  void ProceduralFace::DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const
  {
    assert(faceImg.rows == ProceduralFace::HEIGHT &&
           faceImg.cols == ProceduralFace::WIDTH);
    
    const s32 NominalEyeCenX = (whichEye == Left ? NominalLeftEyeCenX : NominalRightEyeCenX);
    
    const s32 eyeWidthPix = GetScaledValue(GetParameter(whichEye, Parameter::EyeWidth),
                                            MinEyeWidthPix, MaxEyeWidthPix);
    
    const s32 eyeHeightPix = GetScaledValue(GetParameter(whichEye, Parameter::EyeHeight),
                                            MinEyeHeightPix, MaxEyeHeightPix);
    
    const s32 xEye = NominalEyeCenX - eyeWidthPix/2;
    const s32 yEye = NominalEyeCenY - eyeHeightPix/2;
    const cv::Rect eyeRect(xEye, yEye, eyeWidthPix, eyeHeightPix);
    
    const s32 pupilHeightPix = GetScaledValue(GetParameter(whichEye, Parameter::PupilHeight), 0, eyeHeightPix);
    const s32 pupilWidthPix  = GetScaledValue(GetParameter(whichEye, Parameter::PupilWidth), 0, eyeWidthPix);
    const s32 pupilCenXPix   = GetParameter(whichEye, Parameter::PupilCenX) * static_cast<f32>(eyeWidthPix/2);
    const s32 pupilCenYPix   = GetParameter(whichEye, Parameter::PupilCenY) * static_cast<f32>(eyeHeightPix/2);
    const s32 xPupil = NominalEyeCenX + pupilCenXPix - pupilWidthPix/2;
    const s32 yPupil = NominalEyeCenY + pupilCenYPix - pupilHeightPix/2;
    const cv::Rect pupilRect(xPupil,yPupil,pupilWidthPix,pupilHeightPix);
    
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
    
    if(!ScanlinesAsPostProcess) {
      // Set every other row to 0 to get interlaced appearance
      for(int i=_firstScanLine; i<roi.rows; i+=2) {
        roi.row(i).setTo(0);
      }
    }
    
    // Black out pupil
    roi = faceImg(pupilRect & imgRect);
    roi.setTo(0);
    
    // Eyebrow:
    const f32 browAngleRad = DEG_TO_RAD(static_cast<f32>(GetScaledValue(GetParameter(whichEye, Parameter::BrowAngle),
                                                                        -MaxBrowAngle, MaxBrowAngle)));
    const float cosAngle = std::cos(browAngleRad);
    const float sinAngle = std::sin(browAngleRad);
    
    const s32 browXPosPix = GetScaledValue(GetParameter(whichEye, Parameter::BrowCenX),
                                           -eyeWidthPix/2, eyeWidthPix/2) + NominalEyeCenX;
    const s32 browYPosPix = GetScaledValue(-GetParameter(whichEye, Parameter::BrowCenY),
                                            1, NominalEyeCenY-eyeHeightPix/2);

    const cv::Point leftPoint(browXPosPix-EyebrowHalfLength*cosAngle,  browYPosPix-EyebrowHalfLength*sinAngle);
    const cv::Point rightPoint(browXPosPix+EyebrowHalfLength*cosAngle, browYPosPix+EyebrowHalfLength*sinAngle);
    cv::line(faceImg, leftPoint, rightPoint, 255, EyebrowThickness, 4);
    
  } // DrawEye()
  
  cv::Mat_<u8> ProceduralFace::GetFace() const
  {
    cv::Mat_<u8> faceImg(HEIGHT, WIDTH);
    faceImg.setTo(0);
    
    //DrawEyeBrow(Left, faceImg);
    //DrawEyeBrow(Right, faceImg);
    DrawEye(Left, faceImg);
    DrawEye(Right, faceImg);
    
    // Rotate entire face
    if(_faceAngle != 0) {
      // Note negative angle to get mirroring
      const f32 faceAngleDeg = GetScaledValue(_faceAngle, -MaxFaceAngle, MaxFaceAngle);
      cv::Mat R = cv::getRotationMatrix2D(cv::Point2f(WIDTH/2,HEIGHT/2), faceAngleDeg, 1.0);
      cv::warpAffine(faceImg, faceImg, R, cv::Size(WIDTH, HEIGHT), cv::INTER_NEAREST);
    }
    
    if(ScanlinesAsPostProcess) {
      // Apply interlacing / scanlines at the end
      // TODO: Switch odd/even periodically to avoid burn-in?
      for(s32 i=_firstScanLine; i<HEIGHT; i+=2) {
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
                                   float blendFraction)
  {
    assert(blendFraction >= 0.f && blendFraction <= 1.f);
    
    // Special cases, no blending required:
    if(blendFraction == 0.f) {
      // Preserve original timestamp
      TimeStamp_t t = GetTimeStamp();
      *this = face1;
      this->SetTimeStamp(t);
      return;
    } else if(blendFraction == 1.f) {
      // Preserve original timestamp
      TimeStamp_t t = GetTimeStamp();
      *this = face2;
      this->SetTimeStamp(t);
      return;
    }
    
    for(int iWhichEye=0; iWhichEye < 2; ++iWhichEye)
    {
      WhichEye whichEye = static_cast<WhichEye>(iWhichEye);
      
      for(int iParam=0; iParam < static_cast<int>(Parameter::NumParameters); ++iParam)
      {
        Parameter param = static_cast<Parameter>(iParam);
        
        if(param == Parameter::BrowAngle) {
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
  
  
  void ProceduralFace::Blink()
  {
    SetParameter(Left, Parameter::EyeHeight, -1.f);
    SetParameter(Right, Parameter::EyeHeight, -1.f);
    SwitchInterlacing();
  }
  
  void ProceduralFace::MimicHumanFace(const Vision::TrackedFace& face)
  {
    using Face = Vision::TrackedFace;
    
    // TODO Implement mimicking here and use from BehaviorLookForFaces / BehaviorMimicFace
  }

} // namespace Cozmo
} // namespace Anki