/**
 * File: asyncRunnerInterface.h
 *
 * Author: Andrew Stein
 * Date:   4/1/2019
 *
 * Description: Interface for running a detection task asynchronously. The usage model on each update tick is:
 *               - poll for any new "detections", in the form of SalientPoints, and then
 *               - start processing a new image if idle
 *              Images are provided to the asynchronous processor at "processing" resolution defined by Json config.
 *              Implementations of this interface are expecting to define a Run() method which takes an image
 *               and produces a list of SalientPoints.
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "clad/types/salientPointTypes.h"

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/profiler.h"
#include "coretech/vision/engine/image_fwd.h"

#include <future>
#include <list>

#ifndef __Anki_Vision_AsyncRunnerInterface_H__
#define __Anki_Vision_AsyncRunnerInterface_H__

namespace Anki {
namespace Vision {

// Forward delcarations
class ImageCache;
  
class IAsyncRunner
{
public:
  
  IAsyncRunner();
  virtual ~IAsyncRunner();
  
  // Load a DNN model described by Json config
  Result Init(const std::string& cachePath, const Json::Value& config);
  
  // Returns true if image was used, false if otherwise occupied or image wasn't suitable (e.g., not color)
  bool StartProcessingIfIdle(ImageCache& imageCache);
  bool StartProcessingIfIdle(const Vision::ImageRGB& img);
  
  // Returns true if processing of the last image provided using StartProcessingIfIdle is complete
  // and populates salientPoints with any detections.
  bool GetDetections(std::list<SalientPoint>& salientPoints);
  
  // Example usage:
  //
  //  if(GetDetections(salientPoints)) {
  //    <do stuff with salientPoints>
  //  }
  //
  //  StartProcessingIfIdle(imageCache);
  //
  
  const ImageRGB& GetOrigImg() const { return _imgOrig; }
  s32 GetProcessingWidth()  const { return _processingWidth; }
  s32 GetProcessingHeight() const { return _processingHeight; }
  
  struct JsonKeys {
    static const char* const InputHeight;
    static const char* const InputWidth;
  };
  
protected:
  
  virtual Result InitInternal(const std::string& cachePath, const Json::Value& config) = 0;
  
  Profiler& GetProfiler() { return _profiler; }
  const std::string& GetCachePath() const { return _cachePath; }
  
  virtual bool IsVerbose() const = 0;
  
  virtual std::list<SalientPoint> Run(ImageRGB& img) = 0;
  
private:
  
  Profiler _profiler;
  
  // Use a std::future as the mechanism for asynchronous processing
  std::future<std::list<SalientPoint>> _future;
  
  // Stores a copy of the "original" image, from which the processed image is resized,
  // which can be used for saving or chained processing
  ImageRGB              _imgOrig;
  
  // Copy of the image data (at processing resolution) to be processed by the virtual Run() method
  ImageRGB              _imgBeingProcessed;
  
  std::string           _cachePath;
  bool                  _isInitialized = false;
  s32                   _processingWidth;
  s32                   _processingHeight;
  
  bool StartProcessingHelper();
  
}; // class IAsyncRunner

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_AsyncRunnerInterface_H__ */
