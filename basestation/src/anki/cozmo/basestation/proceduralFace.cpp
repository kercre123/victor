#include "anki/cozmo/basestation/proceduralFace.h"

#include "anki/vision/basestation/trackedFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Anki {
namespace Cozmo {

  const ProceduralFace::Value ProceduralFace::MaxFaceAngle = 30.f; // Not sure why I can't set this one here
  
  ProceduralFace::ProceduralFace()
  {
    Reset();
  }
  
  void ProceduralFace::Reset()
  {
    _faceAngle_deg = 0;
    _sentToRobot = false;
    _timestamp = 0;
    
    for(int iWhichEye=0; iWhichEye<2; ++iWhichEye) {
      const WhichEye whichEye = static_cast<WhichEye>(iWhichEye);
      
      _eyeParams[whichEye].fill(0);
      SetParameter(whichEye, Parameter::EyeWidth, NominalEyeWidth);
      SetParameter(whichEye, Parameter::EyeHeight, NominalEyeHeight);
      SetParameter(whichEye, Parameter::PupilHeightFraction, NominalPupilHeightFrac);
      SetParameter(whichEye, Parameter::PupilWidthFraction, NominalPupilWidthFrac);
    }
  }

  
  void ProceduralFace::SetFaceAngle(Value angle_deg) {
    _faceAngle_deg = std::max(-MaxFaceAngle, std::min(MaxFaceAngle, angle_deg));
  }
  
  const ProceduralFace::ValueLimits& ProceduralFace::GetLimits(Parameter param)
  {
    static const std::map<Parameter, ValueLimits> LUT = {
      {Parameter::EyeHeight, {0.125f*static_cast<f32>(HEIGHT), 0.667f*static_cast<f32>(HEIGHT)}},
      {Parameter::EyeWidth,  {0.125f*static_cast<f32>(WIDTH), 0.4f*static_cast<f32>(WIDTH)}},
      {Parameter::PupilHeightFraction, {0.25f, 0.7f}},
      {Parameter::PupilWidthFraction,  {0.25f, 0.75f}},
    };
    
    auto iter = LUT.find(param);
    if(iter == LUT.end()) {
      static ValueLimits NoLimits(std::numeric_limits<Value>::min(),
                                  std::numeric_limits<Value>::max());
      return NoLimits;
    } else {
      return iter->second;
    }
  }
  
  void ProceduralFace::DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const
  {
    static const cv::Rect imgRect(0,0,ProceduralFace::WIDTH, ProceduralFace::HEIGHT);
    assert(faceImg.rows == ProceduralFace::HEIGHT &&
           faceImg.cols == ProceduralFace::WIDTH);
    
    const Value EyeCenX = (whichEye == Left ? NominalLeftEyeCenX : NominalRightEyeCenX);
    
    const Value xEye = EyeCenX-GetParameter(whichEye, Parameter::EyeWidth)/2;
    const Value yEye = NominalEyeCenY - GetParameter(whichEye, Parameter::EyeHeight)/2;
    const cv::Rect eyeRect(xEye, yEye, GetParameter(whichEye, Parameter::EyeWidth),
                           GetParameter(whichEye, Parameter::EyeHeight));
    
    const Value pupilHeight = GetParameter(whichEye, Parameter::PupilHeightFraction)*GetParameter(whichEye,Parameter::EyeHeight);
    const Value pupilWidth  = GetParameter(whichEye, Parameter::PupilWidthFraction)*GetParameter(whichEye,Parameter::EyeWidth);
    const Value xPupil = EyeCenX + GetParameter(whichEye,Parameter::PupilShiftX) - pupilWidth/2;
    const Value yPupil = NominalEyeCenY + GetParameter(whichEye,Parameter::PupilShiftY) - pupilHeight*0.5f;
    const cv::Rect pupilRect(xPupil,yPupil,pupilWidth,pupilHeight);
    
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
      for(int i=0; i<roi.rows; i+=2) {
        roi.row(i).setTo(0);
      }
    }
    
    // Black out pupil
    roi = faceImg(pupilRect & imgRect);
    roi.setTo(0);
  } // DrawEye()
  
  void ProceduralFace::DrawEyeBrow(WhichEye whichEye, cv::Mat_<u8> &faceImg) const
  {
    const Value EyeCenX = (whichEye==Left ? NominalLeftEyeCenX : NominalRightEyeCenX);
    const float cosAngle = std::cos(DEG_TO_RAD(static_cast<float>(GetParameter(whichEye,Parameter::BrowAngle))));
    const float sinAngle = std::sin(DEG_TO_RAD(static_cast<float>(GetParameter(whichEye,Parameter::BrowAngle))));
    const Value x = EyeCenX   + GetParameter(whichEye,Parameter::BrowShiftX);
    const Value y = NominalEyebrowHeight + GetParameter(whichEye,Parameter::BrowShiftY);
    const cv::Point leftPoint(x-EyebrowHalfLength*cosAngle, y-EyebrowHalfLength*sinAngle);
    const cv::Point rightPoint(x+EyebrowHalfLength*cosAngle, y+EyebrowHalfLength*sinAngle);
    cv::line(faceImg, leftPoint, rightPoint, 255, EyebrowThickness, 4);
  }
  
  cv::Mat_<u8> ProceduralFace::GetFace() const
  {
    cv::Mat_<u8> faceImg(HEIGHT, WIDTH);
    faceImg.setTo(0);
    
    DrawEyeBrow(Left, faceImg);
    DrawEyeBrow(Right, faceImg);
    DrawEye(Left, faceImg);
    DrawEye(Right, faceImg);
    
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
  
  
  void ProceduralFace::Blink(const ProceduralFace& face, float fraction)
  {
    // TODO: Implement blinking
  }
  
  void ProceduralFace::MimicHumanFace(const Vision::TrackedFace& face)
  {
    // using Face = Vision::TrackedFace;
    
    // TODO Implement mimicking here and use from BehaviorLookForFaces / BehaviorMimicFace
  }

} // namespace Cozmo
} // namespace Anki