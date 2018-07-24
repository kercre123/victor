/**
 * File: imageSaver.h
 *
 * Author: Andrew Stein
 * Date:   06/07/2018
 *
 * Description: Class for saving image data according to a variety of parameters.
 *              Can optionally create thumbnails and also undistort images.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Cozmo_Engine_Vision_ImageSaver_H__
#define __Anki_Cozmo_Engine_Vision_ImageSaver_H__

#include "clad/types/imageTypes.h"

namespace Anki {
  
namespace Vision {
  class Camera;
  class ImageCache;
  class ImageRGB;
}
  
namespace Cozmo {

class ImageSaver
{
public:
  
  // NOTE: A calibrated camera is required if removeDistortion=true is used
  ImageSaver(const Vision::Camera& camera);
  
  virtual ~ImageSaver() { }
  
  using Mode = ImageSendMode;
  
  struct Params
  {
    std::string               path;                     // absolute path for output images (including thumbnails)
    std::string               basename;                 // leave empty to use frame number
    Mode                      mode = Mode::Off;
    int8_t                    quality = -1;             // -1 for .png, [0,100] for .jpg quality
    Vision::ImageCache::Size  size = Vision::ImageCache::Size::Full;
    float                     thumbnailScale = 0.f;     // in range [0,1], as fraction of size, 0 to disable
    float                     saveScale = 1.f;          // > 0, as fraction of size
    bool                      removeDistortion = false;
  };
  
  Result SetParams(Params&& params);
  
  // Returns true if the current mode is set such that the saver wants to save an image (SingleShot* or Stream)
  bool WantsToSave() const;
  
  // Returns true if the current mode is SingleShotWithSensorData or Stream
  bool ShouldSaveSensorData() const;
  
  // Return the full filename to use for saving, using the given path and frameNumber/basename, and appending
  // the given extension.
  // If no basename has been provided in params, will use frameNumber. Otherwise frameNumber is ignored.
  std::string GetFullFilename(const s32 frameNumber, const char* extension) const;
  
  // Save the specified size image from the cache and a corresponding thumbnail if requested.
  Result Save(Vision::ImageCache& imageCache, const s32 frameNumber);
  
private: 
  
  const Vision::Camera& _camera;
  Params _params;
  
};

} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Engine_Vision_ImageSaver_H__ */
