/**
 * File: cameraParamsController.cpp
 *
 * Author: Andrew Stein
 * Date:   10/11/2016
 *
 *
 * Description: Auto-exposure and white balance control.
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "coretech/vision/engine/cameraParamsController.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/image.h"

#include "util/console/consoleInterface.h"
#include "util/math/math.h"

const char * const kLogChannelName = "VisionSystem"; // TODO: Update and use LOG_CHANNEL

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
CameraParamsController::CameraParamsController(u16 minExpTime_ms, u16 maxExpTime_ms, f32 minGain, f32 maxGain,
                                               const CameraParams& initialParams)
: _minExposureTime_ms(minExpTime_ms)
, _maxExposureTime_ms(maxExpTime_ms)
, _minGain(minGain)
, _maxGain(maxGain)
, _currentCameraParams(initialParams)
, _cyclingTargetValues{32, 128, 224}
, _cycleTargetIter(_cyclingTargetValues.begin())
{
  for(s32 i=0; i<256; ++i)
  {
    _gammaLUT[i] = i;
    _linearizationLUT[i] = i;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::SetExposureParameters(const u8               targetValue,
                                                     const std::vector<u8>& cyclingTargetValues,
                                                     const f32              targetPercentile,
                                                     const f32              maxChangeFraction,
                                                     const s32              subSample)
{
  if(!Util::InRange(maxChangeFraction, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("CameraParamsController.SetExposureParameters.BadMaxChangeFraction",
                      "%f not on interval [0,1]", maxChangeFraction);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(!Util::InRange(targetPercentile, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("CameraParamsController.SetExposureParameters.BadTargetPercentile",
                      "%f not on interval [0,1]", targetPercentile);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(subSample <= 0)
  {
    PRINT_NAMED_ERROR("CameraParamsController.SetExposureParameters.BadSubSample",
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
    PRINT_NAMED_WARNING("CameraParamsController.SetExposureParameters.EmptyCyclingTargetList",
                        "Will use targetValue=%d", (s32)_targetValue);
    _cyclingTargetValues.push_back(_targetValue);
  }

  _cycleTargetIter = _cyclingTargetValues.begin();
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::SetImageQualityParameters(const u8 tooDarkValue,   const f32 tooDarkPercentile,
                                                         const u8 tooBrightValue, const f32 tooBrightPercentile)
{
  if(!Util::InRange(tooDarkPercentile, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("CameraParamsController.SetExposureParameters.BadTooDarkPercentile",
                      "%f not on interval [0,1]", tooDarkPercentile);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  if(!Util::InRange(tooBrightPercentile, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("CameraParamsController.SetExposureParameters.BadTooBrightPercentile",
                      "%f not on interval [0,1]", tooBrightPercentile);
    return RESULT_FAIL_INVALID_PARAMETER;
  }
  
  _tooDarkValue = tooDarkValue;
  _tooDarkPercentile = tooDarkPercentile;
  
  _tooBrightValue = tooBrightValue;
  _tooBrightPercentile = tooBrightPercentile;
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void InterpCurveHelper(const std::vector<u8>& x, const std::vector<u8>& y,
                              std::array<u8,256>& curve)
{
  DEV_ASSERT(!x.empty() && !y.empty(), "CameraParamsController.InterpCurveHelper.EmptyXorY");
  DEV_ASSERT(x.size() == y.size(), "CameraParamsController.InterpCurveHelper.MismatchedSizes");
  
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
Result CameraParamsController::SetGammaTable(const std::vector<u8>& inputs, const std::vector<u8>& gamma)
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
  
    PRINT_CH_DEBUG("VisionSystem", "CameraParamsController.SetGammaTable.InputX", "%s", debugInX.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "CameraParamsController.SetGammaTable.InputY", "%s", debugInY.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "CameraParamsController.SetGammaTable.Gamma",  "%s", debugGamma.str().c_str());
    PRINT_CH_DEBUG("VisionSystem", "CameraParamsController.SetGammaTable.Linear", "%s", debugLinear.str().c_str());
  }
  
  return RESULT_OK;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::UpdateCurrentCameraParams(const CameraParams& newParams)
{
  if(!AreCameraParamsValid(newParams))
  {
    PRINT_NAMED_WARNING("CameraParamsController.UpdateCurrentCameraParams.InvalidParams", "");
    return RESULT_FAIL;
  }
 
  _currentCameraParams = newParams;
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::ComputeNextCameraParams(const Image& img,
                                                       const AutoExpMode aeMode,
                                                       const bool useExposureCycling,
                                                       CameraParams& nextParams)
{
  // Special case: nothing to do
  if(AutoExpMode::Off == aeMode)
  {
    return RESULT_OK;
  }
  
  // Compute the exposure we would like to have
  f32 exposureAdjFrac = 1.f;
  f32 adjR = 1.f;
  f32 adjB = 1.f;
  
  const Result result = ComputeExposureAdjustment(img, useExposureCycling, exposureAdjFrac);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("CameraParamsController.ComputeNextCameraParams.Grayscale.ComputeExpFailed", "");
    return result;
  }
  
  const WhiteBalanceMode wbMode = WhiteBalanceMode::Off;
  
  return ComputeNextCameraParamsHelper(aeMode, wbMode, exposureAdjFrac, adjR, adjB, nextParams);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::ComputeNextCameraParams(const ImageRGB& img,
                                                       const AutoExpMode aeMode,
                                                       const WhiteBalanceMode wbMode,
                                                       const bool useExposureCycling,
                                                       CameraParams& nextParams)
{
  // Special case: nothing to do
  if( (AutoExpMode::Off == aeMode) && (WhiteBalanceMode::Off == wbMode) )
  {
    return RESULT_OK;
  }
  
  // Compute the exposure we would like to have
  f32 exposureAdjFrac = 1.f;
  f32 adjR = 1.f;
  f32 adjB = 1.f;
  
  const Result result = ComputeExposureAndWhiteBalance(img, useExposureCycling, exposureAdjFrac, adjR, adjB);
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("CameraParamsController.ComputeNextCameraParams.RGB.ComputeExpAndWhitBalanceFailed", "");
    return result;
  }
  
  return ComputeNextCameraParamsHelper(aeMode, wbMode, exposureAdjFrac, adjR, adjB, nextParams);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::ComputeNextCameraParamsHelper(const AutoExpMode aeMode,
                                                             const WhiteBalanceMode wbMode,
                                                             f32 exposureAdjFrac, f32 adjR, f32 adjB,
                                                             CameraParams& nextParams)
{
  s32 desiredExposureTime_ms = _currentCameraParams.exposureTime_ms;
  f32 desiredGain = _currentCameraParams.gain;
  _imageQuality = ImageQuality::Good; // In the absence of any evidence to the contrary, assume good
  
  if(AutoExpMode::Off == aeMode)
  {
    // Auto-exposure is disabled: ignore the computed exposure adjustment
    exposureAdjFrac = 1.f;
  }
  
  if(WhiteBalanceMode::Off == wbMode)
  {
    // White balance is disabled: ignore the computed WB adjustments
    adjR = 1.f;
    adjB = 1.f;
  }
  
  if(AutoExpMode::Off != aeMode)
  {
    if(aeMode != _aeMode)
    {
      // Switching modes: try to preserve roughly the same product of gain and exposure
      const f32 prod = static_cast<f32>(_currentCameraParams.exposureTime_ms) * _currentCameraParams.gain;
      
      switch(aeMode)
      {
        case AutoExpMode::Off:
          // We should not get here by virtue of IF above
          PRINT_NAMED_ERROR("CameraParamsController.ComputeNextCameraParamsHelper.UnexpectedAEMode1", "");
          return RESULT_FAIL;
          
        case AutoExpMode::MinGain:
        {
          // Make the new exposure the same fraction of its max that gain currently is.
          // Then pick a new gain to "match".
          const f32 frac = _currentCameraParams.gain / GetMaxCameraGain();
          desiredExposureTime_ms = static_cast<s32>(std::round(frac * static_cast<f32>(GetMaxCameraExposureTime_ms())));
          desiredExposureTime_ms = std::max(desiredExposureTime_ms, GetMinCameraExposureTime_ms());
          desiredGain = Util::Clamp(prod/static_cast<f32>(desiredExposureTime_ms),
                                    GetMinCameraGain(), GetMaxCameraGain());
          break;
        }
          
        case AutoExpMode::MinTime:
        {
          // Make the new gain the same fraction of its max that exposure currently is.
          // Then pick a new exposure to "match".
          const f32 frac = (static_cast<f32>(_currentCameraParams.exposureTime_ms) /
                            static_cast<f32>(GetMaxCameraExposureTime_ms()));
          desiredGain = std::max(frac*GetMaxCameraGain(), GetMinCameraGain());
          desiredExposureTime_ms = Util::Clamp(static_cast<s32>(std::round(prod / desiredGain)),
                                               GetMinCameraExposureTime_ms(), GetMaxCameraExposureTime_ms());
          break;
        }
      }
      
      PRINT_CH_INFO(kLogChannelName, "CameraParamsController.ComputeNextCameraParamsHelper.ToggleExpMode",
                    "Mode:%s, Gain:%.3f->%.3f, Exp:%d->%dms",
                    (_aeMode == AutoExpMode::MinGain ? "MinGain->MinTime" : "MinTime->MinGain"),
                    _currentCameraParams.gain, desiredGain,
                    _currentCameraParams.exposureTime_ms, desiredExposureTime_ms);
      
      _aeMode = aeMode;
    }
    
    if(Util::IsFltLT(exposureAdjFrac, 1.f))
    {
      // Want to bring brightness down
      bool madeAdjustment = false;
      switch(_aeMode)
      {
        case AutoExpMode::Off:
          // We should not get here by virtue of IF above
          PRINT_NAMED_ERROR("CameraParamsController.ComputeNextCameraParamsHelper.UnexpectedAEMode2", "");
          return RESULT_FAIL;
          
        case AutoExpMode::MinTime:
          if(_currentCameraParams.exposureTime_ms > GetMinCameraExposureTime_ms())
          {
            desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
            desiredExposureTime_ms = std::max(GetMinCameraExposureTime_ms(), desiredExposureTime_ms);
            madeAdjustment = true;
          }
          else if(Util::IsFltGT(_currentCameraParams.gain, GetMinCameraGain()))
          {
            // Already at min exposure time; reduce gain
            desiredGain *= exposureAdjFrac;
            desiredGain = std::max(GetMinCameraGain(), desiredGain);
            madeAdjustment = true;
          }
          break;
          
        case AutoExpMode::MinGain:
          if(Util::IsFltGT(_currentCameraParams.gain, GetMinCameraGain()))
          {
            desiredGain *= exposureAdjFrac;
            desiredGain = std::max(GetMinCameraGain(), desiredGain);
            madeAdjustment = true;
          }
          else if(_currentCameraParams.exposureTime_ms > GetMinCameraExposureTime_ms())
          {
            // Already at min gain time; reduce exposure
            desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
            desiredExposureTime_ms = std::max(GetMinCameraExposureTime_ms(), desiredExposureTime_ms);
            madeAdjustment = true;
          }
          break;
      }
      
      if(!madeAdjustment)
      {
        const u8 currentLowValue = GetHistogram().ComputePercentile(_tooBrightPercentile);
        if(currentLowValue > _tooBrightValue)
        {
          // Both exposure and gain are as low as they can go and the low value in the
          // image is still too high: it's too bright!
          _imageQuality = ImageQuality::TooBright;
        }
      }
    }
    else if(Util::IsFltGT(exposureAdjFrac, 1.f))
    {
      // Want to bring brightness up
      bool madeAdjustment = false;
      switch(aeMode)
      {
        case AutoExpMode::Off:
          // We should not get here by virtue of IF above
          PRINT_NAMED_ERROR("CameraParamsController.ComputeNextCameraParamsHelper.UnexpectedAEMode3", "");
          return RESULT_FAIL;
          
        case AutoExpMode::MinTime:
          if(Util::IsFltLT(_currentCameraParams.gain, GetMaxCameraGain()))
          {
            desiredGain *= exposureAdjFrac;
            desiredGain = std::min(GetMaxCameraGain(), desiredGain);
            madeAdjustment = true;
          }
          else if(_currentCameraParams.exposureTime_ms < GetMaxCameraExposureTime_ms())
          {
            // Already at max gain; increase exposure
            desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
            desiredExposureTime_ms = std::min(GetMaxCameraExposureTime_ms(), desiredExposureTime_ms);
            madeAdjustment = true;
          }
          break;
          
        case AutoExpMode::MinGain:
          if(_currentCameraParams.exposureTime_ms < GetMaxCameraExposureTime_ms())
          {
            desiredExposureTime_ms = std::round(static_cast<f32>(desiredExposureTime_ms) * exposureAdjFrac);
            desiredExposureTime_ms = std::min(GetMaxCameraExposureTime_ms(), desiredExposureTime_ms);
            madeAdjustment = true;
          }
          else if(Util::IsFltLT(_currentCameraParams.gain, GetMaxCameraGain()))
          {
            // Already at max exposure; increase gain
            desiredGain *= exposureAdjFrac;
            desiredGain = std::min(GetMaxCameraGain(), desiredGain);
            madeAdjustment = true;
          }
          break;
      }
      
      if(!madeAdjustment)
      {
        const u8 currentHighValue = GetHistogram().ComputePercentile(_tooDarkPercentile);
        if(currentHighValue < _tooDarkValue)
        {
          // Both exposure and gain are as high as they can go and the high value in the
          // image is still too low: it's too dark!
          _imageQuality = ImageQuality::TooDark;
        }
      }
    }
  } // if AE enabled
  
  // (Note that the exposure time/gain don't need to be clamped because of min/max calls above)
  DEV_ASSERT(IsExposureValid(desiredExposureTime_ms),
             "CameraParamsController.ComputeNextCameraParamsHelper.ExposureTimeOOR");
  DEV_ASSERT(IsGainValid(desiredGain),
             "CameraParamsController.ComputeNextCameraParamsHelper.ExposureGainOOR");
  nextParams.exposureTime_ms   = desiredExposureTime_ms;
  nextParams.gain              = desiredGain;
  nextParams.whiteBalanceGainR = Util::Clamp(_currentCameraParams.whiteBalanceGainR * adjR,
                                             GetMinCameraGain(), GetMaxCameraGain());
  nextParams.whiteBalanceGainG = Util::Clamp(_currentCameraParams.whiteBalanceGainG,
                                             GetMinCameraGain(), GetMaxCameraGain()); // Constant!
  nextParams.whiteBalanceGainB = Util::Clamp(_currentCameraParams.whiteBalanceGainB * adjB,
                                             GetMinCameraGain(), GetMaxCameraGain());
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::ComputeAdjustmentFraction(const bool useCycling, f32& adjustmentFraction)
{
  if(_hist.GetTotalCount() == 0)
  {
    PRINT_NAMED_WARNING("CameraParamsController.ComputeAdjustmentFraction.EmptyHistogram", "");
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
    DEV_ASSERT(!_cyclingTargetValues.empty(), "CameraParamsController.ComputeAdjustmentFraction.EmptyCyclingTargetValues");
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
Result CameraParamsController::ComputeExposureAdjustment(const Vision::Image& image,
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
  
  Image weightMask;
  const bool haveWeights = GetMeteringWeightMask(image.GetNumRows(), image.GetNumCols(), weightMask);
  if(haveWeights)
  {
    result = _hist.FillFromImage(image, _subSample, linearizeFcn);
  }
  else
  {
    result = _hist.FillFromImage(image, weightMask, _subSample, linearizeFcn);
  }
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("CameraParamsController.ComputeNewExposure.FillHistogramFailed", "");
    return result;
  }
  
  result = ComputeAdjustmentFraction(useCycling, adjustmentFraction);
  
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result CameraParamsController::ComputeExposureAdjustment(const std::vector<Vision::Image>& imageROIs,
                                                         f32& adjustmentFraction)
{
  // Initialize in case of early return
  adjustmentFraction = 1.f;
  
  if(imageROIs.empty())
  {
    PRINT_NAMED_ERROR("CameraParamsController.ComputeNewExposure.EmptyImageVector", "No image ROIs provided");
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
      PRINT_NAMED_WARNING("CameraParamsController.ComputeNewExposure.EmptyImage", "Skipping");
      continue;
    }
    
    Result result = _hist.FillFromImage(img, _subSample, linearizeFcn);
    
    if(RESULT_OK != result)
    {
      PRINT_NAMED_ERROR("CameraParamsController.ComputeNewExposure.ComputeHistogramFailed", "");
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
Result CameraParamsController::ComputeWhiteBalanceAdjustment(const Vision::ImageRGB& image, f32& adjR, f32& adjB)
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
Result CameraParamsController::ComputeExposureAndWhiteBalance(const Vision::ImageRGB& img,
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
  
  Vision::Image weights;
  const bool haveWeights = GetMeteringWeightMask(img.GetNumRows(), img.GetNumCols(), weights);
  
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CameraParamsController::IsExposureValid(s32 exposure) const
{
  const bool inRange = Util::InRange(exposure, GetMinCameraExposureTime_ms(), GetMaxCameraExposureTime_ms());
  if(!inRange)
  {
    PRINT_NAMED_WARNING("CameraParamsController.IsExposureValid.OOR", "Exposure %dms not in range %dms to %dms",
                        exposure, GetMinCameraExposureTime_ms(), GetMaxCameraExposureTime_ms());
  }
  return inRange;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CameraParamsController::IsGainValid(f32 gain) const
{
  const bool inRange = Util::InRange(gain, GetMinCameraGain(), GetMaxCameraGain());
  if(!inRange)
  {
    PRINT_NAMED_WARNING("CameraParamsController.IsGainValid.OOR", "Gain %f not in range %f to %f",
                        gain, GetMinCameraGain(), GetMaxCameraGain());
  }
  return inRange;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CameraParamsController::AreCameraParamsValid(const CameraParams& params) const
{
  if(!IsGainValid(params.gain) ||
     !IsGainValid(params.whiteBalanceGainR) ||
     !IsGainValid(params.whiteBalanceGainG) ||
     !IsGainValid(params.whiteBalanceGainB))
  {
    return false;
  }
  
  if(!IsExposureValid(params.exposureTime_ms))
  {
    return false;
  }
  
  return true;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CameraParamsController::AddMeteringRegion(const Rectangle<s32>& rect)
{
  _meteringRegions.emplace_back(rect);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CameraParamsController::ClearMeteringRegions()
{
  _meteringRegions.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CameraParamsController::GetMeteringWeightMask(const s32 nrows, const s32 ncols, Image& weightMask)
{
  if(_meteringRegions.empty())
  {
    return false;
  }
  
  s32 totalMeteringArea = 0;
  for(auto const& rect : _meteringRegions)
  {
    totalMeteringArea += rect.Area();
  }
  
  DEV_ASSERT(totalMeteringArea >= 0, "CameraParamsController.GetMeteringWeightMask.NegativeROIArea");
    
  const s32 numPixels = nrows*ncols;
  if(2*totalMeteringArea >= numPixels)
  {
    // Detections already make up more than half the image, so they'll already
    // get more focus. Just expose normally (i.e., don't use a weight mask)
    return false;
  }
  
  const u8 backgroundWeight = Util::numeric_cast<u8>(255.f * static_cast<f32>(totalMeteringArea)/static_cast<f32>(numPixels));
  const u8 roiWeight = 255 - backgroundWeight;
  
  weightMask.Allocate(nrows, ncols);
  weightMask.FillWith(backgroundWeight);
  
  for(auto rect : _meteringRegions) // deliberate copy b/c GetROI can modify rect
  {
    weightMask.GetROI(rect).FillWith(roiWeight);
  }
  
  return true;
}
  
} // namespace Vision
} // namespace Anki
