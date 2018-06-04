/**
 * File: objectDetectorModel_messenger.cpp
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: Implementation of ObjectDetector Model class does not actually run forward inference through
 *              a neural network model but instead communicates with a standalone process that does so.
 *
 * Copyright: Anki, Inc. 2018
 **/

// TODO: put this back if/when we start supporting other ObjectDetectorModels
// // The contents of this file are only used when the build is using *neither* TF or TF Lite
// #if (!defined(USE_TENSORFLOW) || !USE_TENSORFLOW) && (!defined(USE_TENSORFLOW_LITE) || !USE_TENSORFLOW_LITE)

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

  Result Run(const ImageRGB& img, std::list<SalientPoint>& objects);
  
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
  // NOTE: This implementation of an ObjectDetector::Model does not actual load any model as it is communicating
  //       with a standalone process which is loading and running the model. Here we'll just set any inititalization
  //       parameters for communicating with that process.
  
  _cachePath = cachePath;

  // Clear the cache of any stale images/results:
  Util::FileUtils::RemoveDirectory(_cachePath);
  Util::FileUtils::CreateDirectory(_cachePath);

  if (false == JsonTools::GetValueOptional(config, "pollPeriod_ms", _pollPeriod_ms))
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingPollPeriod", "");
    return RESULT_FAIL;
  }

  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadModel.Success", 
                "Polling period: %dms, Cache: %s", _pollPeriod_ms, _cachePath.c_str());

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<SalientPoint>& objects)
{
  objects.clear();

  // Profiling will be from time we write file to when we get results
  auto totalTicToc = _profiler.TicToc("ObjectDetector.Model.Run");
  
  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, "objectDetectionImage.png"});
  {
    // Write image to a temporary file
    auto writeTicToc = _profiler.TicToc("ObjectDetector.Model.Run.WriteImage");
    const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, "temp.png"});
    img.Save(tempFilename);
    
    // Write timestamp to file
    const std::string timestampFilename = Util::FileUtils::FullFilePath({_cachePath, "timestamp.txt"});
    const std::string timestampString = std::to_string(img.GetTimestamp());
    std::ofstream timestampFile(timestampFilename);
    timestampFile << timestampString;
    timestampFile.close();

    // Rename to what vic-neuralnets process expects once the data is fully written (poor man's "lock")
    const bool success = (rename(tempFilename.c_str(), imageFilename.c_str()) == 0);
    if (!success)
    {
      PRINT_NAMED_ERROR("StandaloneInferenceProcess.FailedToRenameFile",
                        "%s -> %s", tempFilename.c_str(), imageFilename.c_str());
      return RESULT_FAIL;
    }

    PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.SavedImageFileForProcessing", "%s t:%d",
                 imageFilename.c_str(), img.GetTimestamp());
  }

  // Wait for detection result JSON to appear
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

  // Delete image file (whether we got the result or timed out)
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
      // Translate JSON into a DetectedObject struct and put it in the output
      const Json::Value& detectedObjects = detectionResult["objects"];
      if(detectedObjects.isArray())
      {
        for(auto const& object : detectedObjects)
        {
          SalientPoint salientPoint;
          const bool success = salientPoint.SetFromJSON(object);
          if(!success)
          {
            PRINT_NAMED_ERROR("ObjectDetector.Model.FailedToSetFromJSON", "");
            continue;
          }
          
          objects.emplace_back(std::move(salientPoint));
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

