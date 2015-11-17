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
    _faceCenter = {0,0};
    _faceScale = {1.f,1.f};
    _sentToRobot = false;
    _timestamp = 0;

    _eyeParams[Left].fill(0);
    _eyeParams[Right].fill(0);
    
    for(auto whichEye : {Left, Right}) {
      SetParameter(whichEye, Parameter::EyeScaleX, 1.f);
      SetParameter(whichEye, Parameter::EyeScaleY, 1.f);
      SetParameter(whichEye, Parameter::EyeCenterX, whichEye == Left ? WIDTH/3 : 2*WIDTH/3);
      SetParameter(whichEye, Parameter::EyeCenterY, HEIGHT/2);
      
      for(auto radius : {Parameter::UpperInnerRadiusX, Parameter::UpperInnerRadiusY,
        Parameter::UpperOuterRadiusX, Parameter::UpperOuterRadiusY,
        Parameter::LowerInnerRadiusX, Parameter::LowerInnerRadiusY,
        Parameter::LowerOuterRadiusX, Parameter::LowerOuterRadiusY})
      {
        SetParameter(whichEye, radius, 0.25f);
      }
    }
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
  
  SmallMatrix<2,3,f32> ProceduralFace::GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
                                                               f32 tX, f32 tY, f32 x0, f32 y0)
  {
    //
    // Create a 2x3 warp matrix which incorporates scale, rotation, and translation
    //    W = R * [scale_x    0   ] * [x - x0] + [x0] + [tx]
    //            [   0    scale_y]   [y - y0] + [y0] + [ty]
    //
    // So a given point gets scaled (first!) and then rotated around the given center
    // (x0,y0)and then translated by (tx,ty).
    //
    // Note: can't use cv::getRotationMatrix2D, b/c it only incorporates one
    // scale factor, not separate scaling in x and y. Otherwise, this is
    // exactly the same thing
    //
    f32 cosAngle = 1, sinAngle = 0;
    if(angleDeg != 0) {
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      cosAngle = std::cos(angleRad);
      sinAngle = std::sin(angleRad);
    }
    
    const f32 alpha_x = scaleX * cosAngle;
    const f32 beta_x  = scaleX * sinAngle;
    const f32 alpha_y = scaleY * cosAngle;
    const f32 beta_y  = scaleY * sinAngle;

    SmallMatrix<2, 3, float> W{
      alpha_x, beta_y,  (1.f - alpha_x)*x0 - beta_y*y0 + tX,
      -beta_x, alpha_y, beta_x*x0 + (1.f - alpha_y)*y0 + tY
    };

    return W;
  } // GetTransformationMatrix()

  
  void ProceduralFace::DrawEye(WhichEye whichEye, cv::Mat_<u8>& faceImg) const
  {
    assert(faceImg.rows == ProceduralFace::HEIGHT &&
           faceImg.cols == ProceduralFace::WIDTH);
    
    const s32 eyeWidth  = NominalEyeWidth;
    const s32 eyeHeight = NominalEyeHeight;
    
    // Left/right here will be in terms of the left eye. We will mirror to get
    // the right eye. So
    const s32 upLeftRadX  = std::round(GetParameter(whichEye, Parameter::UpperOuterRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 upLeftRadY  = std::round(GetParameter(whichEye, Parameter::UpperOuterRadiusY)*0.5f*static_cast<f32>(eyeHeight));
    const s32 lowLeftRadX = std::round(GetParameter(whichEye, Parameter::LowerOuterRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 lowLeftRadY = std::round(GetParameter(whichEye, Parameter::LowerOuterRadiusY)*0.5f*static_cast<f32>(eyeHeight));

    const s32 upRightRadX  = std::round(GetParameter(whichEye, Parameter::UpperInnerRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 upRightRadY  = std::round(GetParameter(whichEye, Parameter::UpperInnerRadiusY)*0.5f*static_cast<f32>(eyeHeight));
    const s32 lowRightRadX = std::round(GetParameter(whichEye, Parameter::LowerInnerRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 lowRightRadY = std::round(GetParameter(whichEye, Parameter::LowerInnerRadiusY)*0.5f*static_cast<f32>(eyeHeight));

    
    //
    // Compute eye and lid polygons:
    //
    std::vector<cv::Point> eyePoly, segment, lowerLidPoly, upperLidPoly;
    const s32 ellipseDelta = 10;
    
    // 1. Eye shape poly
    {
      // Upper right corner
      if(upRightRadX > 0 && upRightRadY > 0) {
        cv::ellipse2Poly(cv::Point(eyeWidth/2  - upRightRadX, -eyeHeight/2 + upRightRadY),
                         cv::Size(upRightRadX,upRightRadY), 0, 270, 360, ellipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.push_back({eyeWidth/2,-eyeHeight/2});
      }
      
      // Lower right corner
      if(lowRightRadX > 0 && lowRightRadY > 0) {
        cv::ellipse2Poly(cv::Point(eyeWidth/2 - lowRightRadX, eyeHeight/2 - lowRightRadY),
                         cv::Size(lowRightRadX,lowRightRadY), 0, 0, 90, ellipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.push_back({eyeWidth/2, eyeHeight/2});
      }
      
      // Lower left corner
      if(lowLeftRadX > 0 && lowLeftRadY > 0) {
        cv::ellipse2Poly(cv::Point(-eyeWidth/2  + lowLeftRadX, eyeHeight/2 - lowLeftRadY),
                         cv::Size(lowLeftRadX,lowLeftRadY), 0, 90, 180, ellipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.push_back({-eyeWidth/2, eyeHeight/2});
      }
    }
    
    // Upper left corner
    if(upLeftRadX > 0 && upLeftRadY > 0) {
      cv::ellipse2Poly(cv::Point(-eyeWidth/2 + upLeftRadX, -eyeHeight/2 + upLeftRadY),
                       cv::Size(upLeftRadX,upLeftRadY), 0, 180, 270, ellipseDelta, segment);
      eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
    } else {
      eyePoly.push_back({-eyeWidth/2,-eyeHeight/2});
    }
    
    // 2. Lower lid poly
    {
      const s32 lowerLidY = std::round(GetParameter(whichEye, Parameter::LowerLidY) * static_cast<f32>(eyeHeight));
      const f32 angleDeg = GetParameter(whichEye, Parameter::LowerLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const s32 yAngleAdj = -std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      lowerLidPoly = {
        { eyeWidth/2 + 1, eyeHeight/2 - lowerLidY - yAngleAdj}, // Upper right corner
        { eyeWidth/2 + 1, eyeHeight/2 + 1}, // Lower right corner
        {-eyeWidth/2 - 1, eyeHeight/2 + 1}, // Lower left corner
        {-eyeWidth/2 - 1, eyeHeight/2 - lowerLidY + yAngleAdj}, // Upper left corner
      };
      // Add bend:
      const f32 yRad = std::round(GetParameter(whichEye, Parameter::LowerLidBend) * static_cast<f32>(eyeHeight));
      if(yRad != 0) {
        const f32 xRad = std::round(static_cast<f32>(eyeWidth)*.5f / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, eyeHeight/2 - lowerLidY),
                         cv::Size(xRad,yRad), angleDeg, 180, 360, ellipseDelta, segment);
        ASSERT_NAMED(std::abs(segment.front().x - lowerLidPoly.back().x)<3 &&
                     std::abs(segment.front().y-lowerLidPoly.back().y)<3,
                     "First curved lower lid segment point not close to last lid poly point.");
        ASSERT_NAMED(std::abs(segment.back().x - lowerLidPoly.front().x)<3 &&
                     std::abs(segment.back().y - lowerLidPoly.front().y)<3,
                     "Last curved lower lid segment point not close to first lid poly point.");
        lowerLidPoly.insert(lowerLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    // 3. Upper lid poly
    {
      const s32 upperLidY = std::round(GetParameter(whichEye, Parameter::UpperLidY) * static_cast<f32>(eyeHeight));
      const f32 angleDeg = GetParameter(whichEye, Parameter::UpperLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const s32 yAngleAdj = -std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      upperLidPoly = {
        {-eyeWidth/2 - 1, -eyeHeight/2 + upperLidY + yAngleAdj}, // Lower left corner
        {-eyeWidth/2 - 1, -eyeHeight/2 - 1}, // Upper left corner
        { eyeWidth/2 + 1, -eyeHeight/2 - 1}, // Upper right corner
        { eyeWidth/2 + 1, -eyeHeight/2 + upperLidY - yAngleAdj}, // Lower right corner
      };
      // Add bend:
      const f32 yRad = std::round(GetParameter(whichEye, Parameter::UpperLidBend) * static_cast<f32>(eyeHeight));
      if(yRad != 0) {
        const f32 xRad = std::round(static_cast<f32>(eyeWidth)*.5f / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, -eyeHeight/2 + upperLidY),
                         cv::Size(xRad,yRad), angleDeg, 0, 180, ellipseDelta, segment);
        ASSERT_NAMED(std::abs(segment.front().x - upperLidPoly.back().x)<3 &&
                     std::abs(segment.front().y - upperLidPoly.back().y)<3,
                     "First curved upper lid segment point not close to last lid poly point.");
        ASSERT_NAMED(std::abs(segment.back().x - upperLidPoly.front().x)<3 &&
                     std::abs(segment.back().y - upperLidPoly.front().y)<3,
                     "Last curved upper lid segment point not close to first lid poly point.");
        upperLidPoly.insert(upperLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    // Apply rotation, translation, and scaling to the eye and lid polygons:
    SmallMatrix<2, 3, f32> W = GetTransformationMatrix(GetParameter(whichEye, Parameter::EyeAngle),
                                                       GetParameter(whichEye, Parameter::EyeScaleX),
                                                       GetParameter(whichEye, Parameter::EyeScaleY),
                                                       GetParameter(whichEye, Parameter::EyeCenterX),
                                                       GetParameter(whichEye, Parameter::EyeCenterY));
    
    for(auto poly : {&eyePoly, &lowerLidPoly, &upperLidPoly})
    {
      for(auto & point : *poly)
      {
        Point<2,f32> temp = W * Point<3,f32>{
          static_cast<f32>(whichEye == Left ? point.x : -point.x), static_cast<f32>(point.y), 1.f};
        point.x = std::round(temp.x());
        point.y = std::round(temp.y()) + _firstScanLine; // Move _with_ the scanlines
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
    
    // Apply whole-face params
    if(_faceAngle != 0 || !(_faceCenter == 0) || !(_faceScale == 1.f)) {
      
      SmallMatrix<2, 3, float> W = GetTransformationMatrix(_faceAngle,
                                                           _faceScale.x(), _faceScale.y(),
                                                           _faceCenter.x(), _faceCenter.y(),
                                                           static_cast<f32>(WIDTH)*0.5f,
                                                           static_cast<f32>(HEIGHT)*0.5f);
      
      cv::warpAffine(faceImg, faceImg, W.get_CvMatx_(), cv::Size(WIDTH, HEIGHT), cv::INTER_NEAREST);
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
        
        if(Parameter::EyeAngle == param) {
          SetParameter(whichEye, param, BlendAngleHelper(face1.GetParameter(whichEye, param),
                                                         face2.GetParameter(whichEye, param),
                                                         blendFraction));
        } else {
          SetParameter(whichEye, param, LinearBlendHelper(face1.GetParameter(whichEye, param),
                                                          face2.GetParameter(whichEye, param),
                                                          blendFraction));
        }

      } // for each parameter
    } // for each eye
    
    SetFaceAngle(BlendAngleHelper(face1.GetFaceAngle(), face2.GetFaceAngle(), blendFraction));
    SetFacePosition({LinearBlendHelper(face1.GetFacePosition().x(), face2.GetFacePosition().x(), blendFraction),
      LinearBlendHelper(face1.GetFacePosition().y(), face2.GetFacePosition().y(), blendFraction)});
    SetFaceScale({LinearBlendHelper(face1.GetFaceScale().x(), face2.GetFaceScale().x(), blendFraction),
      LinearBlendHelper(face1.GetFaceScale().y(), face2.GetFaceScale().y(), blendFraction)});
    
  } // Interpolate()

  bool ProceduralFace::GetNextBlinkFrame(TimeStamp_t& offset)
  {
    static ProceduralFace originalFace;
    
    enum class BlinkState : uint8_t {
      Closing,
      Closed,
      JustOpened,
      Opening
    };
    
    struct BlinkParams {
      Value height, width;
      TimeStamp_t t;
      BlinkState blinkState;
    };
    
    static const std::vector<BlinkParams> blinkParams{
      {.height = .85f, .width = 1.05f, .t = 33,  .blinkState = BlinkState::Closing},
      {.height = .6f,  .width = 1.2f,  .t = 33,  .blinkState = BlinkState::Closing},
      {.height = .1f,  .width = 2.5f,  .t = 33,  .blinkState = BlinkState::Closing},
      {.height = .05f, .width = 5.f,   .t = 33,  .blinkState = BlinkState::Closed},
      {.height = .15f, .width = 2.f,   .t = 33,  .blinkState = BlinkState::JustOpened},
      {.height = .7f,  .width = 1.2f,  .t = 33,  .blinkState = BlinkState::Opening},
      {.height = .9f,  .width = 1.f,   .t = 100, .blinkState = BlinkState::Opening}
    };
    
    static const std::vector<Parameter> lidParams{
      Parameter::LowerLidY, Parameter::LowerLidBend, Parameter::LowerLidAngle,
      Parameter::UpperLidY, Parameter::UpperLidBend, Parameter::UpperLidAngle,
    };
    
    static auto paramIter = blinkParams.begin();
    
    if(paramIter == blinkParams.end()) {
      // Set everything back to original params
      *this = originalFace;
      offset = 33;
      
      // Reset for next time
      paramIter = blinkParams.begin();
      // Let caller know this is the last blink frame
      return false;
      
    } else {
      if(paramIter == blinkParams.begin())
      {
        // Store the current pre-blink parameters before we muck with them
        originalFace = *this;
      }
      
      for(auto whichEye : {Left, Right}) {
        SetParameter(whichEye, Parameter::EyeScaleX,
                     originalFace.GetParameter(whichEye, Parameter::EyeScaleX) * paramIter->width);
        SetParameter(whichEye, Parameter::EyeScaleY,
                     originalFace.GetParameter(whichEye, Parameter::EyeScaleY) * paramIter->height);
      }
      offset = paramIter->t;
      
      switch(paramIter->blinkState)
      {
        case BlinkState::Closed:
        {
          SwitchInterlacing();
          
          // In case eyes are at different height, get the average height so the
          // blink line when completely closed is nice and horizontal
          const Value blinkHeight = (originalFace.GetParameter(Left,  Parameter::EyeCenterY) +
                                     originalFace.GetParameter(Right, Parameter::EyeCenterY))/2;
          
          // Zero out the lids so they don't interfere with the "closed" line
          for(auto whichEye : {Left, Right}) {
            SetParameter(whichEye, Parameter::EyeCenterY, blinkHeight);
            for(auto lidParam : lidParams) {
              SetParameter(whichEye, lidParam, 0);
            }
          }
          break;
        }
        case BlinkState::JustOpened:
        {
          // Restore eye heights and lids
          for(auto whichEye : {Left, Right}) {
            SetParameter(whichEye, Parameter::EyeCenterY,
                         originalFace.GetParameter(whichEye, Parameter::EyeCenterY));
            for(auto lidParam : lidParams) {
              SetParameter(whichEye, lidParam, originalFace.GetParameter(whichEye, lidParam));
            }
          }
          break;
        }
        default:
          break;
      }
      
      ++paramIter;
      
      // Let caller know there are more blink frames left, so keep calling
      return true;
    }
    
  } // GetNextBlinkFrame()
  
  void ProceduralFace::MimicHumanFace(const Vision::TrackedFace& face)
  {
    // using Face = Vision::TrackedFace;
    
    // TODO Implement mimicking here and use from BehaviorInteractWithFaces / BehaviorMimicFace
  }

} // namespace Cozmo
} // namespace Anki