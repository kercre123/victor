/**
 * File: imageSaver.cpp
 *
 * Author: Andrew Stein
 * Date:   06/07/2018
 *
 * Description: Class for saving image data according to a variety of parameters.
 *              Can optionally create thumbnails and also undistort images.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/scopedTicToc.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/undistorter.h"
#include "engine/vision/imageSaver.h"
#include "util/fileUtils/fileUtils.h"
#include "util/math/math.h"

#include "opencv2/imgproc/imgproc.hpp"

namespace Anki {
namespace Cozmo {

static const char* kLogChannelName = "VisionSystem";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageSaverParams::ImageSaverParams(const std::string&       pathIn,
                                   Mode                     saveModeIn,
                                   int8_t                   qualityIn,
                                   const std::string&       basenameIn,
                                   Vision::ImageCache::Size sizeIn,
                                   float                    thumbnailScaleIn,
                                   float                    saveScaleIn,
                                   bool                     removeDistortionIn,
                                   uint8_t                  medianFilterSizeIn,
                                   float                    sharpeningAmountIn)
: path(pathIn)
, basename(basenameIn)
, mode(saveModeIn)
, quality(qualityIn)
, size(sizeIn)
, thumbnailScale(thumbnailScaleIn)
, saveScale(saveScaleIn)
, removeDistortion(removeDistortionIn)
, medianFilterSize(medianFilterSizeIn)
, sharpeningAmount(sharpeningAmountIn)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Required here in the .cpp b/c of use of unique_ptr and forward declaration of Undistorter
ImageSaver::ImageSaver() = default;
ImageSaver::~ImageSaver() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImageSaver::SetCalibration(const std::shared_ptr<Vision::CameraCalibration>& camCalib)
{
  if(ANKI_VERIFY(nullptr != camCalib, "ImageSaver.SetCalibration.NullCamCalib", ""))
  {
    _undistorter = std::make_unique<Vision::Undistorter>(camCalib);
  }
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result ImageSaver::CacheUndistortionMaps(s32 nrows, s32 ncols)
  {
    if(!_undistorter)
    {
      PRINT_NAMED_ERROR("ImageSaver.CacheUndistortionMaps.NoUndistorter", "");
      return RESULT_FAIL;
    }
    
    return _undistorter->CacheUndistortionMaps(nrows, ncols);
  }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImageSaver::SetParams(const ImageSaverParams& params)
{
  // Make sure given params are ok
  
  if(params.path.empty())
  {
    PRINT_NAMED_ERROR("ImageSaver.SetParams.EmptyPath", "");
    return RESULT_FAIL;
  }
  
  if(params.quality != -1 && !Util::InRange(params.quality, int8_t(0), int8_t(100)))
  {
    PRINT_NAMED_ERROR("ImageSaver.SetParams.BadQuality", "Should be -1 or [0,100], not %d", params.quality);
    return RESULT_FAIL;
  }
  
  if(!Util::InRange(params.thumbnailScale, 0.f, 1.f))
  {
    PRINT_NAMED_ERROR("ImageSaver.SetParams.BadThumbnailScale", "Should be [0.0, 1.0], not %.3f",
                      params.thumbnailScale);
    return RESULT_FAIL;
  }
  
  if(params.removeDistortion && !_undistorter)
  {
    PRINT_NAMED_ERROR("ImageSaver.SetParams.NeedUndistorter",
                      "Cannot remove distortion unless undistorter is provided");
    return RESULT_FAIL;
  }
  
  if(!Util::IsFltGTZero(params.saveScale))
  {
    PRINT_NAMED_ERROR("ImageSensor.SetParams.InvalidSaveScale",
                      "Save scale should be > 0");
    return RESULT_FAIL;
  }
  
  // Use 'em:
  _params = params;
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImageSaver::WantsToSave() const
{
  return (_params.mode != Mode::Off);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImageSaver::ShouldSaveSensorData() const
{
  return (Mode::SingleShotWithSensorData == _params.mode) || (Mode::Stream == _params.mode);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImageSaver::Save(Vision::ImageCache& imageCache, const s32 frameNumber)
{
  const std::string fullFilename = GetFullFilename(frameNumber, GetExtension(_params.quality));
  
  PRINT_CH_INFO(kLogChannelName, "ImageSaver.Save.SavingImage", "Saving image with timestamp %u to %s",
                imageCache.GetTimeStamp(), fullFilename.c_str());
  
  // Resize into a new image to avoid affecting downstream updates
  Vision::ImageRGB sizedImage = imageCache.GetRGB(_params.size);
  if(_params.removeDistortion)
  {
    // This should have already been checked during SetParams
    DEV_ASSERT(nullptr != _undistorter, "ImageSaver.Save.NoUndistorter");
        
    ScopedTicToc timer("ImageSaver.RemoveDistortion", kLogChannelName);
    Vision::ImageRGB undistortedImage;
    const Result undistortResult = _undistorter->UndistortImage(sizedImage, undistortedImage);
    if(RESULT_OK != undistortResult)
    {
      PRINT_NAMED_ERROR("ImageSaver.Save.UndistortFailed", "");
    } else {
      std::swap(undistortedImage, sizedImage);
    }
  }
  
  if(_params.medianFilterSize > 0)
  {
    ScopedTicToc timer("ImageSaver.MedianFilter", kLogChannelName);
    
    Vision::ImageRGB smoothedImage;
    Result blurResult = RESULT_OK;
    try {
      cv::medianBlur(sizedImage.get_CvMat_(), smoothedImage.get_CvMat_(), _params.medianFilterSize);
    } catch (cv::Exception& e) {
      PRINT_NAMED_ERROR("ImageSaver.Save.OpenCvMedianBlurFailed",
                        "%s (ksize=%d)", e.what(), (int)_params.medianFilterSize);
      blurResult = RESULT_FAIL;
    }
    if(RESULT_OK == blurResult) {
      std::swap(smoothedImage, sizedImage);
    }
  }
  
  if(Util::IsFltGTZero(_params.sharpeningAmount))
  {
    ScopedTicToc timer("ImageSaver.Sharpening", kLogChannelName);
    
    Vision::ImageRGB imgBlur;
    try {
      cv::GaussianBlur(sizedImage.get_CvMat_(), imgBlur.get_CvMat_(), cv::Size(3,3), 1.0);
      cv::addWeighted(sizedImage.get_CvMat_(), 1.0 + _params.sharpeningAmount,
                      imgBlur.get_CvMat_(), -_params.sharpeningAmount, 0.0,
                      sizedImage.get_CvMat_());
    } catch (cv::Exception& e) {
      PRINT_NAMED_ERROR("ImageSaver.Save.SharpenFailed",
                        "%s", e.what());
    }
  }
  
  if(!Util::IsFltNear(_params.saveScale, 1.f))
  {
    sizedImage.Resize(_params.saveScale, Vision::ResizeMethod::Lanczos);
  }
  
  const Result saveResult = sizedImage.Save(fullFilename, _params.quality);
  
  Result thumbnailResult = RESULT_OK;
  if((RESULT_OK == saveResult) && Util::IsFltGTZero(_params.thumbnailScale))
  {
    const std::string fullFilename = GetFullFilename(frameNumber, GetThumbnailExtension(_params.quality));
    Vision::ImageRGB thumbnail;
    sizedImage.Resize(_params.thumbnailScale);
    thumbnailResult = sizedImage.Save(fullFilename);
  }
  
  if((ImageSendMode::SingleShot == _params.mode) || (ImageSendMode::SingleShotWithSensorData == _params.mode))
  {
    _params.mode = ImageSendMode::Off;
  }
  
  if((RESULT_OK != saveResult) || (thumbnailResult != RESULT_OK))
  {
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ImageSaver::GetFullFilename(const s32 frameNumber, const char *extension) const
{
  std::string fullFilename;
  if(_params.basename.empty())
  {
    // No base name provided: Use zero-padded frame number as filename
    char filename[32];
    snprintf(filename, sizeof(filename)-1, "%08d.%s", frameNumber, extension);
    fullFilename = Util::FileUtils::FullFilePath({_params.path, filename});
  }
  else
  {
    // Add the specified extension to the specified base name
    fullFilename = Util::FileUtils::FullFilePath({_params.path, _params.basename + "." + extension});
  }
  
  return fullFilename;
}
  
} // namespace Cozmo
} // namespace Anki


