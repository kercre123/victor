/**
 * File: neuralNetModel_offboard.cpp
 *
 * Author: Andrew Stein
 * Date:   3/8/2018
 *
 * Description: See header file.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/shared/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "coretech/neuralnets/neuralNetFilenames.h"
#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"

#include "util/fileUtils/fileUtils.h"

#include <list>
#include <fstream>
#include <thread>

#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace NeuralNets {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffboardModel::OffboardModel(const std::string& cachePath)
: _cachePath(cachePath)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::LoadModelInternal(const std::string& modelPath, const Json::Value& config)
{
  // NOTE: This implementation of an INeuralNetRunner::Model does not actual load any model as it is communicating
  //       with a standalone process which is loading and running the model. Here we'll just set any inititalization
  //       parameters for communicating with that process.
  
  _cachePath = Util::FileUtils::FullFilePath({_cachePath, GetName()});

  if (false == JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::PollingPeriod, _pollPeriod_ms))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.MissingPollPeriod", "");
    return RESULT_FAIL;
  }

  LOG_INFO("OffboardModel.LoadModelInternal.Success",
           "Polling period: %dms, Cache: %s", _pollPeriod_ms, _cachePath.c_str());
  
  JsonTools::GetValueOptional(config, "verbose", _isVerbose);

  // Overwrite timeout if it's in the neural net config. This is
  // primarily motivated by longer running models
  if (false == JsonTools::GetValueOptional(config, NeuralNets::JsonKeys::TimeoutDuration,
                                           _timeoutDuration_sec))
  {
    LOG_INFO("OffboardModel.LoadModelInternal.MissingTimeoutDuraction",
             "Keeping timeout at default value, %.3f seconds",
             _timeoutDuration_sec);
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  salientPoints.clear();

  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, NeuralNets::Filenames::Image});
  {
    // Write image to a temporary file
    const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, "temp.png"});
    img.Save(tempFilename);
    
    // Write timestamp to file
    const std::string timestampFilename = Util::FileUtils::FullFilePath({_cachePath, NeuralNets::Filenames::Timestamp});
    const std::string timestampString = std::to_string(img.GetTimestamp());
    std::ofstream timestampFile(timestampFilename);
    timestampFile << timestampString;
    timestampFile.close();

    // Rename to what the "offboard processor" expects once the data is fully written (poor man's "lock")
    const bool success = Util::FileUtils::MoveFile(imageFilename, tempFilename);
    if (!success)
    {
      LOG_ERROR("OffboardModel.Detect.FailedToRenameFile",
                "%s -> %s", tempFilename.c_str(), imageFilename.c_str());
      return RESULT_FAIL;
    }

    LOG_DEBUG("OffboardModel.Detect.SavedImageFileForProcessing", "%s t:%d",
              imageFilename.c_str(), img.GetTimestamp());
  }

  // Wait for detection result JSON to appear
  const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, NeuralNets::Filenames::Result});
  bool resultAvailable = false;
  f32 startTime_sec = 0.f, currentTime_sec = 0.f;
  {
    startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    currentTime_sec = startTime_sec;
    
    while( !resultAvailable && (currentTime_sec - startTime_sec < _timeoutDuration_sec) )
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(_pollPeriod_ms));
      resultAvailable = Util::FileUtils::FileExists(resultFilename);
      currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    }
  }

  // Delete image file (whether we got the result or timed out)
  LOG_DEBUG("NeuralNetRunner.Detect.DeletingImageFile", "%s, deleting %s",
            (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  if(resultAvailable)
  {
    LOG_DEBUG("OffboardModel.Detect.FoundDetectionResultsJSON", "%s",
              resultFilename.c_str());
    
    Json::Reader reader;
    Json::Value detectionResult;
    std::ifstream file(resultFilename);
    const bool success = reader.parse(file, detectionResult);
    if(!success)
    {
      LOG_ERROR("OffboardModel.Detect.FailedToReadJSON", "%s", resultFilename.c_str());
    }
    else
    {
      // Translate JSON into a SalientPoint and put it in the output
      const Json::Value& salientPointsJson = detectionResult["salientPoints"];
      if(salientPointsJson.isArray())
      {
        for(auto const& salientPointJson : salientPointsJson)
        {
          Vision::SalientPoint salientPoint;
          const bool success = salientPoint.SetFromJSON(salientPointJson);
          if(!success)
          {
            LOG_ERROR("OffboardModel.Detect.FailedToSetFromJSON", "");
            continue;
          }
          
          salientPoints.emplace_back(std::move(salientPoint));
        }
      }
    }
    file.close();
    Util::FileUtils::DeleteFile(resultFilename);
  }
  else
  {
    LOG_WARNING("OffboardModel.Detect.PollingForResultTimedOut",
                "Start:%.1fsec Current:%.1f Timeout:%.1fsec",
                startTime_sec, currentTime_sec, _timeoutDuration_sec);
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

