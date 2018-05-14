/**
 * File: objectDetectorModel_standalone.cpp
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: Implementation of ObjectDetector Model class which wrapps OpenCV's DNN module.
 *
 * Copyright: Anki, Inc. 2018
 **/

// TODO: put this back if/when we start supporting other ObjectDetectorModels
//// The contents of this file are only used when the build is using *neither* TF or TF Lite
//#if (!defined(USE_TENSORFLOW) || !USE_TENSORFLOW) && (!defined(USE_TENSORFLOW_LITE) || !USE_TENSORFLOW_LITE)

#include "coretech/vision/engine/objectDetector.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <list>
#include <fstream>

namespace Anki {
namespace Vision {

namespace {
  CONSOLE_VAR_RANGED(f32, kObjectDetection_TimeoutDuration_sec,  "Vision.ObjectDetector", 10.f, 1., 15.f);
}
  
static const char * const kLogChannelName = "VisionSystem";
  
// Useful just for printing every frame, since detection is slow, even through the profiler
// already has settings for printing based on time. Set to 0 to disable.
CONSOLE_VAR(s32, kObjectDetector_PrintTimingFrequency, "Vision.ObjectDetector", 1);

class ObjectDetector::Model
{
public:

  Model(Profiler& profiler);
  
  Result LoadModel(const std::string& modelPath, const std::string& cachePath, const Json::Value& config);

  Result Run(const ImageRGB& img, std::list<ObjectDetector::DetectedObject>& objects);
  
private:
  
  std::string _cachePath;
  int         _pollPeriod_ms;
  
  Profiler& _profiler;
  
}; // class Model
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::Model::Model(Profiler& profiler) 
: _profiler(profiler) 
{ 

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::LoadModel(const std::string& modelPath, const std::string& cachePath, const Json::Value& config)
{
  _cachePath = cachePath;

  // Clear the cache of any stale images/results:
  Util::FileUtils::RemoveDirectory(_cachePath);
  Util::FileUtils::CreateDirectory(_cachePath);

  if(!config.isMember("poll_period_ms")) 
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingPollPeriod", "");
    return RESULT_FAIL;
  }

  _pollPeriod_ms = config["poll_period_ms"].asInt();

  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadModel.Success", 
                "Polling period: %dms, Cache: %s", _pollPeriod_ms, _cachePath.c_str());

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<ObjectDetector::DetectedObject>& objects)
{
  objects.clear();

  // Profiling will be from time we write file to when we get results
  auto totalTicToc = _profiler.TicToc("ObjectDetector.Model.Run");
  
  // Write image to a temporary file
  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, "objectDetectionImage.png"});
  {
    auto writeTicToc = _profiler.TicToc("ObjectDetector.Model.Run.WriteImage");
    const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, "temp.png"});
    img.Save(tempFilename);
    
    const std::string timestampFilename = Util::FileUtils::FullFilePath({_cachePath, "timestamp.txt"});
    const std::string timestampString = std::to_string(img.GetTimestamp());
    std::ofstream timestampFile(timestampFilename);
    timestampFile << timestampString;
    timestampFile.close();

    // Rename to what standalone process expects once the data is fully written (poor man's "lock")
    if(0 != rename(tempFilename.c_str(), imageFilename.c_str()))
    {
      PRINT_NAMED_ERROR("StandaloneInferenceProcess.FailedToRenameFile",
                        "%s -> %s", tempFilename.c_str(), imageFilename.c_str());
      return RESULT_FAIL;
    }

    PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.SavedImageFileForProcessing", "%s t:%d",
                 imageFilename.c_str(), img.GetTimestamp());
  }

  // Wait for result JSON to appear
  const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, "objectDetectionResults.json"});
  bool resultAvailable = false;
  f32 startTime_sec = 0.f, currentTime_sec = 0.f;
  {
    auto pollTicToc = _profiler.TicToc("StandaloneInferenceProcess.Polling");
    startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    currentTime_sec = startTime_sec;
    
    while( !resultAvailable && (currentTime_sec - startTime_sec < kObjectDetection_TimeoutDuration_sec) )
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(_pollPeriod_ms));
      resultAvailable = Util::FileUtils::FileExists(resultFilename);
      currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    }
  }

  // Delete image file (whether or not we got the result or timed out)
  PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Detect.DeletingImageFile", "%s, deleting %s",
                 (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  if(resultAvailable)
  {
    auto readTicToc = _profiler.TicToc("StandaloneInferenceProcess.ReadingResult");
    
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
          
          const int xmin = std::round(object["xmin"].asFloat() * img.GetNumCols());
          const int ymin = std::round(object["ymin"].asFloat() * img.GetNumRows());
          const int xmax = std::round(object["xmax"].asFloat() * img.GetNumCols());
          const int ymax = std::round(object["ymax"].asFloat() * img.GetNumRows());
          
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
  else
  {
    PRINT_NAMED_WARNING("ObjectDetector.Model.PollingForResultTimedOut",
                        "Start:%.1fsec Current:%.1f Timeout:%.1fsec",
                        startTime_sec, currentTime_sec, kObjectDetection_TimeoutDuration_sec);
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

//#endif // #if !defined(USE_TENSORFLOW) || !USE_TENSORFLOW

