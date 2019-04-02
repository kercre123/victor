/**
 * File: parseSalientPointsFromJson.cpp
 *
 * Author: Andrew Stein
 * Date:   3/7/2019
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2019
 **/

#include "clad/types/salientPointTypes.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/math/rect.h"
#include "coretech/common/engine/math/polygon.h"
#include "coretech/vision/engine/offboardProcessor.h"
#include "coretech/vision/engine/parseSalientPointsFromJson.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "NeuralNets"

namespace Anki {
namespace Vision {

namespace {
  
CONSOLE_VAR_RANGED(s32, kNeuralNets_MaxNumSceneDescriptionTags, "NeuralNets", 5, 3, 10);
  
}
 
namespace JsonKeys {
  const char* const SalientPoints = "salientPoints";
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ParseSceneDescriptionFromJson(const Json::Value& jsonSalientPoints,
                                     const int imageRows, const int imageCols, const TimeStamp_t timestamp,
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
      
      Vision::SalientPoint salientPoint(timestamp,
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
            salientPoint.description = "A scene with: " + tagsJson[0].asString();
            s32 tagCount = 1;
            const s32 maxCount = std::min((s32)tagsJson.size(), kNeuralNets_MaxNumSceneDescriptionTags);
            while(tagCount < maxCount-1)
            {
              salientPoint.description += ", " + tagsJson[tagCount].asString();
              ++tagCount;
            }
            if(tagCount < maxCount)
            {
              // Put "and" in front of last tag, if there are any left
              salientPoint.description += ", and " + tagsJson[tagCount].asString();
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
Result ParseObjectDetectionsFromJson(const Json::Value& jsonSalientPoints,
                                     const int imageRows, const int imageCols, const TimeStamp_t timestamp,
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
    
    Vision::SalientPoint salientPoint(timestamp,
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
        salientPoint.shape.emplace_back(CladPoint2d{point.x(), point.y()});
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
      const Poly2f poly(salientPoint.shape);
      salientPoint.area_fraction = (poly.GetMaxX()-poly.GetMinX()) * (poly.GetMaxY()-poly.GetMinY());
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
Result ParseFaceDataFromJson(const Json::Value& resultArray,
                             const int imageRows, const int imageCols, const TimeStamp_t timestamp,
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
    const f32 x = faceRectJson["left"].asFloat() / (f32)imageCols;
    const f32 y = faceRectJson["top"].asFloat() / (f32)imageRows;
    const f32 w = faceRectJson["width"].asFloat() / (f32)imageCols;
    const f32 h = faceRectJson["height"].asFloat() / (f32)imageRows;
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
    
    Vision::SalientPoint salientPoint(timestamp,
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
Result ParseTextDetectionsFromJson(const Json::Value& detectionResult,
                                   const int imageRows, const int imageCols, const TimeStamp_t timestamp,
                                   std::list<Vision::SalientPoint>& salientPoints)
{
  
  if(detectionResult.isMember("recognitionResult"))
  {
    //  "recognitionResult": {
    //    "lines": [
    //              {
    //              "boundingBox": [
    //                              122,
    //                              122,
    //                              401,
    //                              85,
    //                              404,
    //                              229,
    //                              143,
    //                              233
    //                              ],
    //              "text": "Sorry!",
    
    const Json::Value& recResult = detectionResult["recognitionResult"];
    
    DEV_ASSERT(recResult.isMember("lines"), "OffboardModel.ParseTextDetectionsFromJson.MissingLines");
    const Json::Value& linesJson = recResult["lines"];
    DEV_ASSERT(linesJson.isArray(), "OffboardModel.ParseTextDetectionsFromJson.LinesNotArray");
    
    // One SalientPoint per line of text
    for(const auto& lineJson : linesJson)
    {
      Vision::SalientPoint salientPoint(timestamp,
                                        0.f, 0.f,
                                        0.f, 1.f,
                                        Vision::SalientPointType::Text,
                                        "", {}, 0);
      
      DEV_ASSERT(lineJson.isMember("boundingBox"), "OffboardModel.ParseTextDetectionsFromJson.MissingBoundingBox");
      const Json::Value& bboxJson = lineJson["boundingBox"];
      DEV_ASSERT(bboxJson.isArray(), "OffboardModel.ParseTextDetectionsFromJson.BoundingBoxNotArray");
      DEV_ASSERT(bboxJson.size() % 2 == 0, "OffboardModel.ParseTextDetectionsFromJson.BoundingBoxNotEvenSize");
      for(s32 i=0; i<bboxJson.size(); i+=2)
      {
        CladPoint2d pt(bboxJson[i].asFloat() / (f32)imageCols, bboxJson[i+1].asFloat() / (f32)imageRows);
        salientPoint.x_img += (f32)pt.x;
        salientPoint.y_img += (f32)pt.y;
        salientPoint.shape.emplace_back(std::move(pt));
      }
      salientPoint.x_img /= (f32)bboxJson.size()/2;
      salientPoint.y_img /= (f32)bboxJson.size()/2;
      
      DEV_ASSERT(lineJson.isMember("text"), "OffboardModel.ParseTextDetectionsFromJson.MissingText");
      salientPoint.description = lineJson["text"].asString();
      
      salientPoints.emplace_back(std::move(salientPoint));
    }
  }
  else if(detectionResult.isMember("text"))
  {
    // {"text": "Hello World!"}
    
    Vision::SalientPoint salientPoint(timestamp,
                                      0.f, 0.f,
                                      0.f, 1.f,
                                      Vision::SalientPointType::Text,
                                      detectionResult["text"].asString(), {}, 0);
    
    LOG_INFO("OffboardModel.ParseTextDetectionsFromJson.CreatedSalientPoint",
             "Text: %s", salientPoint.description.c_str());
    
    salientPoints.emplace_back(salientPoint);
  }
  else
  {
    LOG_ERROR("OffboardModel.ParseTextDetectionsFromJson.UnrecognizedFormat",
              "Expecting 'text' or 'recognitionResult' fields.");
    return RESULT_FAIL;
  }
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ParseSalientPointsFromJson(const Json::Value& detectionResult,
                                  const int  imageRows, const int imageCols, const TimeStamp_t timestamp,
                                  std::list<Vision::SalientPoint>& salientPoints)
{
  using ParseFcn = std::function<Result(const Json::Value&,
                                        const int imageRows, const int imageCols, const TimeStamp_t timestamp,
                                        std::list<Vision::SalientPoint>& salientPoints)>;

  const std::map<std::string, ParseFcn> kParserLUT{
    {"SceneDescription",   &ParseSceneDescriptionFromJson},
    {"ObjectDetection",    &ParseObjectDetectionsFromJson},
    {"FaceRecognition",    &ParseFaceDataFromJson},
    {"OCR",                &ParseTextDetectionsFromJson},
  };

  auto iter = kParserLUT.end();
  std::string procType;
  if(!JsonTools::GetValueOptional(detectionResult, OffboardProcessor::JsonKeys::ProcType, procType))
  {
    LOG_WARNING("NeuralNets.ParseSalientPointsFromJson.MissingProcessingType",
                "No %s field in Json result. Assuming raw SalientPoints.",
                OffboardProcessor::JsonKeys::ProcType);
  }
  else
  {
    iter = kParserLUT.find(procType);
  }
  
  if(iter == kParserLUT.end())
  {
    // No registered parser: Assume the detectionResult just contains an array of SalientPoints in Json format
    if(!detectionResult.isMember(JsonKeys::SalientPoints))
    {
      LOG_ERROR("OffboardModel.ParseSalientPointsFromJson.MissingSalientPointsArray",
                "%s", JsonKeys::SalientPoints);
      return RESULT_FAIL;
    }
    
    const Json::Value& salientPointsJson = detectionResult[JsonKeys::SalientPoints];
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
  }
  else
  {
    return iter->second(detectionResult, imageRows, imageCols, timestamp, salientPoints);
  }
  
  return RESULT_OK;
}

}
}
