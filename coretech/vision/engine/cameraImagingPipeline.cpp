/**
 * File: cameraImagingPipeline.cpp
 *
 * Author: Andrew Stein
 * Date:   10/11/2016
 *
 * NOTE: Adapted from Pete Barnum's original Embedded implementation
 *
 * Description: Settings and methods for image capture pipeline, including things
 *              like exposure control, gamma lookup tables, vignetting, etc.
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "coretech/vision/engine/cameraImagingPipeline.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/image.h"

#include "util/console/consoleInterface.h"
#include "util/math/math.h"


namespace Anki {
namespace Vision {

// Turn this on to linearize brightness values using the camera's Gamma table
// before computing the desired exposure. Should be more accurate, but may not
// be super necessary.
CONSOLE_VAR(bool,  kLinearizeForAutoExposure,     "Vision.PreProcessing", false);
CONSOLE_VAR(bool,  kUseWhiteBalanceExposureCheck, "Vision.PreProcessing", true);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::SetExposureParameters(u8  targetMidValue,
                                              f32 midPercentile,
                                              f32 maxChangeFraction,
                                              s32 subSample)
{
  if(FLT_LT(maxChangeFraction, 0.f) || FLT_GT(maxChangeFraction, 1.f))
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadMaxChangeFraction",
                      "%f not on interval [0,1]", maxChangeFraction);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(FLT_LT(midPercentile, 0.f) || FLT_GT(midPercentile, 1.f))
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadMidPercentile",
                      "%f not on interval [0,1]", midPercentile);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(subSample <= 0)
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadSubSample",
                      "%d not > 0", subSample);
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  _targetMidValue           = targetMidValue;
  _midPercentile            = midPercentile;
  _maxChangeFraction        = maxChangeFraction;
  _subSample                = subSample;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImagingPipeline::ImagingPipeline()
{
  for(s32 i=0; i<256; ++i)
  {
    _gammaLUT[i] = i;
    _linearizationLUT[i] = i;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void InterpCurveHelper(const std::vector<u8>& x, const std::vector<u8>& y,
                              std::array<u8,256>& curve)
{
  DEV_ASSERT(!x.empty() && !y.empty(), "ImagingPipeline.InterpCurveHelper.EmptyXorY");
  DEV_ASSERT(x.size() == y.size(), "ImagingPipeline.InterpCurveHelper.MismatchedSizes");
  
  auto xNext = x.begin();
  auto yNext = y.begin();
  
  s32 i=0;
  f32 slope = static_cast<f32>(*yNext)/static_cast<f32>(*xNext);
  while(i < *xNext)
  {
    const f32 interpValue = slope * static_cast<f32>(i);
    curve[i] = Util::numeric_cast<u8>(std::round(interpValue));
    ++i;
  }
  
  auto xPrev = x.begin();
  auto yPrev = y.begin();
  ++xNext;
  ++yNext;
  
  while(xNext != x.end())
  {
    const s32 xn = *xNext;
    const s32 xp = *xPrev;
    const s32 yn = *yNext;
    const s32 yp = *yPrev;
    
    slope = 0.f;
    const s32 xdiff = xn - xp;
    if(xdiff != 0)
    {
      slope = static_cast<f32>(yn - yp) / static_cast<f32>(xdiff);
    }
    
    while(i < xn)
    {
      const f32 interpValue = static_cast<f32>(i - xp) * slope + static_cast<f32>(yp);
      curve[i] = Util::numeric_cast<u8>(std::round(interpValue));
      ++i;
    }
    
    ++xNext;
    ++yNext;
    ++xPrev;
    ++yPrev;
  }

  while(i <= 255)
  {
    const f32 interpValue = slope * static_cast<f32>(i - (s32)x.back()) + static_cast<f32>(y.back());
    curve[i] = cv::saturate_cast<u8>(std::round(interpValue));
    ++i;
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::SetGammaTable(const std::vector<u8>& inputs, const std::vector<u8>& gamma)
{
  InterpCurveHelper(inputs, gamma, _gammaLUT);
  InterpCurveHelper(gamma, inputs, _linearizationLUT);
  
  if(ANKI_DEVELOPER_CODE)
  {
    std::stringstream debugInX, debugInY, debugGamma, debugLinear;
    for(auto x : inputs) {
      debugInX << (s32)x << " ";
    }
    for(auto y : gamma) {
      debugInY << (s32)y << " ";
    }
    for(auto g : _gammaLUT) {
      debugGamma << (s32)g << " ";
    }
    for(auto l : _linearizationLUT) {
      debugLinear << (s32)l << " ";
    }
  
    PRINT_CH_DEBUG("VisionSystem", "ImagingPipeline.SetGammaTable.InputX", "%s", debugInX.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "ImagingPipeline.SetGammaTable.InputY", "%s", debugInY.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "ImagingPipeline.SetGammaTable.Gamma",  "%s", debugGamma.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "ImagingPipeline.SetGammaTable.Linear", "%s", debugLinear.str().c_str());
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAdjustment(const Vision::Image& image, const Vision::Image& weightMask,
                                                  f32& adjustmentFraction)
{
  // Initialize in case of early return
  adjustmentFraction = 1.f;
  
  ImageBrightnessHistogram::TransformFcn linearizeFcn = {};
  
  if(kLinearizeForAutoExposure)
  {
    linearizeFcn = [this](u8 value)
    {
      return _linearizationLUT[value];
    };
  }
  
  _hist.Reset();
  
  Result result = RESULT_FAIL;
  if(weightMask.IsEmpty())
  {
    result = _hist.FillFromImage(image, _subSample, linearizeFcn);
  }
  else
  {
    result = _hist.FillFromImage(image, weightMask, _subSample, linearizeFcn);
  }
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("ImagingPipeline.ComputeNewExposure.FillHistogramFailed", "");
    return result;
  }
  
  if(_hist.GetTotalCount() == 0)
  {
    PRINT_NAMED_WARNING("ImagingPipeline.ComputeNewExposure.EmptyHistogram", "");
    return RESULT_FAIL;
  }
  
  const u8 currentMidValue = _hist.ComputePercentile(_midPercentile);
  
  if(currentMidValue == 0)
  {
    // Special case: avoid divide by zero and just increase by maximum amount possible
    adjustmentFraction = 1.f + _maxChangeFraction;
  }
  else
  {
    adjustmentFraction = static_cast<f32>(_targetMidValue) / static_cast<f32>(currentMidValue);
    adjustmentFraction = CLIP(adjustmentFraction, 1.f - _maxChangeFraction, 1.f + _maxChangeFraction);
  }
  
  return RESULT_OK;

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAdjustment(const Image& img, f32& adjustmentFraction)
{
  const Vision::Image emptyWeights;
  return ComputeExposureAdjustment(img, emptyWeights, adjustmentFraction);
}
  
Result ImagingPipeline::ComputeExposureAdjustment(const std::vector<Vision::Image>& imageROIs, f32& adjustmentFraction)
{
  // Initialize in case of early return
  adjustmentFraction = 1.f;
  
  if(imageROIs.empty())
  {
    PRINT_NAMED_ERROR("ImagingPipeline.ComputeNewExposure.EmptyImageVector", "No image ROIs provided");
    return RESULT_FAIL;
  }
  
  std::function<u8(u8)> linearizeFcn = {};
  
  if(kLinearizeForAutoExposure)
  {
    linearizeFcn = [this](u8 value)
                   {
                     return _linearizationLUT[value];
                   };
  }
  
  _hist.Reset();
  
  // Compute the cumulative histogram over all ROIs
  for(auto const& img : imageROIs)
  {
    if(img.IsEmpty())
    {
      PRINT_NAMED_WARNING("ImagingPipeline.ComputeNewExposure.EmptyImage", "Skipping");
      continue;
    }
    
    Result result = _hist.FillFromImage(img, _subSample, linearizeFcn);
    
    if(RESULT_OK != result)
    {
      PRINT_NAMED_ERROR("ImagingPipeline.ComputeNewExposure.ComputeHistogramFailed", "");
      return result;
    }
  }
  
  if(_hist.GetTotalCount() == 0)
  {
    PRINT_NAMED_WARNING("ImagingPipeline.ComputeNewExposure.EmptyHistogram", "");
    return RESULT_FAIL;
  }
  
  const u8 currentMidValue = _hist.ComputePercentile(_midPercentile);
  
  if(currentMidValue == 0)
  {
    // Special case: avoid divide by zero and just increase by maximum amount possible
    adjustmentFraction = 1.f + _maxChangeFraction;
  }
  else
  {
    adjustmentFraction = static_cast<f32>(_targetMidValue) / static_cast<f32>(currentMidValue);
    adjustmentFraction = CLIP(adjustmentFraction, 1.f - _maxChangeFraction, 1.f + _maxChangeFraction);
  }
  
  return RESULT_OK;
}

Result ImagingPipeline::ComputeWhiteBalanceAdjustment(const Vision::ImageRGB& image, f32& adjR, f32& adjB)
{
  s32 sumR = 0;
  s32 sumG = 0;
  s32 sumB = 0;

  const u32 numRows = image.GetNumRows();
  const u32 numCols = image.GetNumCols();

  // Calculate the sum for each of the three color channels
  // Note: we technically care about averages, but since we are just using ratios of the different channels
  //       below, the divisor cancels out, so we can just use sums for better efficiency. This also means
  //       we don't have to worry about tracking the number of [sub-sampled] well-exposed pixels.

  for(u32 i = 0; i < numRows; i += _subSample)
  {
    const Vision::PixelRGB* image_i = image.GetRow(i);
    for(u32 j = 0; j < numCols; j += _subSample)
    {
      // Ignore over/under-exposed pixels in computing statistics
      const Vision::PixelRGB& pixel = image_i[j];
      const bool isWellExposed = !kUseWhiteBalanceExposureCheck || (pixel.r() > 0 && pixel.r() < 255 &&
                                                                    pixel.g() > 0 && pixel.g() < 255 &&
                                                                    pixel.b() > 0 && pixel.b() < 255);
      if(isWellExposed)
      {
        // TODO: Consider linearizing (un-gamma) before computing statistics needed for this update
        sumR += pixel.r();
        sumG += pixel.g();
        sumB += pixel.b();
      }
    }
  }

  // Adjust red and blue gains so their channel's average value match green's, making sure
  // not to adjust too much in a single update:
  adjR = (sumR==0 ? 1.f : (f32)sumG/(f32)sumR);
  adjB = (sumB==0 ? 1.f : (f32)sumG/(f32)sumB);
  
  // Don't change too much at each update
  adjR = Util::Clamp(adjR, 1.f-_maxChangeFraction, 1.f+_maxChangeFraction);
  adjB = Util::Clamp(adjB, 1.f-_maxChangeFraction, 1.f+_maxChangeFraction);

  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki
