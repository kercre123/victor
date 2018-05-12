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

#include "coretech/vision/engine/objectDetector.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <cstdio>
#include <list>
#include <fstream>

// TODO: put this back if/when we start supporting other ObjectDetectorModels
//#if USE_TENSORFLOW
//#  ifndef TENSORFLOW_USE_AOT
//#    error Expecting TENSORFLOW_USE_AOT to be defined by cmake!
//#  elif TENSORFLOW_USE_AOT==1
//#    include "objectDetectorModel_tensorflow_AOT.cpp"
//#  else
//#    include "objectDetectorModel_tensorflow.cpp"
//#  endif
//#elif USE_TENSORFLOW_LITE
//#  include "objectDetectorModel_tensorflow_lite.cpp"
//#elif USE_OPENCV_DNN
//#  include "objectDetectorModel_opencvdnn.cpp"
//#else
#include "objectDetectorModel_standalone.cpp"
//#endif

namespace Anki {
namespace Vision {
 
// Log channel name currently expected to be defined by one of the objectDetectorModel cpp files...
// static const char * const kLogChannelName = "VisionSystem";
 
namespace {
  CONSOLE_VAR(f32,        kObjectDetection_Gamma,                "Vision.ObjectDetector", 1.0f); // set to 1.0 to disable
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector()
: _profiler("ObjectDetector")
, _model(new Model(_profiler))
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Init(const std::string& modelPath, const std::string& cachePath, const Json::Value& config)
{
  Result result = RESULT_OK;
  
  _profiler.Tic("LoadModel");
  result = _model->LoadModel(modelPath, cachePath, config);
  _profiler.Toc("LoadModel");
  
  // Get the input height/width so we can do the resize and only need to share/copy/write as
  // small an image as possible for the standalone CNN process to pick up
  if(false == JsonTools::GetValueOptional(config, "input_height", _processingHeight))
  {
    PRINT_NAMED_ERROR("ObjectDetector.Init.MissingConfig", "input_height");
    return RESULT_FAIL;
  }
  
  if(false == JsonTools::GetValueOptional(config, "input_width", _processingWidth))
  {
    PRINT_NAMED_ERROR("ObjectDetector.Init.MissingConfig", "input_width");
    return RESULT_FAIL;
  }
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Init.LoadModelFailed", "");
    return result;
  }

  PRINT_NAMED_INFO("ObjectDetector.Init.LoadModelTime", "Loading model from '%s' took %.1fsec",
                   modelPath.c_str(), Util::MilliSecToSec(_profiler.AverageToc("LoadModel")));

  _profiler.SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  _profiler.SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());

  _isInitialized = true;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static void ApplyGamma(ImageRGB& img)
{
  static f32 currentGamma = 1.f;
  static std::array<u8,256> gammaLUT{};
  if(!Util::IsFltNear(kObjectDetection_Gamma, currentGamma))
  {
    currentGamma = kObjectDetection_Gamma;
    const f32 gamma = 1.f / currentGamma;
    const f32 divisor = 1.f / 255.f;
    for(s32 value=0; value<256; ++value)
    {
      gammaLUT[value] = std::round(255.f * std::powf((f32)value * divisor, gamma));
    }
  }
  
  s32 nrows = img.GetNumRows();
  s32 ncols = img.GetNumCols();
  if(img.IsContinuous()) {
    ncols *= nrows;
    nrows = 1;
  }
  for(s32 i=0; i<nrows; ++i)
  {
    Vision::PixelRGB* img_i = img.GetRow(i);
    for(s32 j=0; j<ncols; ++j)
    {
      Vision::PixelRGB& pixel = img_i[j];
      pixel.r() = gammaLUT[pixel.r()];
      pixel.g() = gammaLUT[pixel.g()];
      pixel.b() = gammaLUT[pixel.b()];
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectDetector::StartProcessingIfIdle(ImageCache& imageCache)
{
  if(!_isInitialized)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.NotInitialized", "");
    return false;
  }
  
  // If we're not already processing an image with a "future", create one to process this image asynchronously.
  if(!_future.valid())
  {
    // Require color data
    if(!imageCache.HasColor())
    {
      PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "ObjectDetector.Detect.NeedColorData", "");
    }
  
    // Resize to processing size
    _imgBeingProcessed.Allocate(_processingHeight, _processingWidth);
    const ImageCache::Size kImageSize = ImageCache::Size::Full;
    const Vision::ResizeMethod kResizeMethod = Vision::ResizeMethod::Linear;
    imageCache.GetRGB(kImageSize).Resize(_imgBeingProcessed, kResizeMethod);
    
    // Apply gamma since networks are generally trained on gamma-corrected data
    if(!Util::IsFltNear(kObjectDetection_Gamma, 1.f))
    {
      auto ticToc = _profiler.TicToc("Gamma");
      ApplyGamma(_imgBeingProcessed);
    }
    
    // Store its size relative to original size so we can rescale object detections later
    _heightScale = (f32)imageCache.GetOrigNumRows() / (f32)_imgBeingProcessed.GetNumRows();
    _widthScale  = (f32)imageCache.GetOrigNumCols() / (f32)_imgBeingProcessed.GetNumCols();
    
    PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.ProcessingImage", "Detecting objects in %dx%d image t=%u",
                  _imgBeingProcessed.GetNumCols(), _imgBeingProcessed.GetNumRows(), _imgBeingProcessed.GetTimestamp());
    
    _future = std::async(std::launch::async, [this]() {
      std::list<DetectedObject> objects;
  
      _profiler.Tic("Model.Run");
      Result result = _model->Run(_imgBeingProcessed, objects);
      _profiler.Toc("Model.Run");
      if(RESULT_OK != result)
      {
        PRINT_NAMED_WARNING("ObjectDetector.Detect.AsyncLambda.ModelRunFailed", "");
      }
    
      return objects;
    });
    
    // We did start processing the given image
    return true;
  }
  
  // We were not idle, so did not start processing the new image
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ObjectDetector::GetObjects(std::list<DetectedObject>& objects_out)
{
  // Always clear the output list, so it's definitely only populated once the
  // future is ready below
  objects_out.clear();

  if(_future.valid())
  {
    // Check the future's status and keep waiting until it's ready.
    // Once it's ready, return the result.
    const auto kWaitForTime = std::chrono::microseconds(500);
    const std::future_status futureStatus = _future.wait_for(kWaitForTime);
    if(std::future_status::ready == futureStatus)
    {
      std::list<DetectedObject> objects = _future.get();
      DEV_ASSERT(!_future.valid(), "ObjectDetector.Detect.FutureStillValid");
      
      // The detection will be at the processing resolution. Need to convert it to original resolution.
      if( !Util::IsNear(_widthScale, 1.f) || !Util::IsNear(_heightScale,1.f))
      {
        
        std::for_each(objects.begin(), objects.end(), [this](DetectedObject& object)
                      {
                        object.rect = Rectangle<s32>(std::round((f32)object.rect.GetX() * _widthScale),
                                                     std::round((f32)object.rect.GetY() * _heightScale),
                                                     std::round((f32)object.rect.GetWidth() * _widthScale),
                                                     std::round((f32)object.rect.GetHeight() * _heightScale));
                      });
      }
      
      if(ANKI_DEV_CHEATS)
      {
        if(objects.empty())
        {
          PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.NoObjects", "t=%ums", _imgBeingProcessed.GetTimestamp());
        }
        for(auto const& object : objects)
        {
          PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.FoundObject", "t=%ums Name:%s Score:%.3f",
                        _imgBeingProcessed.GetTimestamp(), object.name.c_str(), object.score);
        }
      }
      
      // Put the result from the future into the output argument and let the caller know via the Status
      std::swap(objects, objects_out);
      return true;
    }
  }
  
  return false;
}
  
} // namespace Vision
} // namespace Anki
