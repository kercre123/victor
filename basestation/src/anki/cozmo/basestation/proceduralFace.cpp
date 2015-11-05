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
  
  void ProceduralFace::DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const
  {
    assert(faceImg.rows == ProceduralFace::HEIGHT &&
           faceImg.cols == ProceduralFace::WIDTH);
    
    const s32 eyeWidth  = NominalEyeWidth;
    const s32 eyeHeight = NominalEyeHeight;
    
    const Value maxCornerRadiusPix = std::min(eyeWidth, eyeHeight)/2;
    
    Value LeftRadius, RightRadius;
    if(whichEye == Left) {
      LeftRadius     = GetScaledValue(GetParameter(whichEye, Parameter::OuterRadius), 0, maxCornerRadiusPix);
      RightRadius    = GetScaledValue(GetParameter(whichEye, Parameter::InnerRadius), 0, maxCornerRadiusPix);
    } else {
      LeftRadius     = GetScaledValue(GetParameter(whichEye, Parameter::InnerRadius), 0, maxCornerRadiusPix);
      RightRadius    = GetScaledValue(GetParameter(whichEye, Parameter::OuterRadius), 0, maxCornerRadiusPix);
    }
    
    //
    // Compute eye and lid polygons:
    //
    std::vector<cv::Point> eyePoly, segment, lowerLidPoly, upperLidPoly;
    
    // 1. Eye shape poly
    // Right side:
    const s32 ellipseDelta = 10;
    if(RightRadius > 0) {
      // Upper right corner
      cv::Point radiusCenter( eyeWidth/2  - RightRadius,
                             -eyeHeight/2 + RightRadius);
      cv::ellipse2Poly(radiusCenter, cv::Size(RightRadius,RightRadius), 270, 0, 90, ellipseDelta, segment);
      eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      
      // Lower right corner
      radiusCenter.y = eyeHeight/2 - RightRadius;
      cv::ellipse2Poly(radiusCenter, cv::Size(RightRadius,RightRadius), 0, 0, 90, ellipseDelta, segment);
      eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
    } else {
      eyePoly.push_back({eyeWidth/2,-eyeHeight/2});
      eyePoly.push_back({eyeWidth/2, eyeHeight/2});
    }
    
    // Left side
    if(LeftRadius > 0) {
      // Lower left corner
      cv::Point radiusCenter(-eyeWidth/2  + LeftRadius,
                              eyeHeight/2 - LeftRadius);
      cv::ellipse2Poly(radiusCenter, cv::Size(LeftRadius,LeftRadius), 90, 0, 90, ellipseDelta, segment);
      eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      
      // Upper left corner
      radiusCenter.y = -eyeHeight/2 + LeftRadius;
      cv::ellipse2Poly(radiusCenter, cv::Size(LeftRadius,LeftRadius), 180, 0, 90, ellipseDelta, segment);
      eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
    } else {
      eyePoly.push_back({-eyeWidth/2, eyeHeight/2});
      eyePoly.push_back({-eyeWidth/2,-eyeHeight/2});
    }
    
    // 2. Lower lid poly
    const s32 lowerLidY = std::round(GetParameter(whichEye, Parameter::LowerLidY) * static_cast<f32>(eyeHeight));
    if(lowerLidY > 0) {
      const f32 angleRad = DEG_TO_RAD(GetParameter(whichEye, Parameter::LowerLidAngle));
      const s32 yAngleAdj = std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      lowerLidPoly = {
        {-eyeWidth/2 - 1, eyeHeight/2 + 1}, // Lower left corner
        { eyeWidth/2 + 1, eyeHeight/2 + 1}, // Lower right corner
        { eyeWidth/2 + 1, eyeHeight/2 - lowerLidY - yAngleAdj}, // Upper right corner
        {-eyeWidth/2 - 1, eyeHeight/2 - lowerLidY + yAngleAdj}, // Upper left corner
      };
    }
    
    // 3. Upper lid poly
    const s32 upperLidY = std::round(GetParameter(whichEye, Parameter::UpperLidY) * static_cast<f32>(eyeHeight));
    if(upperLidY > 0) {
      const f32 angleRad = DEG_TO_RAD(GetParameter(whichEye, Parameter::UpperLidAngle));
      const s32 yAngleAdj = std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      upperLidPoly = {
        {-eyeWidth/2 - 1, -eyeHeight/2 - 1}, // Upper left corner
        { eyeWidth/2 + 1, -eyeHeight/2 - 1}, // Upper right corner
        { eyeWidth/2 + 1, -eyeHeight/2 + upperLidY - yAngleAdj}, // Lower right corner
        {-eyeWidth/2 - 1, -eyeHeight/2 + upperLidY + yAngleAdj}, // Lower left corner
      };
    }
    
    // Apply rotation, translation, and scaling to the eye and lid polygons:
    
    // TODO: apply transformation to poly before filling it
    const f32 angleRad = DEG_TO_RAD(GetParameter(whichEye, Parameter::EyeAngle));
    const f32 cosTheta = std::cos(angleRad);
    const f32 sinTheta = std::sin(angleRad);
    const f32 scaleX = GetParameter(whichEye, Parameter::EyeScaleX);
    const f32 scaleY = GetParameter(whichEye, Parameter::EyeScaleY);
    SmallMatrix<2, 3, f32> W{
       scaleX * cosTheta, scaleX * sinTheta, GetParameter(whichEye, Parameter::EyeCenterX),
      -scaleY * sinTheta, scaleY * cosTheta, GetParameter(whichEye, Parameter::EyeCenterY)
    };
    
    for(auto poly : {&eyePoly, &lowerLidPoly, &upperLidPoly})
    {
      for(auto & point : *poly)
      {
        Point<2,f32> temp = W * Point<3,f32>{static_cast<f32>(point.x), static_cast<f32>(point.y), 1.f};
        point.x = std::round(temp.x());
        point.y = std::round(temp.y());
      }
    }

    // Draw eye
    cv::fillConvexPoly(faceImg, eyePoly, 255, 4);
    
    // Black out lids
    if(!upperLidPoly.empty()) {
      cv::fillConvexPoly(faceImg, upperLidPoly, 0, 4);
    }
    if(!lowerLidPoly.empty()) {
      cv::fillConvexPoly(faceImg, lowerLidPoly, 0, 4);
    }
    
  } // DrawEye()
  
  cv::Mat_<u8> ProceduralFace::GetFace() const
  {
    cv::Mat_<u8> faceImg(HEIGHT, WIDTH);
    faceImg.setTo(0);
    
    DrawEye(Left, faceImg);
    DrawEye(Right, faceImg);
    
    // Rotate entire face
    if(_faceAngle != 0) {
      // Create a 2x3 warp matrix which incorporates scale, rotation, and translation
      //    W = [scale*R T]

      // Note negative angle to get mirroring
      cv::Mat W = cv::getRotationMatrix2D(cv::Point2f(WIDTH/2,HEIGHT/2), _faceAngle, _faceScale);
      W.at<Value>(0,2) += _faceCenter.x();
      W.at<Value>(1,2) += _faceCenter.y();
      
      cv::warpAffine(faceImg, faceImg, W, cv::Size(WIDTH, HEIGHT), cv::INTER_NEAREST);
    }

    // Apply interlacing / scanlines at the end
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
        
        SetParameter(whichEye, param, LinearBlendHelper(face1.GetParameter(whichEye, param),
                                                        face2.GetParameter(whichEye, param),
                                                        blendFraction));

      } // for each parameter
    } // for each eye
    
    SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(), face2.GetFaceAngle(), blendFraction));
    
  } // Interpolate()
  
  
  void ProceduralFace::Blink()
  {
    // Smash eyes down to min height and max width to create a horizontal line
    for(auto whichEye : {Left, Right}) {
      SetParameter(whichEye, Parameter::EyeScaleY, 1);
      SetParameter(whichEye, Parameter::EyeScaleX, WIDTH/2);
    }

    SwitchInterlacing();
  }
  
  void ProceduralFace::MimicHumanFace(const Vision::TrackedFace& face)
  {
    // using Face = Vision::TrackedFace;
    
    // TODO Implement mimicking here and use from BehaviorInteractWithFaces / BehaviorMimicFace
  }

} // namespace Cozmo
} // namespace Anki