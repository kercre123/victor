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

#if USE_TENSORFLOW
#  ifndef TENSORFLOW_USE_AOT
#    error Expecting TENSORFLOW_USE_AOT to be defined by cmake!
#  elif TENSORFLOW_USE_AOT==1
#    include "objectDetectorModel_tensorflow_AOT.cpp"
#  else
#    include "objectDetectorModel_tensorflow.cpp"
#  endif
#elif USE_TENSORFLOW_LITE
#  include "objectDetectorModel_tensorflow_lite.cpp"
#else
#  include "objectDetectorModel_opencvdnn.cpp"
#endif

namespace Anki {
namespace Vision {
 
// Currently expecte to be defined by one of the objectDetectorModel cpp files...
// static const char * const kLogChannelName = "VisionSystem";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector()
: Profiler("ObjectDetector")
, _model(new Model(*this))
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Init(const std::string& modelPath, const Json::Value& config)
{
  Tic("LoadModel");
  const Result result = _model->LoadModel(modelPath, config);
  if(RESULT_OK == result)
  {
    _isInitialized = true;
  }
  Toc("LoadModel");

  PRINT_NAMED_INFO("ObjectDetector.Init.LoadModelTime", "Loading model '%s' took %.1fsec", 
                   modelPath.c_str(), Util::MilliSecToSec(AverageToc("LoadModel")));

  SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());

  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::Status ObjectDetector::Detect(ImageCache& imageCache, std::list<DetectedObject>& objects_out)
{
  if(!_isInitialized)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.NotInitialized", "");
    return Status::Error;
  }
  
  objects_out.clear();
    
  const ImageCache::Size kImageSize = ImageCache::Size::Full;
  const bool kCropCenterSquare = false;
  
  if(!_future.valid())
  {
    // Require color data
    if(imageCache.HasColor())
    {
      const ImageRGB& img = imageCache.GetRGB(kImageSize);
     
      // We're going to copy the data into a separate image to be 
      // processed asynchronously
      // TODO: resize here instead of the Model, to copy less data
      // TODO: avoid copying data entirely somehow?
      static ImageRGB imgToProcess;
      
      if(kCropCenterSquare)
      {
        const s32 squareDim = std::min(img.GetNumRows(), img.GetNumCols());
        _upperLeft.x() = img.GetNumCols()/2 - squareDim/2;
        _upperLeft.y() = img.GetNumRows()/2 - squareDim/2;
        Rectangle<s32> centerCrop(_upperLeft.x(), _upperLeft.y(),
                                  squareDim, squareDim);
        
        const ImageRGB& imgCenterCrop = img.GetROI(centerCrop);
        imgCenterCrop.CopyTo(imgToProcess);
      }
      else
      {
        _upperLeft.x() = 0;
        _upperLeft.y() = 0;
        img.CopyTo(imgToProcess);
      }

      PRINT_NAMED_INFO("ObjectDetector.Detect.ProcessingImage", "Detecting objects in %dx%d image t=%u", 
                       imgToProcess.GetNumCols(), imgToProcess.GetNumRows(), imgToProcess.GetTimestamp());

      _future = std::async(std::launch::async, [this](const ImageRGB& img) {
        std::list<DetectedObject> objects;
        Tic("Inference");
        Result result = _model->Run(img, objects);
        Toc("Inference");
        if(RESULT_OK != result)
        {
          PRINT_NAMED_WARNING("ObjectDetector.Detect.AsyncLambda.ModelRunFailed", "");
        }
        return objects;
      }, imgToProcess);
    }
    else
    {
      PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "ObjectDetector.Detect.NeedColorData", "");
      return Status::Idle;
    } 
  }

  const auto kWaitForTime = std::chrono::microseconds(500);
  const std::future_status futureStatus = _future.wait_for(kWaitForTime);
  switch(futureStatus)
  {
    case std::future_status::deferred:
    case std::future_status::timeout:
      return Status::Processing;

    case std::future_status::ready:
    {
      const ImageRGB& img = imageCache.GetRGB(kImageSize);

      std::list<DetectedObject> objects = _future.get();
      
      const f32 widthScale = (f32)imageCache.GetOrigNumRows() / (f32)img.GetNumRows();
      const f32 heightScale = (f32)imageCache.GetOrigNumCols() / (f32)img.GetNumCols();
      
      if( kCropCenterSquare || !Util::IsNear(widthScale, 1.f) || !Util::IsNear(heightScale,1.f))
      {
        std::for_each(objects.begin(), objects.end(), [this,widthScale,heightScale](DetectedObject& object)
                      {
                        object.rect = Rectangle<s32>(std::round((f32)(object.rect.GetX() + _upperLeft.x()) * widthScale),
                                                     std::round((f32)(object.rect.GetY() + _upperLeft.y()) * heightScale),
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

      std::swap(objects, objects_out);
      return Status::ResultReady;
    }
  }
}
  
} // namespace Vision
} // namespace Anki
