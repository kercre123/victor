/**
<<<<<<< HEAD
 * File: objectDetector.h
=======
 * File: objectDetector.cpp
>>>>>>> * Add ObjectDetector with OpenCV DNN Model support (as private implementation)
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Asynchronous wrapper for a deep-learning based object detector / scene classifier.
 *              Abstracts away the private implementation around what kind of inference engine is used
 *              and runs asynchronously since forward inference through deep networks is generally "slow".
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Vision_ObjectDetector_H__
#define __Anki_Vision_ObjectDetector_H__

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/rect.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include <future>
#include <list>

// Forward declaration:
namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {

class ImageCache;

class ObjectDetector
{
public:
  
  ObjectDetector();
  ~ObjectDetector();
  
  // Load a DNN model and assocated labels specified by the "graph" and "labels" fields of config.
  // Supports either TensorFlow or Caffe models using the following conventions:
  //  - If specified graph ends in ".pb", it is assumed to be TensorFlow
  //  - If there is also a ".pbtxt" file of the same name, it is used as well. Otherwise
  //    the detector attempts to interpret the graph entirely from the frozen ".pb" file.
  //  - Otherwise, the model is assumed to be Caffe and both <graph>.prototxt and <graph>.caffemodel are
  //    assumed to exist and used to read the model.
  Result Init(const std::string& modelPath, const Json::Value& config);
  
  struct DetectedObject
  {
    TimeStamp_t     timestamp;
    float           score;
    std::string     name;
    Rectangle<s32>  rect;
  };

  // Returns true if image was used, false if otherwise occupied or image wasn't suitable (e.g., not color)
  bool StartProcessingIfIdle(ImageCache& imageCache);
  
  // Returns true if processing of the last image provided using StartProcessingIfIdle is complete
  // and populates objects with any detections.
  bool GetObjects(std::list<DetectedObject>& objects);
  
  // Example usage:
  //
  //  if(GetObjects(objects)) {
  //    <do stuff with objects>
  //  }
  //
  //  StartProcessingIfIdle(imageCache);
  //
  
private:
  
  Profiler _profiler;
  
  // Hide the library/implementation we actually use for detecting objects
  class Model;
  std::unique_ptr<Model> _model;
  std::future<std::list<DetectedObject>> _future; // for processing aysnchronously

  // We process asynchronsously, so need a copy of the image data
  Vision::ImageRGB _imgBeingProcessed;
  
  bool    _isInitialized = false;
  f32     _widthScale = 1.f;
  f32     _heightScale = 1.f;
  
}; // class ObjectDetector
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_ObjectDetector_H__ */
