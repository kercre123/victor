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

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/imageCache.h"
#include "engine/vision/imageSaver.h"
#include "util/fileUtils/fileUtils.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

static const char* kLogChannelName = "VisionSystem";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImageSaver::ImageSaver(const Vision::Camera& camera)
: _camera(camera)
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ImageSaver::SetParams(Params&& params)
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
  
  if(params.removeDistortion && !_camera.IsCalibrated())
  {
    PRINT_NAMED_ERROR("ImageSaver.SetParams.NeedCalibratedCamera",
                      "Cannot remove disortion unless camera is calibrated");
    return RESULT_FAIL;
  }
  
  // Use 'em:
  std::swap(params, _params);
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
  const std::string fullFilename = GetFullFilename(frameNumber, (_params.quality < 0 ? "png" : "jpg"));
  
  PRINT_CH_INFO(kLogChannelName, "ImageSaver.Save.SavingImage", "Saving to %s", fullFilename.c_str());
  
  // Resize into a new image to avoid affecting downstream updates
  Vision::ImageRGB sizedImage = imageCache.GetRGB(_params.size);
  if(_params.removeDistortion)
  {
    if(ANKI_VERIFY(_camera.IsCalibrated(), "ImageSaver.Save.NoCalibrationForSavingUndistorted", ""))
    {
      Vision::ImageRGB undistortedImage;
      sizedImage.Undistort(*_camera.GetCalibration(), undistortedImage);
      std::swap(undistortedImage, sizedImage);
    }
    else
    {
      return RESULT_FAIL;
    }
  }
  
  const Result saveResult = sizedImage.Save(fullFilename, _params.quality);
  
  Result thumbnailResult = RESULT_OK;
  if((RESULT_OK == saveResult) && Util::IsFltGTZero(_params.thumbnailScale))
  {
    const std::string fullFilename = GetFullFilename(frameNumber, (_params.quality < 0 ? "thm.png" : "thm.jpg"));
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


