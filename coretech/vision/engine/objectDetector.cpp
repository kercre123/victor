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
//#else
#  include "objectDetectorModel_opencvdnn.cpp"
//#endif

namespace Anki {
namespace Vision {
 
// Log channel name currently expected to be defined by one of the objectDetectorModel cpp files...
// static const char * const kLogChannelName = "VisionSystem";
 
namespace {
  CONSOLE_VAR(bool, kObjectDetection_UseTensorFlowProcess, "Vision.ObjectDetector", false);
  CONSOLE_VAR(f32,  kObjectDetection_Gamma,                "Vision.ObjectDetector", 1.0f); // set to 1.0 to disable
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
  _profiler.Tic("LoadModel");
  const Result result = _model->LoadModel(modelPath, config);
  _profiler.Toc("LoadModel");
  
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Init.LoadModelFailed", "");
    return result;
  }

  PRINT_NAMED_INFO("ObjectDetector.Init.LoadModelTime", "Loading model from '%s' took %.1fsec",
                   modelPath.c_str(), Util::MilliSecToSec(_profiler.AverageToc("LoadModel")));

  _profiler.SetPrintFrequency(config.get("ProfilingPrintFrequency_ms", 10000).asUInt());
  _profiler.SetDasLogFrequency(config.get("ProfilingEventLogFrequency_ms", 10000).asUInt());

  _cachePath = cachePath;
  if(kObjectDetection_UseTensorFlowProcess)
  {
    // Clear the cache on startup
    Util::FileUtils::RemoveDirectory(_cachePath);
    Util::FileUtils::CreateDirectory(_cachePath);
  }
  
  _isInitialized = true;
  return result;
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
    if(imageCache.HasColor())
    {
      // This is just the size to grab from cache to copy to the asynchronous processing call.
      // It will still be resized by the actual detector implementation to the exact size specified
      // in the params.
      // TODO: resize here instead of the Model, to copy less data
      // TODO: avoid copying data entirely somehow? (but maybe who cares b/c it's tiny and fwd inference time likely dwarfs the copy)
      const ImageCache::Size kImageSize = ImageCache::Size::Full;
      
      // Grab a copy of the image
      imageCache.GetRGB(kImageSize).CopyTo(_imgBeingProcessed);
      
      // Use gamma to make it easier to see
      // TODO: Do this on the final image size, so we don't waste time applying gamma unnecessarily
      if(!Util::IsFltNear(kObjectDetection_Gamma, 1.f))
      {
        auto ticToc = _profiler.TicToc("Gamma");

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

        s32 nrows = _imgBeingProcessed.GetNumRows();
        s32 ncols = _imgBeingProcessed.GetNumCols();
        if(_imgBeingProcessed.IsContinuous()) {
          ncols *= nrows;
          nrows = 1;
        }
        for(s32 i=0; i<nrows; ++i)
        {
          Vision::PixelRGB* img_i = _imgBeingProcessed.GetRow(i);
          for(s32 j=0; j<ncols; ++j)
          {
            Vision::PixelRGB& pixel = img_i[j];
            pixel.r() = gammaLUT[pixel.r()];
            pixel.g() = gammaLUT[pixel.g()];
            pixel.b() = gammaLUT[pixel.b()];
          }
        }
      }
      
      // Store its size relative to original size so we can rescale object detections later
      _widthScale = (f32)imageCache.GetOrigNumRows() / (f32)_imgBeingProcessed.GetNumRows();
      _heightScale = (f32)imageCache.GetOrigNumCols() / (f32)_imgBeingProcessed.GetNumCols();
      
      PRINT_NAMED_INFO("ObjectDetector.Detect.ProcessingImage", "Detecting objects in %dx%d image t=%u",
                       _imgBeingProcessed.GetNumCols(), _imgBeingProcessed.GetNumRows(), _imgBeingProcessed.GetTimestamp());
      
      _future = std::async(std::launch::async, [this]() {
        std::list<DetectedObject> objects;
        if(kObjectDetection_UseTensorFlowProcess)
        {
          // Profiling will be from time we write file to when we get results
          _profiler.Tic("StandaloneInferenceProcess");
          
          // Write image to a temporary file
          _profiler.Tic("StandaloneInferenceProcess.WriteImage");
          const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, "temp.png"});
          _imgBeingProcessed.Save(tempFilename);
          
          // Rename to what TF process expects once the data is fully written (poor man's "lock")
          const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, "objectDetectionImage.png"});
          const std::string cmd = "mv " + tempFilename + " " + imageFilename;
          system(cmd.c_str());
          _profiler.Toc("StandaloneInferenceProcess.WriteImage");

          PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.SavedImageFileForProcessing", "%s t:%d",
                         imageFilename.c_str(), _imgBeingProcessed.GetTimestamp());
          
          _profiler.Tic("StandaloneInferenceProcess.Polling");
          const u32 kPollPeriod_ms = 10;
          const f32 timeoutDuration_sec = 10.f;
          const f32 starttTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          f32 currentTime_sec = starttTime_sec;
          
          // Wait for result JSON to appear
          const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, "objectDetectionResults.json"});
          bool resultAvailable = false;
          while( !resultAvailable && (currentTime_sec - starttTime_sec < timeoutDuration_sec) )
          {
            std::this_thread::sleep_for(std::chrono::milliseconds(kPollPeriod_ms));
            resultAvailable = Util::FileUtils::FileExists(resultFilename);
          }
          _profiler.Toc("StandaloneInferenceProcess.Polling");

          // Delete image file (whether or not we got the result or timed out)
          Util::FileUtils::DeleteFile(imageFilename);
          
          if(resultAvailable)
          {
            auto ticToc = _profiler.TicToc("StandaloneInferenceProcess.ReadingResult");
            
            PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.FoundDetectionResultsJSON", "%s",
                           resultFilename.c_str());
            
            Json::Reader reader;
            Json::Value detectionResult;
            std::ifstream file(resultFilename);
            const bool success = reader.parse(file, detectionResult);
            if(!success)
            {
              PRINT_NAMED_ERROR("ObjectDetector.Model.FailedToReadJSON", "%s", resultFilename.c_str());
            }
            else
            {
              const Json::Value& detectedObjects = detectionResult["objects"];
              if(detectedObjects.isArray())
              {
                for(auto const& object : detectedObjects)
                {
                  DEV_ASSERT(object.isMember("xmin") && object.isMember("xmax") &&
                             object.isMember("ymin") && object.isMember("ymax"),
                             "ObjectDetector.Model.MissingJsonFieldsXY");
                  
                  const int xmin = std::round(object["xmin"].asFloat() * _imgBeingProcessed.GetNumCols());
                  const int ymin = std::round(object["ymin"].asFloat() * _imgBeingProcessed.GetNumRows());
                  const int xmax = std::round(object["xmax"].asFloat() * _imgBeingProcessed.GetNumCols());
                  const int ymax = std::round(object["ymax"].asFloat() * _imgBeingProcessed.GetNumRows());
                  
                  DEV_ASSERT(object.isMember("timestamp"), "ObjectDetector.Model.MissingJsonFieldTimestamp");
                  DEV_ASSERT(object.isMember("score"),     "ObjectDetector.Model.MissingJsonFieldScore");
                  DEV_ASSERT(object.isMember("name"),      "ObjectDetector.Model.MissingJsonFieldName");
                  
                  objects.emplace_back(DetectedObject{
                    .timestamp = object["timestamp"].asUInt(),
                    .score = object["score"].asFloat(),
                    .name = object["name"].asString(),
                    .rect = Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin)
                  });
                }
              }
            }
            file.close();
            Util::FileUtils::DeleteFile(resultFilename);
          }
          
          _profiler.Toc("StandaloneInferenceProcess");      
        }
        else
        {
          _profiler.Tic("Inference");
          Result result = _model->Run(_imgBeingProcessed, objects);
          _profiler.Toc("Inference");
          if(RESULT_OK != result)
          {
            PRINT_NAMED_WARNING("ObjectDetector.Detect.AsyncLambda.ModelRunFailed", "");
          }
        }
        return objects;
      });
      
      return true;
    }
    else
    {
      PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "ObjectDetector.Detect.NeedColorData", "");
    }
  }
  
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
