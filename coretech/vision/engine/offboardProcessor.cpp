/**
 * File: offboardProcessor.cpp
 *
 * Author: Andrew Stein
 * Date:   4/1/2019
 *
 * Description: See header file.
 *
 * Copyright: Anki, Inc. 2019
 **/
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/common/shared/types.h"

#include "coretech/messaging/shared/LocalUdpClient.h"
#include "coretech/messaging/shared/socketConstants.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/offboardProcessor.h"
#include "coretech/vision/engine/parseSalientPointsFromJson.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <fstream>
#include <thread>

// Maximum size of one message
#define MAX_PACKET_BUFFER_SIZE 2048


#define LOG_CHANNEL "NeuralNets" // TODO: Different channel?

namespace Anki {
namespace Vision {
  
const char* const OffboardProcessor::Filenames::Timestamp = "timestamp.txt";
const char* const OffboardProcessor::Filenames::Image     = "offboardProcImage.png";
const char* const OffboardProcessor::Filenames::Result    = "offboardProcResult.json";
  
const char* const OffboardProcessor::JsonKeys::CommsType       = "commsType";
const char* const OffboardProcessor::JsonKeys::ProcType        = "procType";
const char* const OffboardProcessor::JsonKeys::TimeoutDuration = "timeoutDuration_sec";
const char* const OffboardProcessor::JsonKeys::PollingPeriod   = "pollPeriod_ms";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OffboardProcessor::OffboardProcessor(const std::string& cachePath)
: _cachePath(cachePath)
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOTE: Not using "default" in the header b/c of forward-declared LocalUdpClient
OffboardProcessor::~OffboardProcessor()
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardProcessor::Init(const std::string& name, const Json::Value& config)
{
  // NOTE: This implementation of an INeuralNetRunner::Model does not actual load any model as it is communicating
  //       with a standalone process which is loading and running the model. Here we'll just set any inititalization
  //       parameters for communicating with that process.

  _params.name = name;
  
  if (false == JsonTools::GetValueOptional(config, JsonKeys::PollingPeriod, _params.pollingPeriod_ms))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.MissingPollPeriod", "");
    return RESULT_FAIL;
  }
  
  if (false == JsonTools::GetValueOptional(config, JsonKeys::TimeoutDuration, _params.timeoutDuration_sec))
  {
    LOG_INFO("OffboardModel.LoadModelInternal.MissingTimeoutDuraction",
             "Keeping timeout at default value, %.3f seconds",
             _params.timeoutDuration_sec);
  }
  
  std::string commsTypeString;
  if(false == JsonTools::GetValueOptional(config, JsonKeys::CommsType, commsTypeString))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.MissingCommsType", "");
    return RESULT_FAIL;
  }
  else
  {
    const bool success = OffboardCommsTypeFromString(commsTypeString, _params.commsType);
    if(!success)
    {
      LOG_ERROR("OffboardProcessor.LoadModelInternal.BadCommsType", "%s", commsTypeString.c_str());
      return RESULT_FAIL;
    }
  }
  
  if(!config.isMember(JsonKeys::ProcType))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.MissingOffboardProcType", "");
    return RESULT_FAIL;
  }
  
  const Json::Value& jsonProcTypes = config[JsonKeys::ProcType];
  if(jsonProcTypes.isArray())
  {
    for(auto const& jsonProcType : jsonProcTypes)
    {
      _params.procTypes.emplace_back(JsonTools::GetValue<std::string>(jsonProcType));
    }
  }
  else
  {
    _params.procTypes.emplace_back(JsonTools::GetValue<std::string>(jsonProcTypes));
  }
  
  // Sanity check the inputs
  if(_params.procTypes.empty())
  {
    LOG_ERROR("OffboardProcessor.Init.EmptyProcTypes", "");
    return RESULT_FAIL;
  }
  if(_params.pollingPeriod_ms <= 0)
  {
    LOG_ERROR("OffboardProcessor.Init.BadPollPeriod", "%d", _params.pollingPeriod_ms);
    return RESULT_FAIL;
  }
  if(Util::IsFltLEZero(_params.timeoutDuration_sec))
  {
    LOG_ERROR("OffboardProcessor.Init.BadTimeout", "%f", _params.timeoutDuration_sec);
    return RESULT_FAIL;
  }
  if(_params.name.empty())
  {
    LOG_ERROR("OffboardProcessor.Init.EmptyName", "");
    return RESULT_FAIL;
  }
  
  _cachePath = Util::FileUtils::FullFilePath({_cachePath, _params.name});
  
  LOG_INFO("OffboardProcessor.Init.Success",
           "Polling period: %dms, Cache: %s", _params.pollingPeriod_ms, _cachePath.c_str());
  
  // Anything not talking over FileIO is assumed to be talking over UDP
  if( (OffboardCommsType::FileIO != _params.commsType) && _udpClient == nullptr)
  {
    if(!ANKI_DEV_CHEATS)
    {
      LOG_ERROR("OffboardProcessor.LoadModelInternal.UDPCommsNotAllowedToShip", "");
      return RESULT_FAIL;
    }
    
    _udpClient.reset(new LocalUdpClient());
#ifdef VICOS
    // Right now we only support vicos for offboard vision per the implementation
    // in vic-cloud, if you're running on mac ... there isn't really a use case for
    // offboard ... yet.
    const bool connected = Connect();
    LOG_INFO("OffboardProcessor.Init.ConnectionStatus", "%d", connected);
#endif
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardProcessor::Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  salientPoints.clear();
  
  _imageTimestamp = img.GetTimestamp();
  _imageRows = img.GetNumRows();
  _imageCols = img.GetNumCols();
  
  switch(_params.commsType)
  {
    case OffboardCommsType::FileIO:
      return DetectWithFileIO(img, salientPoints);
      
    case OffboardCommsType::CLAD:
    {
      // If we're not yet connected, keep trying to connect
      bool connected = IsConnected();
      if(!connected)
      {
        connected = Connect();
      }
      
      if(connected)
      {
        return DetectWithCLAD(img, salientPoints);
      }
      else
      {
        LOG_WARNING("OffboardProcessor.Detect.StillNotConnected", "Will keep trying");
        return RESULT_FAIL;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardProcessor::DetectWithFileIO(const ImageRGB& img, std::list<SalientPoint>& salientPoints)
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
      LOG_ERROR("OffboardProcessor.DetectWithFileIO.FailedToRenameFile",
                "%s -> %s", tempFilename.c_str(), imageFilename.c_str());
      return RESULT_FAIL;
    }
    
    LOG_DEBUG("OffboardProcessor.DetectWithFileIO.SavedImageFileForProcessing", "%s t:%d",
              imageFilename.c_str(), img.GetTimestamp());
  }
  
  // Wait for detection result JSON to appear (blocking until timeout!)
  const std::string resultFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Result});
  Json::Value salientPointsJson;
  const bool resultAvailable = WaitForResultFile(resultFilename, salientPoints);
# pragma unused(resultAvailable)
  
  // Delete image file (whether we got the result or timed out)
  LOG_DEBUG("NeuralNetRunner.DetectWithFileIO.DeletingImageFile", "%s, deleting %s",
            (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardProcessor::DetectWithCLAD(const ImageRGB& img, std::list<SalientPoint>& salientPoints)
{
  // Until we have a way to send image data via CLAD (or protobuf), just write to file
  const std::string imageFilename = Util::FileUtils::FullFilePath({_cachePath, Filenames::Image});
  const Result saveResult = img.Save(imageFilename);
  if(RESULT_OK != saveResult)
  {
    LOG_ERROR("OffboardProcessor.DetectWithCLAD.SaveFailed", "%s", imageFilename.c_str());
    return saveResult;
  }
  
  LOG_DEBUG("OffboardProcessor.DetectWithCLAD.SavedImageFileForProcessing", "%s t:%d",
            imageFilename.c_str(), img.GetTimestamp());
  
  const bool kIsCompressed = true; // We chose to use JPG above
  const bool kIsEncrypted = false;
  Vision::OffboardImageReady imageReadyMsg(img.GetTimestamp(),
                                           img.GetNumRows(),
                                           img.GetNumCols(),
                                           img.GetNumChannels(),
                                           kIsCompressed,
                                           kIsEncrypted,
                                           _params.procTypes,
                                           imageFilename);
  
  const auto expectedSize = imageReadyMsg.Size();
  std::vector<uint8_t> messageData(imageReadyMsg.Size());
  const auto packedSize = imageReadyMsg.Pack(messageData.data(), expectedSize);
  if(!ANKI_VERIFY(packedSize == expectedSize, "OffboardProcessor.DetectWithCLAD.MessageSizeMismatch",
                  "PackedSize:%zu ExpectedSize:%zu", packedSize, expectedSize))
  {
    return RESULT_FAIL;
  }
  
  const size_t sentSize = _udpClient->Send((const char*)messageData.data(), packedSize);
  if(!ANKI_VERIFY(sentSize == packedSize,
                  "OffboardProcessor.DetectWithCLAD.SendImageReadyMessageFailed",
                  "MessageSize:%zu Sent:%zu", packedSize, sentSize))
  {
    return RESULT_FAIL;
  }
  
  // Wait for detection result CLAD to appear (blocking until timeout!)
  const bool resultAvailable = WaitForResultCLAD(salientPoints);
# pragma unused(resultAvailable)
  
  // Delete image file (whether we got the result or timed out)
  LOG_INFO("NeuralNetRunner.Detect.DeletingImageFile", "%s, deleting %s",
           (resultAvailable ? "Result found" : "Polling timed out"), imageFilename.c_str());
  Util::FileUtils::DeleteFile(imageFilename);
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardProcessor::WaitForResultFile(const std::string& resultFilename,
                                          std::list<SalientPoint>& salientPoints)
{
  bool resultAvailable = false;
  const f32 startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  f32 currentTime_sec = startTime_sec;
  
  while( !resultAvailable && (currentTime_sec - startTime_sec < _params.timeoutDuration_sec) )
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(_params.pollingPeriod_ms));
    resultAvailable = Util::FileUtils::FileExists(resultFilename);
    currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  if(resultAvailable)
  {
    LOG_DEBUG("OffboardProcessor.DetectWithFileIO.FoundDetectionResultsJSON", "%s",
              resultFilename.c_str());
    
    Json::Reader reader;
    Json::Value detectionResult;
    std::ifstream file(resultFilename);
    const bool parseSuccess = reader.parse(file, detectionResult);
    file.close();
    if(parseSuccess)
    {
      resultAvailable = ParseSalientPointsFromJson(detectionResult,
                                                   _imageRows, _imageCols, _imageTimestamp,
                                                   salientPoints);
    }
    else
    {
      LOG_ERROR("OffboardProcessor.WaitForResultFile.FailedToReadJSON", "%s", resultFilename.c_str());
    }
    
    Util::FileUtils::DeleteFile(resultFilename);
  }
  else
  {
    LOG_WARNING("OffboardProcessor.WaitForResultFile.PollingForResultTimedOut",
                "Start:%.1fsec Current:%.1f Timeout:%.1fsec",
                startTime_sec, currentTime_sec, _params.timeoutDuration_sec);
  }
  
  return resultAvailable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardProcessor::WaitForResultCLAD(std::list<SalientPoint>& salientPoints)
{
  bool resultAvailable = false;
  const f32 startTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  f32 currentTime_sec = startTime_sec;
  
  while( !resultAvailable && (currentTime_sec - startTime_sec < _params.timeoutDuration_sec) )
  {
    while (_udpClient->IsConnected())
    {
      LOG_PERIODIC_INFO(100, "OffboardProcessor.WaitForResultCLAD.CheckingForMessage", "");
      
      char buf[MAX_PACKET_BUFFER_SIZE];
      const ssize_t n = _udpClient->Recv(buf, sizeof(buf));
      if (n < 0) {
        LOG_ERROR("OffboardProcessor.WaitForResultCLAD.ReceiveError", "");
        break;
      } else if (n == 0) {
        LOG_PERIODIC_INFO(100, "OffboardProcessor.WaitForResultCLAD.NothingToRead", "");
        std::this_thread::sleep_for(std::chrono::milliseconds(_params.pollingPeriod_ms));
        break;
      } else {
        LOG_INFO("OffboardProcessor.WaitForResultCLAD.ProcessMessage", "Read %zu/%zu", n, sizeof(buf));
        
        OffboardResultReady resultReadyMsg{(const uint8_t*)buf, (size_t)n};
        
        if(resultReadyMsg.timestamp != _imageTimestamp)
        {
          LOG_INFO("OffboardProcessor.WaitForResultCLAD.DumpingOldResult",
                   "Found result with t:%u, instead of t:%u. Dropping on floor and continuing to wait.",
                   resultReadyMsg.timestamp, _imageTimestamp);
          continue;
        }
        
        Json::Reader reader;
        Json::Value detectionResult;
        const bool parseSuccess = reader.parse(resultReadyMsg.jsonResult, detectionResult);
        if(parseSuccess)
        {
          std::string keys;
          if(detectionResult.isArray())
          {
            keys = "<array>";
          }
          else
          {
            for(const auto& key : detectionResult.getMemberNames())
            {
              keys += key + " ";
            }
          }
          LOG_INFO("OffboardProcessor.WaitForResultCLAD.ParsedMessageJson", "Keys:%s", keys.c_str());
          const Result parseResult = ParseSalientPointsFromJson(detectionResult,
                                                                _imageRows, _imageCols, _imageTimestamp,
                                                                salientPoints);
          resultAvailable |= (parseResult == RESULT_OK);
        }
      }
    }
    
    currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
  
  return resultAvailable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardProcessor::Connect()
{
  static s32 numTries = 0;
  
  const char* serverPath = LOCAL_SOCKET_PATH "offboard_vision_server";
  const std::string clientPath = std::string(LOCAL_SOCKET_PATH) + "_" + _params.name;
  
  const bool udpSuccess = _udpClient->Connect(clientPath, serverPath);
  ++numTries;
  
  LOG_INFO("OffboardProcessor.Connect.Status", "Try %d: %s - Server:%s Client:%s",
           numTries, (udpSuccess ? "Success" : "Fail"), serverPath, clientPath.c_str());
  
  return udpSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardProcessor::IsConnected() const
{
  if(ANKI_VERIFY(nullptr != _udpClient, "OffboardProcessor.IsConnected.NoUdpClient", "UDP client should be instantiated"))
  {
    return _udpClient->IsConnected();
  }
  return false;
}
  
} // namespace Vision
} // namespace Anki

