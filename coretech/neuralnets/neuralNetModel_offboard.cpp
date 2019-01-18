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
#include "coretech/messaging/shared/socketConstants.h"

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
  if(Vision::OffboardCommsType::FileIO != _commsType && _udpClient == nullptr)
  {
    _udpClient.reset(new LocalUdpClient());
    
    const bool connected = Connect();
    LOG_INFO("OffboardModel.LoadModelInternal.ConnectionStatus", "%d", connected);
  }
  
  // Use the graphFile from params as the processing type
  // TODO: Support multiple procTypes (comma-delimited?)
  _procTypes.resize(1);
  if(!Vision::OffboardProcTypeFromString(_params.graphFile, _procTypes[0]))
  {
    LOG_ERROR("OffboardModel.LoadModelInternal.BadOffboardProcType",
              "Could not get OffboardProcType(s) from graphFile: %s",
              _params.graphFile.c_str());
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints)
{
  salientPoints.clear();
  
  _imageTimestamp = img.GetTimestamp();
  _imageRows = img.GetNumRows();
  _imageCols = img.GetNumCols();
  
  switch(_commsType)
  {
    case Vision::OffboardCommsType::FileIO:
      return DetectWithFileIO(img, salientPoints);
      
    case Vision::OffboardCommsType::CLAD:
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
        LOG_WARNING("OffboardModel.Detect.StillNotConnected", "Will keep trying");
        return RESULT_FAIL;
      }
    }
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
# pragma unused(resultAvailable)

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
                                           _procTypes,
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
# pragma unused(resultAvailable)
  
  // Delete image file (whether we got the result or timed out)
  LOG_INFO("NeuralNetRunner.Detect.DeletingImageFile", "%s, deleting %s",
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
      Vision::OffboardProcType procType = _procTypes[0];
      std::string procTypeStr;
      if(JsonTools::GetValueOptional(detectionResult, "processingType", procTypeStr))
      {
        if(!Vision::OffboardProcTypeFromString(procTypeStr, procType))
        {
          LOG_ERROR("OffboardModel.WaitForResultFile.BadProcessingType", "Could not parse %s. Assuming %s.",
                    procTypeStr.c_str(), EnumToString(procType));
        }
      }
      resultAvailable = ParseSalientPointsFromJson(detectionResult, procType, salientPoints);
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
    while (_udpClient->IsConnected())
    {
      LOG_PERIODIC_INFO(100, "OffboardModel.WaitForResultCLAD.CheckingForMessage", "");
      
      char buf[MAX_PACKET_BUFFER_SIZE];
      const ssize_t n = _udpClient->Recv(buf, sizeof(buf));
      if (n < 0) {
        LOG_ERROR("OffboardModel.WaitForResultCLAD.ReceiveError", "");
        break;
      } else if (n == 0) {
        LOG_PERIODIC_INFO(100, "OffboardModel.WaitForResultCLAD.NothingToRead", "");
        std::this_thread::sleep_for(std::chrono::milliseconds(_pollPeriod_ms));
        break;
      } else {
        LOG_INFO("OffboardModel.WaitForResultCLAD.ProcessMessage", "Read %zu/%zu", n, sizeof(buf));

        Vision::OffboardResultReady resultReadyMsg{(const uint8_t*)buf, (size_t)n};
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
          LOG_INFO("OffboardModel.WaitForResultCLAD.ParsedMessageJson", "Keys:%s", keys.c_str());
          const Result parseResult = ParseSalientPointsFromJson(detectionResult, resultReadyMsg.procType, salientPoints);
          resultAvailable |= (parseResult == RESULT_OK);
        }
      }
    }

    currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }

  return resultAvailable;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::ParseSalientPointsFromJson(const Json::Value& detectionResult,
                                                 const Vision::OffboardProcType procType,
                                                 std::list<Vision::SalientPoint>& salientPoints)
{
  switch(procType)
  {
    case Vision::OffboardProcType::SceneDescription:
      return ParseSceneDescriptionFromJson(detectionResult, salientPoints);
      
    case Vision::OffboardProcType::ObjectDetection:
      return ParseObjectDetectionsFromJson(detectionResult, salientPoints);
      
    case Vision::OffboardProcType::FaceRecognition:
      return ParseFaceDataFromJson(detectionResult, salientPoints);
      
    default:
    {
      // Assume the detectionResult just contains an array of SalientPoints in Json format
      if(!detectionResult.isMember(NeuralNets::JsonKeys::SalientPoints))
      {
        LOG_ERROR("OffboardModel.ParseSalientPointsFromJson.MissingSalientPointsArray",
                  "%s", NeuralNets::JsonKeys::SalientPoints);
        return RESULT_FAIL;
      }
      
      const Json::Value& salientPointsJson = detectionResult[NeuralNets::JsonKeys::SalientPoints];
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
      break;
    }
  } // switch(procType)
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::ParseSceneDescriptionFromJson(const Json::Value& jsonSalientPoints,
                                                    std::list<Vision::SalientPoint>& salientPoints)
{
  // Assumes Json structure like this is present in detectionResult:
  //      "description":{
  //        "captions":[
  //          {
  //            "text":"a man sitting in front of a laptop",  <-- this is the scene description
  //            "confidence":0.8529333419354393               <-- this is the "score"
  //          }
  //        ]
  //      }
  bool gotSalientPoint = false;
  if(jsonSalientPoints.isMember("description"))
  {
    const Json::Value& descriptionJson = jsonSalientPoints["description"];
    if(descriptionJson.isMember("captions"))
    {
      const Json::Value& captionsJson = descriptionJson["captions"];
      
      Vision::SalientPoint salientPoint(_imageTimestamp,
                                        0.5f, 0.5f,
                                        0.f, 1.f,
                                        Vision::SalientPointType::SceneDescription,
                                        "<no description>",
                                        {}, 0);
      
      if(captionsJson.empty())
      {
        // If there are no captions, fall back on a comma separated list of tags
        if(descriptionJson.isMember("tags"))
        {
          const Json::Value& tagsJson = descriptionJson["tags"];
          
          if(tagsJson.isArray() && !tagsJson.empty())
          {
            auto iter = tagsJson.begin();
            salientPoint.description = iter->asString();
            ++iter;
            while(iter != tagsJson.end())
            {
              salientPoint.description += ", " + iter->asString();
              ++iter;
            }
            salientPoints.emplace_back(std::move(salientPoint));
            gotSalientPoint = true;
          }
        }
      }
      else
      {
        // Otherwise, create a SalientPoint for each caption, with the "text" field
        // as each one's description, and "confidence" as the score, if available
        for(const Json::Value& captionJson : captionsJson)
        {
          if(captionJson.isMember("text"))
          {
            salientPoint.score = 0.f;
            JsonTools::GetValueOptional(captionJson, "confidence", salientPoint.score);
            salientPoint.description = captionJson["text"].asString();
            salientPoints.emplace_back(salientPoint);
            gotSalientPoint = true;
          }
        }
      }
    }
  }
  
  if(!gotSalientPoint)
  {
    LOG_ERROR("OffboardModel.ParseSalientPointsFromJson.FailedToGetSceneDescriptionSalientPoint", "");
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::ParseObjectDetectionsFromJson(const Json::Value& jsonSalientPoints,
                                                    std::list<Vision::SalientPoint>& salientPoints)
{
  // Assume this Json structure:
  //
  //  {"objects": [
  //    {
  //      "name": "Person",
  //      "score": 0.9962483,
  //      "bounding_poly": [
  //        {"x":0.01023531, "y":0.0017647222},
  //        {"x":0.89, "y":0.0017647222},
  //        ...
  //      ]
  //    }
  //  ]}
  
  if(!jsonSalientPoints.isMember("objects"))
  {
    LOG_ERROR("OffboardModel.ParseObjectDetectionsFromJson.MissingObjects", "");
    return RESULT_FAIL;
  }
    
  const Json::Value& objectsJson = jsonSalientPoints["objects"];
  if(!objectsJson.isArray())
  {
    LOG_ERROR("OffboardModel.ParseObjectDetectionsFromJson.ExpectingObjectsArray", "");
    return RESULT_FAIL;
  }
  
  for(const auto& objectJson : objectsJson)
  {
    if(!objectJson.isMember("name") || !objectJson.isMember("bounding_poly"))
    {
      LOG_ERROR("OffboardModel.ParseObjectDetectionsFromJson.MissingNameOrBoundingPoly", "");
      continue;
    }
  
    const std::string& name = objectJson["name"].asString();
    
    Vision::SalientPoint salientPoint(_imageTimestamp,
                                      0.5f, 0.5f,
                                      0.f, 1.f,
                                      Vision::SalientPointType::Unknown,
                                      name,
                                      {}, 0);
    
    if(name == "Person" || name == "Man" || name == "Woman")
    {
      salientPoint.salientType = Vision::SalientPointType::Person;
    }
    // TODO: Handle other object types
    
    Point2f center(0.f, 0.f);
    const Json::Value& polyJson = objectJson["bounding_poly"];
    for(const auto& pointJson : polyJson)
    {
      if(ANKI_VERIFY(pointJson.isMember("x") && pointJson.isMember("y"),
                     "OffboardModel.ParseObjectDetectionsFromJson.ExpectingXandY", ""))
      {
        const Point2f point(pointJson["x"].asFloat(), pointJson["y"].asFloat());
        center += point;
        salientPoint.shape.emplace_back(point.ToCladPoint2d());
      }
    }
    
    if(!salientPoint.shape.empty())
    {
      // center contains a sum at this point: make it an average and store in SalientPoint
      center *= 1.f / (float)salientPoint.shape.size();
      salientPoint.x_img = center.x();
      salientPoint.y_img = center.y();
      
      // For now, just using the bounding rectangle area as the area fraction
      // TODO: Something more accurate (area of poly)
      Rectangle<f32> boundingBox(Poly2f(salientPoint.shape));
      salientPoint.area_fraction = boundingBox.Area();
    }
    
    // Not the end of the world if we don't get a score, so make it optional
    JsonTools::GetValueOptional(objectJson, "score", salientPoint.score);
    
    LOG_INFO("OffboardModel.ParseObjectDetectionsFromJson.FoundSalientPoint",
             "Type:%s Name:%s", EnumToString(salientPoint.salientType), salientPoint.description.c_str());
    
    salientPoints.emplace_back(std::move(salientPoint));
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result OffboardModel::ParseFaceDataFromJson(const Json::Value& resultArray,
                                            std::list<Vision::SalientPoint>& salientPoints)
{
  // Assumes this Json structure:
  //  [
  //    {
  //      "face_id":"1ddbd1d6-730f-47ce-bcac-a7a2954cbf45",
  //      "raw_face_result": {
  //        "faceId": "1ddbd1d6-730f-47ce-bcac-a7a2954cbf45",
  //        "faceRectangle": {
  //          "width":193,
  //          "height":193,
  //          "left":298,
  //          "top":102
  //        },
  //        "faceAttributes": {
  //          "age": 33,
  //          "gender": "male",
  //          "emotion": {
  //            "anger":0,
  //            "contempt":0,
  //            "disgust":0,
  //            "fear":0,
  //            "happiness":1,
  //            "neutral":0,
  //            "sadness":0,
  //            "surprise":0
  //          },
  //        }
  //      }
  //      "candidates": [
  //        {
  //          "person_name": "test-brad",
  //          "person_id":"9e2cd8b6-e433-4de6-a81b-081e5e6fcdd2",
  //          "confidence":0.62241
  //        }
  //      ]
  //    }
  //  ]
  
  if(!resultArray.isArray())
  {
    LOG_ERROR("OffboardModel.ParseFaceDataFromJson.ExpectingArray", "");
    return RESULT_FAIL;
  }
  
  for(const Json::Value& result : resultArray)
  {
    if(!result.isMember("raw_face_result"))
    {
      LOG_ERROR("OffboardModel.ParseFaceDataFromJson.MissingRawFaceResult", "");
      return RESULT_FAIL;
    }
    
    const Json::Value& rawFaceResultJson = result["raw_face_result"];
    if(!rawFaceResultJson.isMember("faceRectangle"))
    {
      LOG_ERROR("OffboardModel.ParseFaceDataFromJson.MissingFaceRectangle", "");
      return RESULT_FAIL;
    }
    
    const Json::Value& faceRectJson = rawFaceResultJson["faceRectangle"];
    if(!faceRectJson.isMember("width") || !faceRectJson.isMember("height") ||
       !faceRectJson.isMember("left")  || !faceRectJson.isMember("top"))
    {
      LOG_ERROR("OffboardModel.ParseFaceDataFromJson.BadFaceRectangle", "");
      return RESULT_FAIL;
    }
    
    if(!rawFaceResultJson.isMember("faceAttributes"))
    {
      LOG_ERROR("OffboardModel.ParseFaceDataFromJson.MissingFaceAttributes", "");
      return RESULT_FAIL;
    }
    
    // NOTE: Microsoft endpoint does not send back normalized coordinates!
    const f32 x = faceRectJson["left"].asFloat() / (f32)_imageCols;
    const f32 y = faceRectJson["top"].asFloat() / (f32)_imageRows;
    const f32 w = faceRectJson["width"].asFloat() / (f32)_imageCols;
    const f32 h = faceRectJson["height"].asFloat() / (f32)_imageRows;
    const Rectangle<f32> faceRect(x, y, w, h);
    
    const Json::Value& faceAttrJson = rawFaceResultJson["faceAttributes"];
    Json::Value descriptionJson;
    if(faceAttrJson.isMember("age"))
    {
      descriptionJson["age"] = faceAttrJson["age"];
    }
    if(faceAttrJson.isMember("emotion"))
    {
      descriptionJson["emotion"] = faceAttrJson["emotion"];
    }
    
    Vision::SalientPoint salientPoint(_imageTimestamp,
                                      faceRect.GetXmid(), faceRect.GetYmid(),
                                      0.f, faceRect.Area(),
                                      Vision::SalientPointType::Face,
                                      "",
                                      Poly2f(faceRect).ToCladPoint2dVector(), 0);
    
    if(result.isMember("candidates"))
    {
      const Json::Value& candidatesJson = result["candidates"];
      if(!candidatesJson.isNull())
      {
        if(!candidatesJson.isArray())
        {
          LOG_ERROR("OffboardModel.ParseFaceDataFromJson.CandidatesNotArray", "");
          continue;
        }
        
        for(const auto& candidateJson : candidatesJson)
        {
          if(!candidateJson.isMember("confidence"))
          {
            LOG_ERROR("OffboardModel.ParseFaceDataFromJson.MissingCandidateConfidence", "");
            continue;
          }
          
          if(!candidateJson.isMember("person_name"))
          {
            LOG_ERROR("OffboardModel.ParseFaceDataFromJson.MissingCandidateName", "");
            continue;
          }
          
          const f32 score = candidateJson["confidence"].asFloat();
          if(score > salientPoint.score)
          {
            salientPoint.score = score;
            descriptionJson["name"] = candidateJson["person_name"];
          }
        }
      }
    }
    
    // Take all the description data and stuff it into the description string as Json
    Json::FastWriter writer;
    salientPoint.description = writer.write(descriptionJson);
    
    salientPoints.emplace_back(std::move(salientPoint));
  }
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardModel::Connect()
{
  static s32 numTries = 0;
  
  const char* serverPath = LOCAL_SOCKET_PATH "box_server";
  const char* clientPath = LOCAL_SOCKET_PATH "_box_vision_client";
  
  const bool udpSuccess = _udpClient->Connect(clientPath, serverPath);
  ++numTries;
  
  LOG_INFO("OffboardModel.Connect.Status", "Try %d: %s - Server:%s Client:%s",
           numTries, (udpSuccess ? "Success" : "Fail"), serverPath, clientPath);
  
  return udpSuccess;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OffboardModel::IsConnected() const
{
  if(ANKI_VERIFY(nullptr != _udpClient, "OffboardModel.IsConnected.NoUdpClient", "UDP client should be instantiated"))
  {
    return _udpClient->IsConnected();
  }
  return false;
}
  
  
} // namespace Vision
} // namespace Anki

