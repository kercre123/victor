/**
 * File: objectDetector.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 **/


#ifndef __Anki_Vision_ObjectDetector_H__
#define __Anki_Vision_ObjectDetector_H__

#include "anki/common/types.h"
#include "anki/common/basestation/math/rect.h"

#include "anki/vision/basestation/profiler.h"

#include <future>
#include <list>

// Forward declaration:
namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {

class ImageCache;

class ObjectDetector : Profiler
{
public:
  
  ObjectDetector();
  ~ObjectDetector();
  
  Result Init(const std::string& modelPath, const Json::Value& config);
  
  struct DetectedObject
  {
    TimeStamp_t     timestamp;
    float           score;
    std::string     name;
    Rectangle<s32>  rect;
  };
  
  enum class Status {
    Idle,
    Processing,
    ResultReady,
    Error,
  };

  Status Detect(ImageCache& imageCache, std::list<DetectedObject>& objects);
  
  //void EnableDisplay(bool enabled);
  
  //void PrintTiming();
  
private:
  
  // Hide the library/implementation we actually use for detecting objects
  class Model;
  std::unique_ptr<Model> _model;
  std::future<std::list<DetectedObject>> _future;

  bool    _isInitialized = false;
  Point2i _upperLeft;

}; // class ObjectDetector
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_ObjectDetector_H__ */
