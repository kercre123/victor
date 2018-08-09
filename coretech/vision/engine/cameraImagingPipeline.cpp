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

namespace {
#define CONSOLE_GROUP "Vision.PreProcessing"
    
// Turn this on to linearize brightness values using the camera's Gamma table
// before computing the desired exposure. Should be more accurate, but may not
// be super necessary.
CONSOLE_VAR(bool,         kLinearizeForAutoExposure,       CONSOLE_GROUP, false);
  
// Pixels are considered under-exposed if strictly less than this value
CONSOLE_VAR(u8,           kUnderExposedThreshold,          CONSOLE_GROUP, 15);

// Pixels are considered over-exposed if strictly greater than this value
CONSOLE_VAR(u8,           kOverExposedThreshold,           CONSOLE_GROUP, 240);

CONSOLE_VAR_RANGED(f32,   kExposure_TargetPercentile,      CONSOLE_GROUP, 0.f, 0.f, 1.f); // 0 to disable
CONSOLE_VAR_RANGED(s32,   kExposure_TargetValue,           CONSOLE_GROUP, 128, 0, 255);   
CONSOLE_VAR_RANGED(f32,   kMaxFractionOverexposed,         CONSOLE_GROUP, 0.8f, 0.f, 1.f);
CONSOLE_VAR_RANGED(f32,   kOverExposedAdjustmentFraction,  CONSOLE_GROUP, 0.5f, 0.f, 1.f);

#undef CONSOLE_GROUP
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::SetExposureParameters(const u8               targetValue,
                                              const std::vector<u8>& cyclingTargetValues,
                                              const f32              targetPercentile,
                                              const f32              maxChangeFraction,
                                              const s32              subSample)
{
  if(!Util::InRange(maxChangeFraction, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadMaxChangeFraction",
                      "%f not on interval [0,1]", maxChangeFraction);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(!Util::InRange(targetPercentile, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadTargetPercentile",
                      "%f not on interval [0,1]", targetPercentile);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(subSample <= 0)
  {
    PRINT_NAMED_ERROR("ImagingPipeline.SetExposureParameters.BadSubSample",
                      "%d not > 0", subSample);
    return RESULT_FAIL_INVALID_PARAMETER;
  }

  _targetValue              = targetValue;
  _targetPercentile         = targetPercentile;
  _maxChangeFraction        = maxChangeFraction;
  _subSample                = subSample;
  _cyclingTargetValues      = cyclingTargetValues;
  
  if(cyclingTargetValues.empty())
  {
    PRINT_NAMED_WARNING("ImagingPipeline.SetExposureParameters.EmptyCyclingTargetList",
                        "Will use targetValue=%d", (s32)_targetValue);
    _cyclingTargetValues.push_back(_targetValue);
  }
  else
  {
    _cyclingTargetValues = cyclingTargetValues;
  }
  _cycleTargetIter          = _cyclingTargetValues.begin();
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImagingPipeline::ImagingPipeline()
: _cyclingTargetValues{32, 128, 224}
, _cycleTargetIter(_cyclingTargetValues.begin())
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
Result ImagingPipeline::ComputeAdjustmentFraction(const bool useCycling, f32& adjustmentFraction)
{
  if(_hist.GetTotalCount() == 0)
  {
    PRINT_NAMED_WARNING("ImagingPipeline.ComputeAdjustmentFraction.EmptyHistogram", "");
    return RESULT_FAIL;
  }
  
  const s32 numOverexposed = _hist.GetCounts().back();
  const f32 fractionOverexposed = static_cast<f32>(numOverexposed) / static_cast<f32>(_hist.GetTotalCount());
  if(fractionOverexposed > kMaxFractionOverexposed)
  {
    // Special case: too over-exposed. Drop the exposure drastically.
    adjustmentFraction = kOverExposedAdjustmentFraction;
    return RESULT_OK;
  }
  
  const bool useConsoleVarTargets = Util::IsFltGTZero(kExposure_TargetPercentile);

  const f32 currentTargetPercentile = (useConsoleVarTargets ? kExposure_TargetPercentile : _targetPercentile);

  const u8 currentPercentileValue = _hist.ComputePercentile(currentTargetPercentile);
  
  if(currentPercentileValue == 0)
  {
    // Special case: avoid divide by zero and just increase by maximum amount possible
    adjustmentFraction = 1.f + _maxChangeFraction;
    return RESULT_OK;
  }
  
  u8 currentTargetValue = 128;
  if(useCycling)
  {
    DEV_ASSERT(!_cyclingTargetValues.empty(), "ImagingPipeline.ComputeAdjustmentFraction.EmptyCyclingTargetValues");
    currentTargetValue = *_cycleTargetIter;
    ++_cycleTargetIter;
    if(_cycleTargetIter == _cyclingTargetValues.end()) {
      _cycleTargetIter = _cyclingTargetValues.begin();
    }
  }
  else
  {
    currentTargetValue = (useConsoleVarTargets ? kExposure_TargetValue : _targetValue);
  }
  
  // Normal case: Current exposure is "reasonable" so just figure out how to make it a bit better
  adjustmentFraction = static_cast<f32>(currentTargetValue) / static_cast<f32>(currentPercentileValue);
  adjustmentFraction = Util::Clamp(adjustmentFraction, 1.f - _maxChangeFraction, 1.f + _maxChangeFraction);
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAdjustment(const Vision::Image& image,
                                                  const Vision::Image& weightMask,
                                                  const bool useCycling,
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
  
  result = ComputeAdjustmentFraction(useCycling, adjustmentFraction);
  
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAdjustment(const Image& img, const bool useCycling, f32& adjustmentFraction)
{
  const Vision::Image emptyWeights;
  return ComputeExposureAdjustment(img, emptyWeights, useCycling, adjustmentFraction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

  const bool kUseCycling = false;
  Result result = ComputeAdjustmentFraction(kUseCycling, adjustmentFraction);
  
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline static bool IsWellExposed(const Vision::PixelRGB& pixel)
{
  if( (kUnderExposedThreshold==0) && (kOverExposedThreshold==0) )
  {
    // Special case: everything is well exposed
    return true;
  }
  
  const bool isWellExposed = (Util::InRange(pixel.r(), kUnderExposedThreshold, kOverExposedThreshold) &&
                              Util::InRange(pixel.g(), kUnderExposedThreshold, kOverExposedThreshold) &&
                              Util::InRange(pixel.b(), kUnderExposedThreshold, kOverExposedThreshold));
  
  return isWellExposed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
      
      if(IsWellExposed(pixel))
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAndWhiteBalance(const Vision::ImageRGB& img,
                                                       const bool useCycling,
                                                       f32 &adjExp, f32& adjR, f32& adjB)
{
  Vision::Image weights; // empty
  return ComputeExposureAndWhiteBalance(img, weights, useCycling, adjExp, adjR, adjB);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImagingPipeline::ComputeExposureAndWhiteBalance(const Vision::ImageRGB& img,
                                                       const Vision::Image& weights,
                                                       const bool useCycling,
                                                       f32 &adjExp, f32& adjR, f32& adjB)
{
  _hist.Reset();
  
  // Calculate the sum for each of the three color channels for use in white balancing
  // Note: we technically care about averages, but since we are just using ratios of the different channels
  //       below, the divisor cancels out, so we can just use sums for better efficiency. This also means
  //       we don't have to worry about tracking the number of [sub-sampled] well-exposed pixels.
  s32 sumR = 0;
  s32 sumG = 0;
  s32 sumB = 0;
  
  const bool haveWeights = !weights.IsEmpty();
  
  // Use green channel to compute the desired exposure update
  for(u32 i = 0; i < img.GetNumRows(); i += _subSample)
  {
    const Vision::PixelRGB* image_i = img.GetRow(i);
    const u8* weight_i = (haveWeights ? weights.GetRow(i) : nullptr);
    
    for(u32 j = 0; j < img.GetNumCols(); j += _subSample)
    {
      const Vision::PixelRGB& pixel = image_i[j];
      
      // Ignore over/under-exposed pixels in computing statistics for white balance
      if(IsWellExposed(pixel))
      {
        // TODO: Consider linearizing (un-gamma) before computing statistics needed for this update
        sumR += pixel.r();
        sumG += pixel.g();
        sumB += pixel.b();
      }
      
      // Use green channel for brightness statistics to compute exposure adjustment
      if(haveWeights)
      {
        const u8 weight = weight_i[j];
        _hist.IncrementBin(pixel.g(), weight);
      }
      else
      {
        _hist.IncrementBin(pixel.g());
      }
    }
  }

  // Adjust red and blue gains so their channel's average value match green's, making sure
  // not to adjust too much in a single update:
  adjR = (sumR==0 ? 1.f : (f32)sumG/(f32)sumR);
  adjB = (sumB==0 ? 1.f : (f32)sumG/(f32)sumB);
  adjR = Util::Clamp(adjR, 1.f-_maxChangeFraction, 1.f+_maxChangeFraction);
  adjB = Util::Clamp(adjB, 1.f-_maxChangeFraction, 1.f+_maxChangeFraction);
  
  const Result result = ComputeAdjustmentFraction(useCycling, adjExp);
  
  return result;
}
  
  
} // namespace Vision
} // namespace Anki
