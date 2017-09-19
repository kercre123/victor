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

#include "anki/vision/basestation/objectDetector.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/imageCache.h"
#include "anki/vision/basestation/profiler.h"

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <list>
#include <fstream>

#define USE_TENSORFLOW 0

#if USE_TENSORFLOW
#  include "objectDetectorModel_tensorflow.cpp"
#else
#  include "objectDetectorModel_opencvdnn.cpp"
#endif

namespace Anki {
namespace Vision {
 
// Currently expecte to be defined by one of the objectDetectorModel cpp files...
// static const char * const kLogChannelName = "VisionSystem";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector()
: _model(new Model())
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Init(const std::string& modelPath, const Json::Value& config)
{
  Result result = _model->LoadModel(modelPath, config);
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(ImageCache&           imageCache,
                              std::list<DetectedObject>&  objects)
{
  Result result = RESULT_OK;
  
  objects.clear();
  
  // Require color data
  if(imageCache.HasColor())
  {
    const ImageRGB& img = imageCache.GetRGB(ImageCache::Size::Full);
    
    result = _model->Run(img, objects);
    
    const f32 widthScale = (f32)imageCache.GetOrigNumRows() / (f32)img.GetNumRows();
    const f32 heightScale = (f32)imageCache.GetOrigNumCols() / (f32)img.GetNumCols();
    
    if(!Util::IsNear(widthScale, 1.f) || !Util::IsNear(heightScale,1.f))
    {
      //const f32 widthScale  = (f32)imageCache.GetOrigNumCols() / (f32)img.GetNumCols();
      std::for_each(objects.begin(), objects.end(), [widthScale,heightScale](DetectedObject& object)
                    {
                      object.rect = Rectangle<s32>(std::round((f32)object.rect.GetX() * widthScale),
                                                   std::round((f32)object.rect.GetY() * heightScale),
                                                   std::round((f32)object.rect.GetWidth() * widthScale),
                                                   std::round((f32)object.rect.GetHeight() * heightScale));
                    });
    }
    
    if(ANKI_DEV_CHEATS)
    {
      if(objects.empty())
      {
        PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.NoObjects", "t=%ums", imageCache.GetRGB().GetTimestamp());
      }
      for(auto const& object : objects)
      {
        PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.FoundObject", "t=%ums Name:%s Score:%.3f",
                      imageCache.GetRGB().GetTimestamp(), object.name.c_str(), object.score);
      }
    }
  }
  else
  {
    PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "ObjectDetector.Detect.NeedColorData", "");
  }
  
  return result;
}
  
} // namespace Vision
} // namespace Anki
