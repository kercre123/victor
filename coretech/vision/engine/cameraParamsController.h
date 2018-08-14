/**
 * File: cameraParamsController.h
 *
 * Author: Andrew Stein
 * Date:   10/11/2016
 *
 *
 * Description: Auto-exposure and white balance control.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Coretech_Vision_CameraParamsController_H__
#define __Anki_Coretech_Vision_CameraParamsController_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/imageBrightnessHistogram.h"

#include "clad/types/cameraParams.h"

#include <array>
#include <list>
#include <vector>

namespace Anki {
namespace Vision {

class Image;

class CameraParamsController
{
public:
  
  CameraParamsController(u16 minExpTime_ms, u16 maxExpTime_ms, f32 minGain, f32 maxGain,
                         const CameraParams& initialParams);
  
  // Set parameters used for computing exposure (maxChangeFraction and subSample also used for white balance)
  Result SetExposureParameters(const u8               targetValue,
                               const std::vector<u8>& cyclingTargetValues,
                               const f32              targetPercentile,
                               const f32              maxChangeFraction,
                               const s32              subSample);
  
  // Set parameters for determining ImageQuality (Too Bright / Too Dark)
  // - Image is too bright if the value of the current image at the tooBrightPercentile
  //    is greater than the tooBrightValue.
  // - Image is too dark if the value of the current image at the tooDarkPercentile
  //    is less than the tooDarkValue.
  Result SetImageQualityParameters(const u8 tooDarkValue,   const f32 tooDarkPercentile,
                                   const u8 tooBrightValue, const f32 tooBrightPercentile);
  
  enum class AutoExpMode : u8 {
    Off,
    MinTime,      // Prefer keeping exposure time low to avoid motion blur
    MinGain,      // Prefer keeping gain low to make less noisy/grainy images
  };
  
  enum class WhiteBalanceMode : u8 {
    Off,
    GrayWorld,
  };
  
  // Give half the weight for exposure to the metering regions, the other half to the rest of the image
  void AddMeteringRegion(const Rectangle<s32>& rect); // TODO: Add ability to specify weight
  void ClearMeteringRegions();
  
  // Compute "next" exposure and white balance, using color image. (Green channel is used for exposure.)
  Result ComputeNextCameraParams(const ImageRGB& img,
                                 const AutoExpMode autoExpMode,
                                 const WhiteBalanceMode wbMode,
                                 const bool useExposureCycling,
                                 CameraParams& nextParams);
  
  // Just computes exposure, given a grayscale image
  Result ComputeNextCameraParams(const Image& img,
                                 const AutoExpMode autoExpMode,
                                 const bool useExposureCycling,
                                 CameraParams& nextParams);
  
  // Actual update the "current" camera parameters, presumably after using the "next" params
  // above to update the real camera's settings
  Result UpdateCurrentCameraParams(const CameraParams& newParams);
  
  Result SetGammaTable(const std::vector<u8>& inputs, const std::vector<u8>& gamma);
  
  Result Linearize(const Vision::Image& image, Vision::Image& linearizedImage) const;
  
  // Get histogram computed in last ComputeExposureAdjustment() call
  const ImageBrightnessHistogram& GetHistogram() const { return _hist; }
  
  s32 GetMinCameraExposureTime_ms() const { return _minExposureTime_ms; }
  s32 GetMaxCameraExposureTime_ms() const { return _maxExposureTime_ms; }
  
  f32 GetMinCameraGain() const { return _minGain; }
  f32 GetMaxCameraGain() const { return _maxGain; }
  
  bool AreCameraParamsValid(const CameraParams& params) const;
  
  const CameraParams& GetNextCameraParams() const { return _currentCameraParams; }
  ImageQuality GetImageQuality() const { return _imageQuality; }
  
private:
  
  // These are parameters of the specific camera and passed in on construction
  const u16 _minExposureTime_ms;
  const u16 _maxExposureTime_ms;
  const f32 _minGain;
  const f32 _maxGain;
  
  // These are the camera parameters / image quality based on last update
  CameraParams _currentCameraParams;
  ImageQuality _imageQuality = ImageQuality::Good;
  
  // These are configurable parameters controlling how auto exposure and white
  // balancing are done
  u8        _targetValue              = 128;
  f32       _targetPercentile         = 0.5f;
  f32       _maxChangeFraction        = 0.5f;
  s32       _subSample                = 1;
  
  // Parameters for determining image "quality"
  u8        _tooDarkValue;
  u8        _tooBrightValue;
  f32       _tooDarkPercentile;
  f32       _tooBrightPercentile;
  
  AutoExpMode _aeMode = AutoExpMode::MinTime;
  
  std::vector<u8>                   _cyclingTargetValues;
  std::vector<u8>::const_iterator   _cycleTargetIter;
  
  ImageBrightnessHistogram _hist;
  
  std::array<u8,256> _gammaLUT;
  std::array<u8,256> _linearizationLUT;
  
  std::list<Rectangle<s32>> _meteringRegions;
  
  bool GetMeteringWeightMask(const s32 nrows, const s32 ncols, Image& weightMask);
  
  // Compute multiplicative amount by which to adjust exposure (whether time or gain)
  Result ComputeExposureAdjustment(const Vision::Image& image,
                                   const bool useCycling,
                                   f32& adjustmentFraction);
  
  Result ComputeExposureAdjustment(const std::vector<Vision::Image>& imageROIs, f32& adjustmentFraction);
  
  // Computes multiplicative amount by which to adjust red, and blue gains for WhiteBalance
  // - Caller can update gains via: newGainX = adjX * oldGainX
  // - Green gain is assumed to be fixed
  // - Uses the gray world assumption
  Result ComputeWhiteBalanceAdjustment(const Vision::ImageRGB& image, f32& adjR, f32& adjB);
  
  // Simultaneously compute exposure and white balance adjustment from a color image
  Result ComputeExposureAndWhiteBalance(const Vision::ImageRGB& img,
                                        const bool useCycling,
                                        f32 &adjExp, f32& adjR, f32& adjB);
  
  Result ComputeAdjustmentFraction(const bool useCycling, f32& fraction);
  
  Result ComputeNextCameraParamsHelper(const AutoExpMode aeMode,
                                       const WhiteBalanceMode wbMode,
                                       f32 exposureAdjFrac, f32 adjR, f32 adjB,
                                       CameraParams& nextParams);
  
  bool IsGainValid(f32 gain) const;
  bool IsExposureValid(s32 exposure) const;
};
  
} // namespace Vision
} // namespace Anki

#endif //__Anki_Coretech_Vision_CameraParamsController_H__
