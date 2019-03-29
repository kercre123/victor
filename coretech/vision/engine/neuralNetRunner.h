/**
 * File: objectDetector.h
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Asynchronous wrapper for running forward inference with a deep neural network on an image.
 *              Currently supports detection of SalientPoints, but API could be extended to get other
 *              types of outputs later.
 *
 *              Abstracts away the private implementation around what kind of inference engine is used
 *              and runs asynchronously since forward inference through deep networks is generally "slow".
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Vision_IAsyncRunner_H__
#define __Anki_Vision_IAsyncRunner_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/shared/math/rect.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "clad/types/salientPointTypes.h"

#include <future>
#include <list>

// Forward declaration:
namespace Json {
  class Value;
}

namespace Anki {
  
namespace NeuralNets {
  class INeuralNetModel;
}
  
namespace Vision {

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
  
protected:
  
  virtual Result InitInternal(const std::string& cachePath, const Json::Value& config) = 0;
  
  Profiler& GetProfiler() { return _profiler; }
  const std::string& GetCachePath() const { return _cachePath; }
  
  virtual bool IsVerbose() const = 0;
  
  virtual std::list<SalientPoint> Run(ImageRGB& img) = 0;
  
private:
  
  Profiler _profiler;
  
  // Use a std::future as the mechenism for asynchronous processing
  std::future<std::list<SalientPoint>> _future;

  // Stores a copy of the "original" image, from which the processed image is resized,
  // which can be used for saving or chained processing
  ImageRGB              _imgOrig;
  
  // Copy of the image data (at processing resolution) to be processed by the virtual Run() method
  ImageRGB              _imgBeingProcessed;
  
  std::string           _cachePath;
  std::string           _visualizationDirectory;
  bool                  _isInitialized = false;
  s32                   _processingWidth;
  s32                   _processingHeight;
  
  bool StartProcessingHelper();
  
}; // class IAsyncRunner
  
class NeuralNetRunner : public IAsyncRunner
{
public:
  
  NeuralNetRunner(const std::string& modelPath);
  virtual ~NeuralNetRunner();
  
protected:
  
  virtual Result InitInternal(const std::string& cachePath, const Json::Value& config) override;
  virtual bool IsVerbose() const override;
  virtual std::list<SalientPoint> Run(ImageRGB& img) override;
  
private:
  std::string _modelPath;
  std::unique_ptr<NeuralNets::INeuralNetModel> _model;
};
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_IAsyncRunner_H__ */
