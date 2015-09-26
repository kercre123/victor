#include "anki/cozmo/basestation/proceduralFace.h"

#include "anki/vision/basestation/trackedFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Anki {
namespace Cozmo {

  //const ProceduralFace::Value ProceduralFace::MaxFaceAngle = 30.f; // Not sure why I can't set this one here
  u8 ProceduralFace::_firstScanLine = 0;
  
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
  
  inline const s32 GetScaledValue(ProceduralFace::Value value, s32 min, s32 mid, s32 max)
  {
    if(value == 0) {
      return mid;
    } else if(value < 0) {
      return (value + 1.f)*static_cast<ProceduralFace::Value>(mid-min) + min;
    } else {
      assert(value > 0);
      return value*static_cast<ProceduralFace::Value>(max-mid) + mid;
    }
  }
  
  inline s32 ProceduralFace::GetBrowHeight(const s32 eyeHeightPix, const Value browCenY) const
  {
    return GetScaledValue(-browCenY, 1, NominalEyeCenY-std::round(static_cast<f32>(eyeHeightPix)*0.5f));// + _firstScanLine;
  }

  inline void RoundedRectangleHelper(cv::Mat& roi,
                                     const u8 fillValue,
                                     const s32 numCornerPixels)
  {
    assert(fillValue == 0 || fillValue == 255);
    if(!roi.empty())
    {
      roi.setTo(fillValue);
      
      if(numCornerPixels > 0) {
        const u8 cornerValue = (fillValue == 0 ? 255 : 0);
        
        // Add a few pixels to the four corners
        u8* topLine0 = roi.ptr(0);
        u8* topLine1 = roi.ptr(1);
        u8* btmLine0 = roi.ptr(roi.rows-1);
        u8* btmLine1 = roi.ptr(roi.rows-2);
        for(s32 j=0; j<numCornerPixels; ++j) {
          topLine0[j] = topLine1[j] = cornerValue;
          btmLine0[j] = btmLine1[j] = cornerValue;
          topLine0[roi.cols-1-j] = topLine1[roi.cols-1-j] = cornerValue;
          btmLine0[roi.cols-1-j] = btmLine1[roi.cols-1-j] = cornerValue;
        }
      }
    }
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
    RoundedRectangleHelper(roi, 255, 3);
    
    // Black out pupil
    roi = faceImg(pupilRect & imgRect);
    RoundedRectangleHelper(roi, 0, 2);
    
    // Eyebrow: (quadrilateral)
    const f32 browAngleRad = DEG_TO_RAD(static_cast<f32>(GetScaledValue(GetParameter(whichEye, Parameter::BrowAngle),
                                                                        -MaxBrowAngle, MaxBrowAngle)));
    const float cosAngle = std::cos(browAngleRad);
    const float sinAngle = std::sin(browAngleRad);
    
    const s32 browXPosPix = GetScaledValue(GetParameter(whichEye, Parameter::BrowCenX),
                                           -eyeWidthPix/2, eyeWidthPix/2) + NominalEyeCenX;
    const s32 browYPosPix = GetBrowHeight(eyeHeightPix, GetParameter(whichEye, Parameter::BrowCenY));

    const s32 browHalfLength = GetScaledValue(GetParameter(whichEye, Parameter::BrowLength), 0, MidBrowLengthPix, MaxBrowLengthPix)/2;
    
    if(browHalfLength > 0) {
      const cv::Point leftPoint(browXPosPix-std::round(static_cast<f32>(browHalfLength)*cosAngle),
                                browYPosPix-std::round(static_cast<f32>(browHalfLength)*sinAngle));
      const cv::Point rightPoint(browXPosPix+std::round(static_cast<f32>(browHalfLength)*cosAngle),
                                 browYPosPix+std::round(static_cast<f32>(browHalfLength)*sinAngle));
      
      const cv::Point leftPointBtm(leftPoint.x,   leftPoint.y + 3);
      const cv::Point rightPointBtm(rightPoint.x, rightPoint.y + 3);
      
      const std::vector<std::vector<cv::Point> > eyebrow = {
        {leftPoint, rightPoint, rightPointBtm, leftPointBtm}
      };
      cv::fillConvexPoly(faceImg, eyebrow, 255, 4);
    }
    
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

    // Apply interlacing / scanlines at the end
    // TODO: Switch odd/even periodically to avoid burn-in?
    for(s32 i=_firstScanLine; i<HEIGHT; i+=2) {
      faceImg.row(i).setTo(0);
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
                                   float blendFraction, bool usePupilSaccades)
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
        } else if(usePupilSaccades && (param == Parameter::PupilCenX || param == Parameter::PupilCenY)) {
          // Special case: pupils saccade rather than moving slowly. So immediately
          // jump to new position halfway between the two frames
          SetParameter(whichEye, param, (blendFraction <= .5f ?
                                         face1.GetParameter(whichEye, param) :
                                         face2.GetParameter(whichEye, param)));
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
    // Figure out the height needed to keep the eyebrows from moving:
    for(auto whichEye : {Left, Right}) {
      // Current eye height in pixels:
      const s32 eyeHeightPix = GetScaledValue(GetParameter(whichEye, Parameter::EyeHeight),
                                              MinEyeHeightPix, MaxEyeHeightPix);
      // Current eyebrow Y position in pixels
      const s32 browYPosPix = GetBrowHeight(eyeHeightPix, GetParameter(whichEye, Parameter::BrowCenY));
      
      // Get corresponding parameter value for that same brow height, when
      // eye is closed (as it will be when we blink)
      SetParameter(whichEye, Parameter::BrowCenY, 2.f*(1.f-static_cast<f32>(browYPosPix)/static_cast<f32>(ProceduralFace::NominalEyeCenY-1))-1.f);
    }
    
    // Close eyes
    SetParameter(Left, Parameter::EyeHeight, -1.f);
    SetParameter(Right, Parameter::EyeHeight, -1.f);

    SwitchInterlacing();
  }
  
  void ProceduralFace::MimicHumanFace(const Vision::TrackedFace& face)
  {
    // using Face = Vision::TrackedFace;
    
    // TODO Implement mimicking here and use from BehaviorInteractWithFaces / BehaviorMimicFace
  }

} // namespace Cozmo
} // namespace Anki