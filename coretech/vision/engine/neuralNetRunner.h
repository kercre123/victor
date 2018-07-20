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
#include "coretech/common/engine/math/rect.h"

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
namespace Vision {

class ImageCache;

class NeuralNetRunner
{
public:
  
  NeuralNetRunner();
  ~NeuralNetRunner();
  
  // Load a DNN model and assocated labels specified by the "graph" and "labels" fields of config.
  // Supports either TensorFlow or Caffe models using the following conventions:
  //  - If specified graph ends in ".pb", it is assumed to be TensorFlow
  //  - If there is also a ".pbtxt" file of the same name, it is used as well. Otherwise
  //    the detector attempts to interpret the graph entirely from the frozen ".pb" file.
  //  - Otherwise, the model is assumed to be Caffe and both <graph>.prototxt and <graph>.caffemodel are
  //    assumed to exist and used to read the model.
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
  
  // Hide the library/implementation we actually use for detecting objects
  class Model;
  std::unique_ptr<Model> _model;
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
  
}; // class NeuralNetworkRunner
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetRunner_H__ */
