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

#ifndef __Anki_Vision_NeuralNetRunner_H__
#define __Anki_Vision_NeuralNetRunner_H__

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

class NeuralNetRunner
{
public:
  
  NeuralNetRunner();
  ~NeuralNetRunner();
  
  // Load a DNN model described by Json config
  Result Init(const std::string& modelPath, const std::string& cachePath, const Json::Value& config);
  
  // Returns true if image was used, false if otherwise occupied or image wasn't suitable (e.g., not color)
  bool StartProcessingIfIdle(ImageCache& imageCache);
  
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
  
private:
  
  Profiler _profiler;
  
  std::unique_ptr<NeuralNets::INeuralNetModel> _model;
  std::future<std::list<SalientPoint>> _future; // for processing aysnchronously

  // We process asynchronsously, so need a copy of the image data (at processing resolution)
  ImageRGB              _imgBeingProcessed;
  
  std::string           _cachePath;
  std::string           _visualizationDirectory;
  bool                  _isInitialized = false;
  s32                   _processingWidth;
  s32                   _processingHeight;
  f32                   _widthScale = 1.f;
  f32                   _heightScale = 1.f;
  f32                   _currentGamma;
  std::array<u8,256>    _gammaLUT{};
  
  void ApplyGamma(ImageRGB& img);
  
  std::list<SalientPoint> RunModel();
  
}; // class NeuralNetworkRunner
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetRunner_H__ */
