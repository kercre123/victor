#include "anki/cozmo/basestation/proceduralFaceDrawer.h"

#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/basestation/scanlineDistorter.h"

#include "anki/vision/basestation/trackedFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace Anki {
namespace Cozmo {

  u8 ProceduralFaceDrawer::_firstScanLine = 0;
  
  SmallMatrix<2,3,f32> ProceduralFaceDrawer::GetTransformationMatrix(f32 angleDeg, f32 scaleX, f32 scaleY,
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

  
  void ProceduralFaceDrawer::DrawEye(const ProceduralFace& faceData, WhichEye whichEye, Vision::Image& faceImg,
                                     Rectangle<f32>& eyeBoundingBox)
  {
    DEV_ASSERT_MSG(faceImg.GetNumRows() == ProceduralFace::HEIGHT &&
                   faceImg.GetNumCols() == ProceduralFace::WIDTH,
                   "ProceduralFaceDrawer.DrawEye.ImageWrongSize",
                   "Face image should be %dx%d not %dx%d",
                   ProceduralFace::WIDTH, ProceduralFace::HEIGHT,
                   faceImg.GetNumCols(), faceImg.GetNumRows());
    
    const s32 eyeWidth  = ProceduralFace::NominalEyeWidth;
    const s32 eyeHeight = ProceduralFace::NominalEyeHeight;
    
    // Left/right here will be in terms of the left eye. We will mirror to get
    // the right eye. So
    const s32 upLeftRadX  = std::round(faceData.GetParameter(whichEye, Parameter::UpperOuterRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 upLeftRadY  = std::round(faceData.GetParameter(whichEye, Parameter::UpperOuterRadiusY)*0.5f*static_cast<f32>(eyeHeight));
    const s32 lowLeftRadX = std::round(faceData.GetParameter(whichEye, Parameter::LowerOuterRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 lowLeftRadY = std::round(faceData.GetParameter(whichEye, Parameter::LowerOuterRadiusY)*0.5f*static_cast<f32>(eyeHeight));

    const s32 upRightRadX  = std::round(faceData.GetParameter(whichEye, Parameter::UpperInnerRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 upRightRadY  = std::round(faceData.GetParameter(whichEye, Parameter::UpperInnerRadiusY)*0.5f*static_cast<f32>(eyeHeight));
    const s32 lowRightRadX = std::round(faceData.GetParameter(whichEye, Parameter::LowerInnerRadiusX)*0.5f*static_cast<f32>(eyeWidth));
    const s32 lowRightRadY = std::round(faceData.GetParameter(whichEye, Parameter::LowerInnerRadiusY)*0.5f*static_cast<f32>(eyeHeight));

    
    //
    // Compute eye and lid polygons:
    //
    std::vector<cv::Point> eyePoly, segment, lowerLidPoly, upperLidPoly;
    const s32 ellipseDelta = 10;
    
    // 1. Eye shape poly
    {
      // Upper right corner
      if(upRightRadX > 0 && upRightRadY > 0) {
       cv::ellipse2Poly(cv::Point(eyeWidth/2  - upRightRadX, -eyeHeight/2 + upRightRadY),cv::Size(upRightRadX,upRightRadY), 0, 270, 
          360, (int)ellipseDelta, segment);
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
      const s32 lowerLidY = std::round(faceData.GetParameter(whichEye, Parameter::LowerLidY) * static_cast<f32>(eyeHeight));
      const f32 angleDeg = faceData.GetParameter(whichEye, Parameter::LowerLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const s32 yAngleAdj = -std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      lowerLidPoly = {
        { eyeWidth/2 + 1, eyeHeight/2 - lowerLidY - yAngleAdj}, // Upper right corner
        { eyeWidth/2 + 1, eyeHeight/2 + 1}, // Lower right corner
        {-eyeWidth/2 - 1, eyeHeight/2 + 1}, // Lower left corner
        {-eyeWidth/2 - 1, eyeHeight/2 - lowerLidY + yAngleAdj}, // Upper left corner
      };
      // Add bend:
      const f32 yRad = std::round(faceData.GetParameter(whichEye, Parameter::LowerLidBend) * static_cast<f32>(eyeHeight));
      if(yRad != 0) {
        const f32 xRad = std::round(static_cast<f32>(eyeWidth)*.5f / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, eyeHeight/2 - lowerLidY),
                         cv::Size(xRad,yRad), angleDeg, 180, 360, ellipseDelta, segment);
        DEV_ASSERT(std::abs(segment.front().x - lowerLidPoly.back().x)<3 &&
                   std::abs(segment.front().y-lowerLidPoly.back().y)<3,
                   "First curved lower lid segment point not close to last lid poly point.");
        DEV_ASSERT(std::abs(segment.back().x - lowerLidPoly.front().x)<3 &&
                   std::abs(segment.back().y - lowerLidPoly.front().y)<3,
                   "Last curved lower lid segment point not close to first lid poly point.");
        lowerLidPoly.insert(lowerLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    // 3. Upper lid poly
    {
      const s32 upperLidY = std::round(faceData.GetParameter(whichEye, Parameter::UpperLidY) * static_cast<f32>(eyeHeight));
      const f32 angleDeg = faceData.GetParameter(whichEye, Parameter::UpperLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const s32 yAngleAdj = -std::round(static_cast<f32>(eyeWidth)*.5f * std::tan(angleRad));
      upperLidPoly = {
        {-eyeWidth/2 - 1, -eyeHeight/2 + upperLidY + yAngleAdj}, // Lower left corner
        {-eyeWidth/2 - 1, -eyeHeight/2 - 1}, // Upper left corner
        { eyeWidth/2 + 1, -eyeHeight/2 - 1}, // Upper right corner
        { eyeWidth/2 + 1, -eyeHeight/2 + upperLidY - yAngleAdj}, // Lower right corner
      };
      // Add bend:
      const f32 yRad = std::round(faceData.GetParameter(whichEye, Parameter::UpperLidBend) * static_cast<f32>(eyeHeight));
      if(yRad != 0) {
        const f32 xRad = std::round(static_cast<f32>(eyeWidth)*.5f / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, -eyeHeight/2 + upperLidY),
                         cv::Size(xRad,yRad), angleDeg, 0, 180, ellipseDelta, segment);
        DEV_ASSERT(std::abs(segment.front().x - upperLidPoly.back().x)<3 &&
                   std::abs(segment.front().y - upperLidPoly.back().y)<3,
                   "First curved upper lid segment point not close to last lid poly point");
        DEV_ASSERT(std::abs(segment.back().x - upperLidPoly.front().x)<3 &&
                   std::abs(segment.back().y - upperLidPoly.front().y)<3,
                   "Last curved upper lid segment point not close to first lid poly point");
        upperLidPoly.insert(upperLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    Point<2, Value> eyeCenter = (whichEye == WhichEye::Left) ?
                                Point<2, Value>(ProceduralFace::NominalLeftEyeX, ProceduralFace::NominalEyeY) :
                                Point<2, Value>(ProceduralFace::NominalRightEyeX, ProceduralFace::NominalEyeY);
    eyeCenter.x() += faceData.GetParameter(whichEye, Parameter::EyeCenterX);
    eyeCenter.y() += faceData.GetParameter(whichEye, Parameter::EyeCenterY);
    
    // Apply rotation, translation, and scaling to the eye and lid polygons:
    SmallMatrix<2, 3, f32> W = GetTransformationMatrix(faceData.GetParameter(whichEye, Parameter::EyeAngle),
                                                       faceData.GetParameter(whichEye, Parameter::EyeScaleX),
                                                       faceData.GetParameter(whichEye, Parameter::EyeScaleY),
                                                       eyeCenter.x(),
                                                       eyeCenter.y());
    
    Point2f upperLeft(ProceduralFace::WIDTH, ProceduralFace::HEIGHT);
    Point2f bottomRight(0.f,0.f);
    
    for(auto poly : {&eyePoly, &lowerLidPoly, &upperLidPoly})
    {
      for(auto & point : *poly)
      {
        Point<2,f32> temp = W * Point<3,f32>{
          static_cast<f32>(whichEye == WhichEye::Left ? point.x : -point.x), static_cast<f32>(point.y), 1.f};
        point.x = std::round(temp.x());
        point.y = std::round(temp.y()) + _firstScanLine; // Move _with_ the scanlines
        
        upperLeft.x()   = std::min(upperLeft.x(),   (f32)std::floor(point.x));
        bottomRight.x() = std::max(bottomRight.x(), (f32)std::ceil(point.x));
        upperLeft.y()   = std::min(upperLeft.y(),   (f32)std::floor(point.y));
        bottomRight.y() = std::max(bottomRight.y(), (f32)std::ceil(point.y));
      }
    }
    
    eyeBoundingBox = Rectangle<f32>(upperLeft, bottomRight);

    // Draw eye
    cv::fillConvexPoly(faceImg.get_CvMat_(), eyePoly, 255, 4);

    // Add distortion noise
    auto scanlineDistorter = faceData.GetScanlineDistorter();
    if(nullptr != scanlineDistorter)
    {
      scanlineDistorter->AddOffNoise(W, eyeHeight, eyeWidth, faceImg);
    }
    
    // Black out lids
    if(!upperLidPoly.empty()) {
      cv::fillConvexPoly(faceImg.get_CvMat_(), upperLidPoly, 0, 4);
    }
    if(!lowerLidPoly.empty()) {
      cv::fillConvexPoly(faceImg.get_CvMat_(), lowerLidPoly, 0, 4);
    }
    
  } // DrawEye()
  
  Vision::Image ProceduralFaceDrawer::DrawFace(const ProceduralFace& faceData)
  {
    Vision::Image faceImg(ProceduralFace::HEIGHT, ProceduralFace::WIDTH);
    
    faceImg.FillWith(0);
    
    Rectangle<f32> leftBBox, rightBBox;
    DrawEye(faceData, WhichEye::Left,  faceImg, leftBBox);
    DrawEye(faceData, WhichEye::Right, faceImg, rightBBox);
    
    s32 rowMin = ProceduralFace::HEIGHT-1, rowMax = 0;
    
    // Apply whole-face params
    if(faceData.GetFaceAngle() != 0 || !(faceData.GetFacePosition() == 0) || !(faceData.GetFaceScale() == 1.f)) {
      
      SmallMatrix<2, 3, float> W = GetTransformationMatrix(faceData.GetFaceAngle(),
                                                           faceData.GetFaceScale() .x(), faceData.GetFaceScale() .y(),
                                                           faceData.GetFacePosition().x(), faceData.GetFacePosition().y(),
                                                           static_cast<f32>(ProceduralFace::WIDTH)*0.5f,
                                                           static_cast<f32>(ProceduralFace::HEIGHT)*0.5f);
      
      cv::warpAffine(faceImg.get_CvMat_(), faceImg.get_CvMat_(), W.get_CvMatx_(),
                     cv::Size(ProceduralFace::WIDTH, ProceduralFace::HEIGHT), cv::INTER_NEAREST);
      
      std::array<Quad2f,2> leftRightQuads;
      leftBBox.GetQuad(leftRightQuads[0]);
      rightBBox.GetQuad(leftRightQuads[1]);
      
      for(const auto& quad : leftRightQuads)
      {
        for(const auto& pt : quad) {
          const Point2f& warpedPoint = W * Point3f(pt.x(), pt.y(), 1.f);
          rowMin = std::min(rowMin, (s32)std::floor(warpedPoint.y()));
          rowMax = std::max(rowMax, (s32)std::ceil(warpedPoint.y()));
        }
      }
      
    }
    else
    {
      rowMin = std::min(leftBBox.GetY(), rightBBox.GetY());
      rowMax = std::max(leftBBox.GetYmax(), rightBBox.GetYmax());
    }
    
    // Just to be safe:
    rowMin = Util::Clamp(rowMin, 0, ProceduralFace::HEIGHT-1);
    rowMax = Util::Clamp(rowMax, 0, ProceduralFace::HEIGHT-1);

    // Apply interlacing / scanlines at the end
    for(s32 i=_firstScanLine; i<ProceduralFace::HEIGHT; i+=2) {
      memset(faceImg.GetRow(i), 0, ProceduralFace::WIDTH);
    }
    
    // Distort the scanlines
    auto scanlineDistorter = faceData.GetScanlineDistorter();
    if(nullptr != scanlineDistorter)
    {
      if(rowMax > rowMin)
      {
        const f32 scale = 1.f / (rowMax - rowMin);
        for(s32 row=rowMin; row < rowMax; ++row)
        {
          const f32 eyeFrac = (row - rowMin) * scale;
          const s32 shift = scanlineDistorter->GetEyeDistortionAmount(eyeFrac);
          
          if(shift < 0)
          {
            u8* faceImg_row = faceImg.GetRow(row);
            memmove(faceImg_row, faceImg_row-shift, ProceduralFace::WIDTH+shift);
            memset(faceImg_row+ProceduralFace::WIDTH+shift, 0, -shift);
          }
          else if(shift > 0)
          {
            u8* faceImg_row = faceImg.GetRow(row);
            memmove(faceImg_row+shift, faceImg_row, ProceduralFace::WIDTH-shift);
            memset(faceImg_row, 0, shift);
          }
        }
      }
    }
    
    return faceImg;
  } // DrawFace()

  bool ProceduralFaceDrawer::GetNextBlinkFrame(ProceduralFace& faceData, TimeStamp_t& offset)
  {
    static ProceduralFace originalFace;
    
    enum class BlinkState : uint8_t {
      Closing,
      Closed,
      JustOpened,
      Opening
    };
    
    struct BlinkParams {
      ProceduralFace::Value height, width;
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
      faceData = originalFace;
      offset = 33;
      
      // Reset for next time
      paramIter = blinkParams.begin();
      // Let caller know this is the last blink frame
      return false;
      
    } else {
      if(paramIter == blinkParams.begin())
      {
        // Store the current pre-blink parameters before we muck with them
        originalFace = faceData;
      }
      
      for(auto whichEye : {WhichEye::Left, WhichEye::Right}) {
        faceData.SetParameter(whichEye, Parameter::EyeScaleX,
                     originalFace.GetParameter(whichEye, Parameter::EyeScaleX) * paramIter->width);
        faceData.SetParameter(whichEye, Parameter::EyeScaleY,
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
          const ProceduralFace::Value blinkHeight = (originalFace.GetParameter(WhichEye::Left,  Parameter::EyeCenterY) +
                                     originalFace.GetParameter(WhichEye::Right, Parameter::EyeCenterY))/2;
          
          // Zero out the lids so they don't interfere with the "closed" line
          for(auto whichEye : {WhichEye::Left, WhichEye::Right}) {
            faceData.SetParameter(whichEye, Parameter::EyeCenterY, blinkHeight);
            for(auto lidParam : lidParams) {
              faceData.SetParameter(whichEye, lidParam, 0);
            }
          }
          break;
        }
        case BlinkState::JustOpened:
        {
          // Restore eye heights and lids
          for(auto whichEye : {WhichEye::Left, WhichEye::Right}) {
            faceData.SetParameter(whichEye, Parameter::EyeCenterY,
                         originalFace.GetParameter(whichEye, Parameter::EyeCenterY));
            for(auto lidParam : lidParams) {
              faceData.SetParameter(whichEye, lidParam, originalFace.GetParameter(whichEye, lidParam));
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

} // namespace Cozmo
} // namespace Anki
