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

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "coretech/messaging/shared/LocalUdpClient.h"

#include "coretech/neuralnets/neuralNetFilenames.h"
#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/neuralnets/neuralNetModel_offboard.h"

#include "util/fileUtils/fileUtils.h"

#include <list>
#include <fstream>
#include <thread>

#define LOG_CHANNEL "NeuralNets"

// Maximum size of one message
#define MAX_PACKET_BUFFER_SIZE 2048

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

  if (false == JsonTools::GetValueOptional(config, JsonKeys::PollingPeriod, _pollPeriod_ms))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.MissingPollPeriod", "");
    return RESULT_FAIL;
  }

  LOG_INFO("OffboardModel.LoadModelInternal.Success",
           "Polling period: %dms, Cache: %s", _pollPeriod_ms, _cachePath.c_str());
  
  JsonTools::GetValueOptional(config, JsonKeys::Verbose, _isVerbose);

  // Overwrite timeout if it's in the neural net config. This is
  // primarily motivated by longer running models
  if (false == JsonTools::GetValueOptional(config, JsonKeys::TimeoutDuration, _timeoutDuration_sec))
  {
    LOG_INFO("OffboardModel.LoadModelInternal.MissingTimeoutDuraction",
             "Keeping timeout at default value, %.3f seconds",
             _timeoutDuration_sec);
  }
  
  std::string commsTypeString;
  if(JsonTools::GetValueOptional(config, JsonKeys::OffboardCommsType, commsTypeString))
  {
    const bool success = Vision::OffboardCommsTypeFromString(commsTypeString, _commsType);
    if(!success)
    {
      LOG_ERROR("OffboardModel.LoadModelInternal.BadCommsType", "%s", commsTypeString.c_str());
      return RESULT_FAIL;
    }
  }
  
  // Anything not talking over FileIO is assumed to be talking over UDP
  if(Vision::OffboardCommsType::FileIO != _commsType)
  {
    _udpClient.reset(new LocalUdpClient());
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  salientPoints.clear();
  
  switch(_commsType)
  {
    case Vision::OffboardCommsType::FileIO:
      return DetectWithFileIO(img, salientPoints);
      
    case Vision::OffboardCommsType::CLAD:
      return DetectWithCLAD(img, salientPoints);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::DetectWithFileIO(const Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Image});
  {
    // Write image to a temporary file
    const std::string tempFilename = Util::FileUtils::FullFilePath({_cachePath, "temp.png"});
    img.Save(tempFilename);
    
    // Write timestamp to file
    const std::string timestampFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Timestamp});
    const std::string timestampString = std::to_string(img.GetTimestamp());
    std::ofstream timestampFile(timestampFilename);
    timestampFile << timestampString;
    timestampFile.close();

    // Rename to what the "offboard processor" expects once the data is fully written (poor man's "lock")
    const bool success = Util::FileUtils::MoveFile(imageFilename, tempFilename);
    if (!success)
    {
      LOG_ERROR("OffboardModel.DetectWithFileIO.FailedToRenameFile",
                "%s -> %s", tempFilename.c_str(), imageFilename.c_str());
      return RESULT_FAIL;
    }

    LOG_DEBUG("OffboardModel.DetectWithFileIO.SavedImageFileForProcessing", "%s t:%d",
              imageFilename.c_str(), img.GetTimestamp());
  }

  // Wait for detection result JSON to appear (blocking until timeout!)
  const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Result});
  Json::Value salientPointsJson;
  const bool resultAvailable = WaitForResultFile(resultFilename, salientPoints);

  // Delete image file (whether we got the result or timed out)
  LOG_DEBUG("NeuralNetRunner.DetectWithFileIO.DeletingImageFile", "%s, deleting %s",
            (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::DetectWithCLAD(const Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  // Until we have a way to send image data via CLAD (or protobuf), just write to file
  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Image});
  const Result saveResult = img.Save(imageFilename);
  if(RESULT_OK != saveResult)
  {
    LOG_ERROR("OffboardModel.DetectWithCLAD.SaveFailed", "%s", imageFilename.c_str());
    return saveResult;
  }
    
  LOG_DEBUG("OffboardModel.DetectWithCLAD.SavedImageFileForProcessing", "%s t:%d",
            imageFilename.c_str(), img.GetTimestamp());
  
  const bool kIsCompressed = true; // We chose to use JPG above
  const bool kIsEncrypted = false;
  Vision::OffboardImageReady imageReadyMsg(img.GetTimestamp(),
                                           img.GetNumRows(),
                                           img.GetNumCols(),
                                           img.GetNumChannels(),
                                           kIsCompressed,
                                           kIsEncrypted,
                                           imageFilename);
  
  const auto expectedSize = imageReadyMsg.Size();
  std::vector<uint8_t> messageData(imageReadyMsg.Size());
  const auto packedSize = imageReadyMsg.Pack(messageData.data(), expectedSize);
  if(!ANKI_VERIFY(packedSize == expectedSize, "OffboardModel.DetectWithCLAD.MessageSizeMismatch",
                  "PackedSize:%zu ExpectedSize:%zu", packedSize, expectedSize))
  {
    return RESULT_FAIL;
  }
  
  const size_t sentSize = _udpClient->Send((const char*)messageData.data(), packedSize);
  if(!ANKI_VERIFY(sentSize == packedSize,
                  "OffboardModel.DetectWithCLAD.SendImageReadyMessageFailed",
                  "MessageSize:%zu Sent:%zu", packedSize, sentSize))
  {
    return RESULT_FAIL;
  }
  
  // Wait for detection result CLAD to appear (blocking until timeout!)
  const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Result});
  const bool resultAvailable = WaitForResultCLAD(salientPoints);
  
  // Delete image file (whether we got the result or timed out)
  LOG_DEBUG("NeuralNetRunner.Detect.DeletingImageFile", "%s, deleting %s",
            (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardModel::WaitForResultFile(const std::string& resultFilename,
                                      std::list<Vision::SalientPoint>& salientPoints)
{
  bool resultAvailable = false;
  const f32 startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  f32 currentTime_sec = startTime_sec;
    
  while( !resultAvailable && (currentTime_sec - startTime_sec < _timeoutDuration_sec) )
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(_pollPeriod_ms));
    resultAvailable = Util::FileUtils::FileExists(resultFilename);
    currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  if(resultAvailable)
  {
    LOG_DEBUG("OffboardModel.DetectWithFileIO.FoundDetectionResultsJSON", "%s",
              resultFilename.c_str());
    
    Json::Reader reader;
    Json::Value detectionResult;
    std::ifstream file(resultFilename);
    const bool parseSuccess = reader.parse(file, detectionResult);
    file.close();
    if(parseSuccess)
    {
      resultAvailable = ParseSalientPointsFromJson(detectionResult, salientPoints);
    }
    else
    {
      LOG_ERROR("OffboardModel.WaitForResultFile.FailedToReadJSON", "%s", resultFilename.c_str());
    }
    
    Util::FileUtils::DeleteFile(resultFilename);
  }
  else
  {
    LOG_WARNING("OffboardModel.WaitForResultFile.PollingForResultTimedOut",
                "Start:%.1fsec Current:%.1f Timeout:%.1fsec",
                startTime_sec, currentTime_sec, _timeoutDuration_sec);
  }
  
  return resultAvailable;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardModel::WaitForResultCLAD(std::list<Vision::SalientPoint>& salientPoints)
{
  bool resultAvailable = false;
  const f32 startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  f32 currentTime_sec = startTime_sec;
  
  while( !resultAvailable && (currentTime_sec - startTime_sec < _timeoutDuration_sec) )
  {
    // TODO: copy-and-pasted from robot comms, baustin is gonna implement this for realz
    while (_udpClient->IsConnected())
    {
      char buf[MAX_PACKET_BUFFER_SIZE];
      const ssize_t n = _udpClient->Recv(buf, sizeof(buf));
      if (n < 0) {
        LOG_ERROR("RobotConnectionManager.ProcessArrivedMessages", "Read error from robot");
        break;
      } else if (n == 0) {
        //LOG_DEBUG("RobotConnectionManager.ProcessArrivedMessages", "Nothing to read");
        break;
      } else {
        //LOG_DEBUG("RobotConnectionManager.ProcessArrivedMessages", "Read %zd/%lu from robot", n, sizeof(buf));

        Vision::OffboardResultReady resultReadyMsg;
        // TODO: turn buf into resultReadyMsg
        // _currentConnectionData->PushArrivedMessage((const uint8_t *) buf, (uint32_t) n, addr);
        Json::Reader reader;
        Json::Value detectionResult;
        const bool parseSuccess = reader.parse(resultReadyMsg.jsonResult, detectionResult);
        if(parseSuccess)
        {
          resultAvailable |= ParseSalientPointsFromJson(detectionResult, salientPoints);
        }
      }
    }

    currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }

  return resultAvailable;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::ParseSalientPointsFromJson(const Json::Value& salientPointsJson,
                                                 std::list<Vision::SalientPoint>& salientPoints)
{
  if(!salientPointsJson.isArray())
  {
    LOG_ERROR("OffboardModel.ParseSalientPointsFromJson.ExpectingArray", "");
    return RESULT_FAIL;
  }
  
  for(auto const& salientPointJson : salientPointsJson)
  {
    Vision::SalientPoint salientPoint;
    const bool success = salientPoint.SetFromJSON(salientPointJson);
    if(!success)
    {
      LOG_ERROR("OffboardModel.ParseSalientPointsFromJson.FailedToSetFromJSON", "");
      continue;
    }
    
    salientPoints.emplace_back(std::move(salientPoint));
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

