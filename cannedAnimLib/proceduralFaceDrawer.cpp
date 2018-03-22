#include "cannedAnimLib/proceduralFaceDrawer.h"
#include "cannedAnimLib/scanlineDistorter.h"

#include "coretech/common/engine/array2d_impl.h"

//#include "coretech/vision/engine/trackedFace.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/math/math.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {
  
  #define CONSOLE_GROUP "ProceduralFace"

#if PROCEDURALFACE_NOISE_FEATURE
  static const s32 kNumNoiseImages = 7;

  CONSOLE_VAR_RANGED(s32, kProcFace_NoiseNumFrames,     "ProceduralFace", 0, 0, kNumNoiseImages);
  CONSOLE_VAR_RANGED(f32, kProcFace_NoiseMinLightness,  "ProceduralFace", 1.f-0.15f, 0.f, 2.f); // replaces kProcFace_NoiseFraction
  CONSOLE_VAR_RANGED(f32, kProcFace_NoiseMaxLightness,  "ProceduralFace", 1.f+0.15f, 0.f, 2.f); // replaces kProcFace_NoiseFraction

  CONSOLE_VAR_EXTERN(s32, kProcFace_NoiseNumFrames);
#endif
    
  CONSOLE_VAR(bool,       kProcFace_UseAntiAliasedLines,        CONSOLE_GROUP, true); // Only affects OpenCV drawing, not post-smoothing
  CONSOLE_VAR_RANGED(f32, kProcFace_EyeLightnessMultiplier,     CONSOLE_GROUP, 1.f, 0.f, 10.f);

  CONSOLE_VAR(bool,       kProcFace_HotspotRender,              CONSOLE_GROUP, true); // Render glow
  CONSOLE_VAR_RANGED(f32, kProcFace_HotspotFalloff,             CONSOLE_GROUP, 0.85f, 0.05f, 1.f);

  CONSOLE_VAR_RANGED(f32, kProcFace_GlowSizeMultiplier,         CONSOLE_GROUP, 1.f, 0.f, 1.f);
  CONSOLE_VAR_RANGED(f32, kProcFace_GlowLightnessMultiplier,    CONSOLE_GROUP, 1.f, 0.f, 10.f);
  CONSOLE_VAR(bool,       kProcFace_GlowGaussianFilter,         CONSOLE_GROUP, false); // Gausssian or boxfilter for glow

  CONSOLE_VAR_RANGED(s32, kProcFace_AntiAliasingSize,           CONSOLE_GROUP, 5, 0, 63); // full image antialiasing
  CONSOLE_VAR(bool,       kProcFace_AntiAliasingGaussianFilter, CONSOLE_GROUP, false); // simpler box filter if not

  static void VictorFaceRenderer(ConsoleFunctionContextRef context)
  {
    kProcFace_HotspotRender = true;
    kProcFace_AntiAliasingSize = 5;
  }
  static void CozmoFaceRenderer(ConsoleFunctionContextRef context)
  {
    kProcFace_HotspotRender = false;
    kProcFace_AntiAliasingSize = 0;
  }
  CONSOLE_FUNC(VictorFaceRenderer, CONSOLE_GROUP);
  CONSOLE_FUNC(CozmoFaceRenderer, CONSOLE_GROUP);

  #undef CONSOLE_GROUP

  // Initialize static vars
  Vision::Image ProceduralFaceDrawer::_glowImg;
  Vision::Image ProceduralFaceDrawer::_eyeShape;

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


  // Based on Taylor series expansion with N terms
  inline static f32 fastExp(f32 x)
  {
#   define NUM_FAST_EXP_TERMS 2
    
    if(x < -(f32)(2*NUM_FAST_EXP_TERMS))
    {
      // Things get numerically unstable for very negative inputs x. Value is basically zero anyway.
      return 0.f;
    }
#   if NUM_FAST_EXP_TERMS==2
    x = 1.f + (x * 0.25f);  // Constant here is 1/(2^N)
    x *= x; x *= x;         // Number of multiplies here is also N
#   elif NUM_FAST_EXP_TERMS==3
    x = 1.f + (x * 0.125f); // Constant here is 1/(2^N)
    x *= x; x *= x; x *= x; // Number of multiplies here is also N
#   else
#   error Unsupported number of terms for fastExp()
#   endif
    
    return x;
    
#   undef NUM_FAST_EXP_TERMS
  }
  
#if PROCEDURALFACE_NOISE_FEATURE
  inline Array2d<f32> CreateNoiseImage(const Util::RandomGenerator& rng)
  {
    Array2d<f32> noiseImg(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
    f32* noiseImg_i = noiseImg.GetRow(0);
    for(s32 j=0; j<noiseImg.GetNumElements(); ++j)
    {
      noiseImg_i[j] = rng.RandDblInRange(kProcFace_NoiseMinLightness, kProcFace_NoiseMaxLightness);
    }
    return noiseImg;
  }

  const Array2d<f32>& ProceduralFaceDrawer::GetNoiseImage(const Util::RandomGenerator& rng)
  {
    // NOTE: Since this is called separately for each eye, this looks better if we use an odd number of images
    static_assert(kNumNoiseImages % 2 == 1, "Use odd number of noise images");
    static std::array<Array2d<f32>, kNumNoiseImages> kNoiseImages{{
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
      CreateNoiseImage(rng),
    }};

    kProcFace_NoiseMinLightness = Anki::Util::Clamp(kProcFace_NoiseMinLightness, 0.f, kProcFace_NoiseMaxLightness);
    kProcFace_NoiseMaxLightness = Anki::Util::Clamp(kProcFace_NoiseMaxLightness, kProcFace_NoiseMinLightness, 2.f);

    static f32 kProcFace_NoiseMinLightness_old = kProcFace_NoiseMinLightness;
    static f32 kProcFace_NoiseMaxLightness_old = kProcFace_NoiseMaxLightness;
    if(kProcFace_NoiseMinLightness_old != kProcFace_NoiseMinLightness || kProcFace_NoiseMaxLightness_old != kProcFace_NoiseMaxLightness) {
      for(auto& currentNoiseImage : kNoiseImages) {
        Array2d<f32> noiseImg = CreateNoiseImage(rng);
        std::swap(currentNoiseImage, noiseImg);
      }

      kProcFace_NoiseMinLightness_old = kProcFace_NoiseMinLightness;
      kProcFace_NoiseMaxLightness_old = kProcFace_NoiseMaxLightness;
    }
    
    if(kProcFace_NoiseNumFrames == 0) {
      return kNoiseImages[0];
    } else {
      // Cycle circularly through the set of noise images
      static s32 index = 0;
      index = (index + 1) % kProcFace_NoiseNumFrames;
      return kNoiseImages[index];
    }
  }
#endif // PROCEDURALFACE_NOISE_FEATURE

  void ProceduralFaceDrawer::DrawEye(const ProceduralFace& faceData, WhichEye whichEye, const Util::RandomGenerator& rng,
                                     Vision::ImageRGB& faceImg, Rectangle<f32>& eyeBoundingBox)
  {
    DEV_ASSERT_MSG(faceImg.GetNumRows() == ProceduralFace::HEIGHT &&
                   faceImg.GetNumCols() == ProceduralFace::WIDTH,
                   "ProceduralFaceDrawer.DrawEye.ImageWrongSize",
                   "Face image should be %dx%d not %dx%d",
                   ProceduralFace::WIDTH, ProceduralFace::HEIGHT,
                   faceImg.GetNumCols(), faceImg.GetNumRows());
    
    const s32 eyeWidth  = ProceduralFace::NominalEyeWidth;
    const s32 eyeHeight = ProceduralFace::NominalEyeHeight;
    const f32 halfEyeWidth  = 0.5f*eyeWidth;
    const f32 halfEyeHeight = 0.5f*eyeHeight;
    
    // Left/right here will be in terms of the left eye. We will mirror to get
    // the right eye. So
    const f32 upLeftRadX  = faceData.GetParameter(whichEye, Parameter::UpperOuterRadiusX)*halfEyeWidth;
    const f32 upLeftRadY  = faceData.GetParameter(whichEye, Parameter::UpperOuterRadiusY)*halfEyeHeight;
    const f32 lowLeftRadX = faceData.GetParameter(whichEye, Parameter::LowerOuterRadiusX)*halfEyeWidth;
    const f32 lowLeftRadY = faceData.GetParameter(whichEye, Parameter::LowerOuterRadiusY)*halfEyeHeight;

    const f32 upRightRadX  = faceData.GetParameter(whichEye, Parameter::UpperInnerRadiusX)*halfEyeWidth;
    const f32 upRightRadY  = faceData.GetParameter(whichEye, Parameter::UpperInnerRadiusY)*halfEyeHeight;
    const f32 lowRightRadX = faceData.GetParameter(whichEye, Parameter::LowerInnerRadiusX)*halfEyeWidth;
    const f32 lowRightRadY = faceData.GetParameter(whichEye, Parameter::LowerInnerRadiusY)*halfEyeHeight;
    
    //
    // Compute eye and lid polygons:
    //
    std::vector<cv::Point> eyePoly, segment, lowerLidPoly, upperLidPoly;
    const s32 kEllipseDelta = 5;
    const s32 kLineType = (kProcFace_UseAntiAliasedLines ? cv::LINE_AA : cv::LINE_8);
    
    // 1. Eye shape poly
    {
      ANKI_CPU_PROFILE("EyeShapePoly");
      // Upper right corner
      if(upRightRadX > 0 && upRightRadY > 0) {
       cv::ellipse2Poly(cv::Point(std::round(halfEyeWidth  - upRightRadX), std::round(-halfEyeHeight + upRightRadY)),
                        cv::Size(upRightRadX,upRightRadY), 0, 270, 360, kEllipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.emplace_back(halfEyeWidth,-halfEyeHeight);
      }
      
      // Lower right corner
      if(lowRightRadX > 0 && lowRightRadY > 0) {
        cv::ellipse2Poly(cv::Point(std::round(halfEyeWidth - lowRightRadX), std::round(halfEyeHeight - lowRightRadY)),
                         cv::Size(lowRightRadX,lowRightRadY), 0, 0, 90, kEllipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.emplace_back(halfEyeWidth, halfEyeHeight);
      }
      
      // Lower left corner
      if(lowLeftRadX > 0 && lowLeftRadY > 0) {
       cv::ellipse2Poly(cv::Point(std::round(-halfEyeWidth  + lowLeftRadX), std::round(halfEyeHeight - lowLeftRadY)),
                        cv::Size(lowLeftRadX,lowLeftRadY), 0, 90, 180, kEllipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.emplace_back(-halfEyeWidth, halfEyeHeight);
      }
    
      // Upper left corner
      if(upLeftRadX > 0 && upLeftRadY > 0) {
        cv::ellipse2Poly(cv::Point(std::round(-halfEyeWidth + upLeftRadX), std::round(-halfEyeHeight + upLeftRadY)),
                        cv::Size(upLeftRadX,upLeftRadY), 0, 180, 270, kEllipseDelta, segment);
        eyePoly.insert(eyePoly.end(), segment.begin(), segment.end());
      } else {
        eyePoly.emplace_back(-halfEyeWidth,-halfEyeHeight);
      }
    }
    
    // 2. Lower lid poly
    {
      ANKI_CPU_PROFILE("LowerLidPoly");
      const f32 lowerLidY = faceData.GetParameter(whichEye, Parameter::LowerLidY) * static_cast<f32>(eyeHeight);
      const f32 angleDeg = faceData.GetParameter(whichEye, Parameter::LowerLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const f32 yAngleAdj = -halfEyeWidth * std::tan(angleRad);
      lowerLidPoly = {
        {(s32)std::round( halfEyeWidth + 1.f), (s32)std::round(halfEyeHeight - lowerLidY - yAngleAdj)}, // Upper right corner
        {(s32)std::round( halfEyeWidth + 1.f), (s32)std::round(halfEyeHeight + 1.f)}, // Lower right corner
        {(s32)std::round(-halfEyeWidth - 1.f), (s32)std::round(halfEyeHeight + 1.f)}, // Lower left corner
        {(s32)std::round(-halfEyeWidth - 1.f), (s32)std::round(halfEyeHeight - lowerLidY + yAngleAdj)}, // Upper left corner
      };
      // Add bend:
      const f32 yRad = faceData.GetParameter(whichEye, Parameter::LowerLidBend) * static_cast<f32>(eyeHeight);
      if(yRad != 0) {
        const f32 xRad = std::round(halfEyeWidth / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, std::round(halfEyeHeight - lowerLidY)),
                         cv::Size(xRad,yRad), angleDeg, 180, 360, kEllipseDelta, segment);
        DEV_ASSERT(std::abs(segment.front().x - lowerLidPoly.back().x)<3 &&
                   std::abs(segment.front().y - lowerLidPoly.back().y)<3,
                   "First curved lower lid segment point not close to last lid poly point.");
        DEV_ASSERT(std::abs(segment.back().x - lowerLidPoly.front().x)<3 &&
                   std::abs(segment.back().y - lowerLidPoly.front().y)<3,
                   "Last curved lower lid segment point not close to first lid poly point.");
        lowerLidPoly.insert(lowerLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    // 3. Upper lid poly
    {
      ANKI_CPU_PROFILE("UpperLidPoly");
      const f32 upperLidY = faceData.GetParameter(whichEye, Parameter::UpperLidY) * static_cast<f32>(eyeHeight);
      const f32 angleDeg = faceData.GetParameter(whichEye, Parameter::UpperLidAngle);
      const f32 angleRad = DEG_TO_RAD(angleDeg);
      const f32 yAngleAdj = -halfEyeWidth * std::tan(angleRad);
      upperLidPoly = {
        {(s32)std::round(-halfEyeWidth - 1.f), (s32)std::round(-halfEyeHeight + upperLidY + yAngleAdj)}, // Lower left corner
        {(s32)std::round(-halfEyeWidth - 1.f), (s32)std::round(-halfEyeHeight - 1.f)}, // Upper left corner
        {(s32)std::round( halfEyeWidth + 1.f), (s32)std::round(-halfEyeHeight - 1.f)}, // Upper right corner
        {(s32)std::round( halfEyeWidth + 1.f), (s32)std::round(-halfEyeHeight + upperLidY - yAngleAdj)}, // Lower right corner
      };
      // Add bend:
      const f32 yRad = faceData.GetParameter(whichEye, Parameter::UpperLidBend) * static_cast<f32>(eyeHeight);
      if(yRad != 0) {
        const f32 xRad = std::round(halfEyeWidth / std::cos(angleRad));
        cv::ellipse2Poly(cv::Point(0, std::round(-halfEyeHeight + upperLidY)),
                         cv::Size(xRad,yRad), angleDeg, 0, 180, kEllipseDelta, segment);
        DEV_ASSERT(std::abs(segment.front().x - upperLidPoly.back().x)<3 &&
                   std::abs(segment.front().y - upperLidPoly.back().y)<3,
                   "First curved upper lid segment point not close to last lid poly point");
        DEV_ASSERT(std::abs(segment.back().x - upperLidPoly.front().x)<3 &&
                   std::abs(segment.back().y - upperLidPoly.front().y)<3,
                   "Last curved upper lid segment point not close to first lid poly point");
        upperLidPoly.insert(upperLidPoly.end(), segment.begin(), segment.end());
      }
    }
    
    ANKI_CPU_PROFILE_START(prof_getMat, "GetMatrices");
    Point<2, Value> eyeCenter = (whichEye == WhichEye::Left) ?
                                 Point<2, Value>(ProceduralFace::GetNominalLeftEyeX(), ProceduralFace::GetNominalEyeY()) :
                                 Point<2, Value>(ProceduralFace::GetNominalRightEyeX(), ProceduralFace::GetNominalEyeY());
    eyeCenter.x() += faceData.GetParameter(whichEye, Parameter::EyeCenterX);
    eyeCenter.y() += faceData.GetParameter(whichEye, Parameter::EyeCenterY);
    
    // Apply rotation, translation, and scaling to the eye and lid polygons:
    const SmallMatrix<2, 3, f32> W = GetTransformationMatrix(faceData.GetParameter(whichEye, Parameter::EyeAngle),
                                                             faceData.GetParameter(whichEye, Parameter::EyeScaleX),
                                                             faceData.GetParameter(whichEye, Parameter::EyeScaleY),
                                                             eyeCenter.x(),
                                                             eyeCenter.y());

    const Value glowFraction = Util::Min(1.f, Util::Max(-1.f, kProcFace_GlowSizeMultiplier * faceData.GetParameter(whichEye, Parameter::GlowSize)));
    const Value glowLightness = kProcFace_GlowLightnessMultiplier * faceData.GetParameter(whichEye, Parameter::GlowLightness);
    const SmallMatrix<2, 3, f32> W_glow = GetTransformationMatrix(faceData.GetParameter(whichEye, Parameter::EyeAngle),
                                                                  (1+glowFraction) * faceData.GetParameter(whichEye, Parameter::EyeScaleX),
                                                                  (1+glowFraction) * faceData.GetParameter(whichEye, Parameter::EyeScaleY),
                                                                  eyeCenter.x(),
                                                                  eyeCenter.y());                                                        
    ANKI_CPU_PROFILE_STOP(prof_getMat);
    
    // Initialize bounding box corners at their opposite extremes. We will figure out their
    // true locations as we loop over the eyePoly below.
    Point2f upperLeft(ProceduralFace::WIDTH, ProceduralFace::HEIGHT);
    Point2f bottomRight(0.f,0.f);
    
    ANKI_CPU_PROFILE_START(prof_applyMat, "ApplyMatrices");
    // Warp the poly and the glow. Use the warped glow (which is a larger shape) to compute
    // the overall eye bounding box 
    for(auto & point : eyePoly)
    {
      const Point<3,f32> pointF32{
        static_cast<f32>(whichEye == WhichEye::Left ? point.x : -point.x), static_cast<f32>(point.y), 1.f
      };

      Point<2,f32> temp = W * pointF32;
      point.x = std::round(temp.x());
      point.y = std::round(temp.y());

      // Use glow warp to figure out larger bounding box which contains glow as well
      temp = W_glow * pointF32;
      upperLeft.x()   = std::min(upperLeft.x(),   (f32)std::floor(temp.x()));
      bottomRight.x() = std::max(bottomRight.x(), (f32)std::ceil(temp.x()));
      upperLeft.y()   = std::min(upperLeft.y(),   (f32)std::floor(temp.y()));
      bottomRight.y() = std::max(bottomRight.y(), (f32)std::ceil(temp.y()));
    }
    
    // Warp the lids
    for(auto poly : {&lowerLidPoly, &upperLidPoly})
    {
      for(auto & point : *poly)
      {
        const Point<3,f32> pointF32{
          static_cast<f32>(whichEye == WhichEye::Left ? point.x : -point.x), static_cast<f32>(point.y), 1.f
        };

        Point<2,f32> temp = W * pointF32;
        point.x = std::round(temp.x());
        point.y = std::round(temp.y());
      }
    }
    ANKI_CPU_PROFILE_STOP(prof_applyMat);

    // Make sure the upper left and bottom right points are in bounds (note that we loop over
    // pixels below *inclusive* of the bottom right point, so we use HEIGHT/WIDTH-1)
    upperLeft.x() = std::max(0.f, upperLeft.x());
    upperLeft.y() = std::max(0.f, upperLeft.y());
    // Push bottom-right co-ordinate to avoid un-antialiased edges
    bottomRight.x() = std::min((f32)(ProceduralFace::WIDTH-1), bottomRight.x()+1);
    bottomRight.y() = std::min((f32)(ProceduralFace::HEIGHT-1), bottomRight.y()+1);

    // Create the bounding that we're returning from the upperLeft and bottomRight points
    eyeBoundingBox = Rectangle<f32>(upperLeft, bottomRight);
    
    // Draw eye
    _eyeShape.Allocate(faceImg.GetNumRows(), faceImg.GetNumCols());   
    _eyeShape.FillWith(0);                                  
    cv::fillConvexPoly(_eyeShape.get_CvMat_(), eyePoly, 255, kLineType);

    // Black out lids
    if(!upperLidPoly.empty()) {
      if(faceData.GetParameter(whichEye, Parameter::UpperLidBend) < 0.f) {
        const cv::Point* pts[1] = { &upperLidPoly[0] };
        int npts[1] = { (int)upperLidPoly.size() };
        cv::fillPoly(_eyeShape.get_CvMat_(), pts, npts, 1, 0, kLineType);
      } else {
        cv::fillConvexPoly(_eyeShape.get_CvMat_(), upperLidPoly, 0, kLineType);
      }
    }
    if(!lowerLidPoly.empty()) {
      if(faceData.GetParameter(whichEye, Parameter::LowerLidBend) < 0.f) {
        const cv::Point* pts[1] = { &lowerLidPoly[0] };
        int npts[1] = { (int)lowerLidPoly.size() };
        cv::fillPoly(_eyeShape.get_CvMat_(), pts, npts, 1, 0, kLineType);
      } else {
        cv::fillConvexPoly(_eyeShape.get_CvMat_(), lowerLidPoly, 0, kLineType);
      }
    }
    
    const f32 eyeScaleX = faceData.GetParameter(whichEye, Parameter::EyeScaleX);
    const f32 eyeScaleY = faceData.GetParameter(whichEye, Parameter::EyeScaleY);
    
    if(eyeWidth > 0 && eyeHeight > 0 && eyeScaleX > 0 && eyeScaleY > 0)
    {
      const f32 scaledEyeWidth  = eyeScaleX * static_cast<f32>(eyeWidth);
      const f32 scaledEyeHeight = eyeScaleY * static_cast<f32>(eyeHeight);
      
      s32 glowCenX = eyeCenter.x();
      s32 glowCenY = eyeCenter.y();
      
      // The hotspot center params leave the hot spot at the eye center if zero. If non-zero,
      // they shift left/right/up/down where a magnitude of 1.0 moves the center to the extreme
      // edge of the eye shape
      const Value hotSpotCenterX = faceData.GetParameter(whichEye, Parameter::HotSpotCenterX);
      const Value hotSpotCenterY = faceData.GetParameter(whichEye, Parameter::HotSpotCenterY);
      if(!Util::IsNearZero(hotSpotCenterX))
      {
        glowCenX += hotSpotCenterX * (0.5f*scaledEyeWidth);
      }
      if(!Util::IsNearZero(hotSpotCenterY))
      {
        glowCenY += hotSpotCenterY * (0.5f*scaledEyeHeight);
      }
      
      // Inner Glow = the brighter glow at the center of the eye that falls off radially towards the edge of the eye
      // Outer Glow = the "halo" effect around the outside of the eye shape
      // Add inner glow to the eye shape, before we compute the outer glow, so that boundaries conditions match.
      if(kProcFace_HotspotRender) {
        ANKI_CPU_PROFILE("RenderInnerOuterGlow");
        const f32 sigmaX = kProcFace_HotspotFalloff*scaledEyeWidth;
        const f32 sigmaY = kProcFace_HotspotFalloff*scaledEyeHeight;
        const f32 invInnerGlowSigmaX_sq = 1.f / (2.f * (sigmaX*sigmaX));
        const f32 invInnerGlowSigmaY_sq = 1.f / (2.f * (sigmaY*sigmaY));
        for(s32 i=upperLeft.y(); i<=bottomRight.y(); ++i)
        {
          DEV_ASSERT_MSG(i>=0 && i<faceImg.GetNumRows(), "ProceduralFaceDrawer.DrawEye.BadRow", "%d", i);
          
          u8* eyeShape_i = _eyeShape.GetRow(i);
          for(s32 j=upperLeft.x(); j<=bottomRight.x(); ++j)
          {
            DEV_ASSERT_MSG(j>=0 && j<faceImg.GetNumCols(), "ProceduralFaceDrawer.DrawEye.BadCol", "%d", j);
            
            u8& eyeValue  = eyeShape_i[j];
            const bool insideEye = (eyeValue > 0);
            
            if(insideEye)
            {
              // TODO: Use a separate approximation helper or LUT to get falloff
              const s32 dx = j-glowCenX;
              const s32 dy = i-glowCenY;
              const f32 falloff = fastExp(-f32(dx*dx)*invInnerGlowSigmaX_sq - f32(dy*dy)*invInnerGlowSigmaY_sq);
              DEV_ASSERT_MSG(Util::InRange(falloff, 0.f, 1.f), "ProceduralFaceDrawer.DrawEye.BadInnerGlowFalloffValue", "%f", falloff);

              eyeValue = Util::numeric_cast_clamped<u8>(std::round(static_cast<f32>(eyeValue) * falloff));
            }
          }
        }
      }
      
      Rectangle<s32> eyeBoundingBoxS32(upperLeft.CastTo<s32>(), bottomRight.CastTo<s32>());
      Vision::Image eyeShapeROI = _eyeShape.GetROI(eyeBoundingBoxS32);
      
      // Compute glow from the final eye shape (after lids are drawn)
      _glowImg.Allocate(faceImg.GetNumRows(), faceImg.GetNumCols());
      _glowImg.FillWith(0);

      if(Util::IsFltGTZero(glowLightness) && Util::IsFltGTZero(glowFraction)) {
        ANKI_CPU_PROFILE("GlowRender");
        Vision::Image glowImgROI  = _glowImg.GetROI(eyeBoundingBoxS32);

        s32 glowSizeX = std::ceil(glowFraction * 0.5f * scaledEyeWidth);
        s32 glowSizeY = std::ceil(glowFraction * 0.5f * scaledEyeHeight);

        // Make sure sizes are odd:
        if(glowSizeX % 2 == 0) {
          ++glowSizeX;
        }
        if(glowSizeY % 2 == 0) {
          ++glowSizeY;
        }

        if(kProcFace_GlowGaussianFilter) {
          cv::GaussianBlur(eyeShapeROI.get_CvMat_(), glowImgROI.get_CvMat_(), cv::Size(glowSizeX,glowSizeY),
                           (f32)glowSizeX, (f32)glowSizeY);
        } else {
          cv::boxFilter(eyeShapeROI.get_CvMat_(), glowImgROI.get_CvMat_(), -1, cv::Size(glowSizeX,glowSizeY));
        }
      }

      // Antialiasing (AFTER glow because it changes eyeShape, which we use to compute the glow above)
      if(kProcFace_AntiAliasingSize > 0) {
        ANKI_CPU_PROFILE("AntiAliasing");
        if(kProcFace_AntiAliasingSize % 2 == 0) {
          ++kProcFace_AntiAliasingSize; // Antialiasing filter size should be odd
        }
        if(kProcFace_AntiAliasingGaussianFilter) {
          const f32 kAntiAliasingSigmaFraction = 0.5f;
          cv::GaussianBlur(eyeShapeROI.get_CvMat_(), eyeShapeROI.get_CvMat_(), cv::Size(kProcFace_AntiAliasingSize,kProcFace_AntiAliasingSize),
                           (f32)kProcFace_AntiAliasingSize * kAntiAliasingSigmaFraction);
        } else {
          cv::boxFilter(eyeShapeROI.get_CvMat_(), eyeShapeROI.get_CvMat_(), -1, cv::Size(kProcFace_AntiAliasingSize,kProcFace_AntiAliasingSize));
        }
      }
      
 
      const f32 hueFactor = faceData.GetHue();
      DEV_ASSERT(Util::InRange(hueFactor, 0.f, 1.f), "ProceduralFaceDrawer.DrawEye.InvalidHue");
      const u8 drawHue = std::round(255.f*hueFactor);
      
      const f32 satFactor = faceData.GetParameter(whichEye, Parameter::Saturation);
      DEV_ASSERT(Util::InRange(satFactor, -1.f, 1.f), "ProceduralFaceDrawer.DrawEye.InvalidSaturation");
      const u8 drawSat = std::round(255.f * satFactor);
      
      const f32 eyeLightness = faceData.GetParameter(whichEye, Parameter::Lightness);
      DEV_ASSERT(Util::InRange(eyeLightness, -1.f, 1.f), "ProceduralFaceDrawer.DrawEye.InvalidLightness");
    
      const f32 scanlineOpacity = faceData.GetScanlineOpacity();
      DEV_ASSERT(Util::InRange(scanlineOpacity, 0.f, 1.f), "ProceduralFaceDrawer.DrawEye.InvalidScanlineOpacity");
      
      // Draw the eye into the face image, adding outer glow, noise, and stylized scanlines
      {
        ANKI_CPU_PROFILE("DrawEyePixels");
#if PROCEDURALFACE_NOISE_FEATURE
        const Array2d<f32>& noiseImg = GetNoiseImage(rng);
#endif

        for(s32 i=upperLeft.y(); i<=bottomRight.y(); ++i)
        {
          //DEV_ASSERT_MSG(i>=0 && i<faceImg.GetNumRows(), "ProceduralFaceDrawer.DrawEye.BadRow", "%d", i);
          
          Vision::PixelRGB* faceImg_i = faceImg.GetRow(i);
          const u8*  eyeShape_i = _eyeShape.GetRow(i);
          const u8*  glowImg_i  = _glowImg.GetRow(i);
#if PROCEDURALFACE_NOISE_FEATURE
          const f32* noiseImg_i = noiseImg.GetRow(i);
#endif

          for(s32 j=upperLeft.x(); j<=bottomRight.x(); ++j)
          {
            //DEV_ASSERT_MSG(j>=0 && j<faceImg.GetNumCols(), "ProceduralFaceDrawer.DrawEye.BadCol", "%d", j);
            
            const u8 eyeValue  = eyeShape_i[j];
            const u8 glowValue = glowImg_i[j];
            
            const bool somethingToDraw = (eyeValue > 0 || glowValue > 0);
            
            if(somethingToDraw)
            {
              // Combine everything together: noise, scanline, inner glow falloff, and the antialiasing / glow value.
              // Note that the value in glowImg/eyeShape is already [0,255]
              f32 newValue = static_cast<f32>(std::max(glowValue,eyeValue));
#if PROCEDURALFACE_NOISE_FEATURE
              if (kProcFace_NoiseNumFrames > 0) {
                newValue *= noiseImg_i[j] * eyeLightness;
              } else
#endif
              {
                newValue *= eyeLightness;
              }

              // Don't do scanlines in the glow region.
              const bool isPartOfEye = (eyeValue >= glowValue); // (and not part of glow)
              if(isPartOfEye)
              {
                newValue *= kProcFace_EyeLightnessMultiplier;
              }
              else
              {
                newValue *= glowLightness;
              }
              
              // Put the final value into the face image
              // Note: If we're drawing the right eye, there may already be something in the image
              //       from when we drew the left eye (e.g. with large glow), so use max
              Vision::PixelRGB& facePixel = faceImg_i[j];
              facePixel.r() = drawHue;
              facePixel.g() = drawSat;
              facePixel.b() = std::max(facePixel.b(), Util::numeric_cast_clamped<u8>(std::round(newValue)));
            }
          }
        } 
      }
    }
    
    // Add distortion noise
    auto scanlineDistorter = faceData.GetScanlineDistorter();
    if(nullptr != scanlineDistorter)
    {
      scanlineDistorter->AddOffNoise(W, eyeHeight, eyeWidth, faceImg);
    }
    
  } // DrawEye()
  
  void ProceduralFaceDrawer::DrawFace(const ProceduralFace& faceData, 
                                      const Util::RandomGenerator& rng, 
                                      Vision::ImageRGB565& output)
  {
#if ANKI_CPU_PROFILER_ENABLED
    constexpr float kMaxExpectedFaceDrawTime_ms = 13.f;
#endif
    // Replace Util::CpuThreadProfiler::kLogFrequencyNever with a small value to output logging,
    // can be used with Chrome Tracing format
    ANKI_CPU_TICK("ProceduralFaceDrawer", kMaxExpectedFaceDrawTime_ms, Util::CpuThreadProfiler::kLogFrequencyNever);
    // Replace the above line with the below line to see profiling info in the log
    // ANKI_CPU_TICK("ProceduralFaceDrawer", kMaxExpectedFaceDrawTime_ms, 61);
    ANKI_CPU_PROFILE("DrawFace");

    // Make sure output is allocated appropriately.
    // Will do nothing if already the right size.
    output.Allocate(ProceduralFace::HEIGHT, ProceduralFace::WIDTH);
    
    // Static image to do all our drawing in, will be converted to RGB565 at the end
    // This is treated as an HSV image
    static Vision::ImageRGB faceImg;
    faceImg.Allocate(ProceduralFace::HEIGHT, ProceduralFace::WIDTH); // Will do nothing if already the right size
    faceImg.FillWith(Vision::PixelRGB(0,0,0));
    
    Rectangle<f32> leftBBox, rightBBox;
    {
      ANKI_CPU_PROFILE("DrawEyes");
      DrawEye(faceData, WhichEye::Left,  rng, faceImg, leftBBox);
      DrawEye(faceData, WhichEye::Right, rng, faceImg, rightBBox);
    }

    s32 rowMin = ProceduralFace::HEIGHT-1, rowMax = 0;
    s32 colMin = leftBBox.GetX(), colMax = rightBBox.GetXmax();

    ANKI_CPU_PROFILE_START(prof_applyWholeFaceParams, "ApplyWholeFaceParams");
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
          colMin = std::min(colMin, (s32)std::floor(warpedPoint.x()));
          colMax = std::max(colMax, (s32)std::ceil(warpedPoint.x()));
        }
      }
      
    }
    else
    {
      rowMin = std::min(leftBBox.GetY(), rightBBox.GetY());
      rowMax = std::max(leftBBox.GetYmax(), rightBBox.GetYmax());
    }
    ANKI_CPU_PROFILE_STOP(prof_applyWholeFaceParams);

    const float scanlineOpacity = faceData.GetScanlineOpacity();
    const bool applyScanlines = !Util::IsNear(scanlineOpacity, 1.f);
    if (applyScanlines) {
      ANKI_CPU_PROFILE("ApplyScanlines");
      ProceduralFaceDrawer::ApplyScanlines(faceImg, scanlineOpacity);
    }

    // Just to be safe:
    rowMin = Util::Clamp(rowMin, 0, ProceduralFace::HEIGHT-1);
    rowMax = Util::Clamp(rowMax, 0, ProceduralFace::HEIGHT-1);

    if(rowMax > rowMin)
    {   
      ANKI_CPU_PROFILE("DistortScanlines");
      // Distort the scanlines
      auto scanlineDistorter = faceData.GetScanlineDistorter();
      if(nullptr != scanlineDistorter)
      {
        const f32 scale = 1.f / (rowMax - rowMin);
        for(s32 row=rowMin; row < rowMax; ++row)
        {
          const f32 eyeFrac = (row - rowMin) * scale;
          const s32 shift = scanlineDistorter->GetEyeDistortionAmount(eyeFrac) * sizeof(Vision::PixelRGB);
          
          if(shift < 0)
          {
            Vision::PixelRGB* faceImg_row = faceImg.GetRow(row);
            memmove(faceImg_row, faceImg_row-shift, ProceduralFace::WIDTH+shift);
            memset(faceImg_row+ProceduralFace::WIDTH+shift, 0, -shift);
          }
          else if(shift > 0)
          {
            Vision::PixelRGB* faceImg_row = faceImg.GetRow(row);
            memmove(faceImg_row+shift, faceImg_row, ProceduralFace::WIDTH-shift);
            memset(faceImg_row, 0, shift);
          }
        }
      }
      
      Rectangle<s32> eyesROI(colMin, rowMin, colMax-colMin+1, rowMax-rowMin+1);
      output.FillWith(0);
      Vision::ImageRGB565 roi = output.GetROI(eyesROI);
      faceImg.GetROI(eyesROI).ConvertHSV2RGB565(roi);
    }
    
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
  
  void ProceduralFaceDrawer::ApplyScanlines(Vision::ImageRGB& imageHsv, const float opacity)
  {
    DEV_ASSERT(Util::InRange(opacity, 0.f, 1.f), "ProceduralFaceDrawer.ApplyCurrentFaceHue.InvalidOpacity");
    
    const auto nRows = imageHsv.GetNumRows();
    const auto nCols = imageHsv.GetNumCols();
    
    for (int i=0 ; i < nRows ; i++) {
      if (ShouldApplyScanlineToRow(i)) {
        auto* thisRow = imageHsv.GetRow(i);
        for (int j=0 ; j < nCols ; j++) {
          // the 'blue' channel in an HSV image is the value
          thisRow[j].b() *= opacity;
        }
      }
    }
  }
  
} // namespace Cozmo
} // namespace Anki
