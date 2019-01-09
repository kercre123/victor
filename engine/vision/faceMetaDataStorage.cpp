#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/trackedFace.h"
#include "engine/vision/faceMetaDataStorage.h"
#include "util/console/consoleInterface.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Vector {

#define LOG_CHANNEL "NeuralNets" // Consider "FaceRecognizer"?
#define DEBUG_FACE_METADATA 0
  
namespace {

// Min overlap score for establishing correspondence with onboard/offboard face detections
CONSOLE_VAR(f32, kTheBox_MinFaceOverlapScore, "TheBox.Faces", 0.25);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceMetaDataStorage::MetaData::MetaData(const Json::Value& json)
{
  const char* kNameKey = "name";
  const char* kAgeKey  = "age";
  
  JsonTools::GetValueOptional(json, kNameKey, name);
  JsonTools::GetValueOptional(json, kAgeKey, age);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceMetaDataStorage::OnUpdatedID(const Vision::FaceID_t oldID, const Vision::FaceID_t newID)
{
  auto iter = _data.find(oldID);
  if(iter != _data.end())
  {
    _data[newID] = iter->second;
    _data.erase(oldID);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceMetaDataStorage::GetMetaData(Vision::TrackedFace& forFace) const
{
  auto iter = _data.find(forFace.GetID());
  if(iter == _data.end())
  {
    return false;
  }
  
  const MetaData& metaData = iter->second;
  
  // TODO: Update other parts of face
  if(!metaData.name.empty())
  {
    forFace.SetName(metaData.name);
  }
  if(metaData.age > 0)
  {
    forFace.SetAge(metaData.age);
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceMetaDataStorage::SetFaceBeingProcessed(const Vision::TrackedFace& face)
{
  _currentFacesBeingRecognized[face.GetID()] = face.GetRect();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceMetaDataStorage::Update(const std::list<Vision::SalientPoint>& salientPoints,
                                 const s32 numImgRows, const s32 numImgCols)
{
  // TODO: Establish correspondence with SalientPointType::Faces based on rectangles
  // and add new meta data from each
  
  Json::Reader reader;
  
  for(const auto& faceBeingRecognized : _currentFacesBeingRecognized)
  {
    const Vision::FaceID_t& faceID   = faceBeingRecognized.first;
    const Rectangle<f32>&   faceRect = faceBeingRecognized.second;
    
    // Find a matching salientPoint (one with most overlap above a minimum threshold)
    const Vision::SalientPoint* matchingSalientPoint = nullptr;
    f32 maxIOU = kTheBox_MinFaceOverlapScore;
    for(const auto& salientPoint : salientPoints)
    {
      if(salientPoint.salientType == Vision::SalientPointType::Face)
      {
        Poly2f salientPointPoly(salientPoint.shape);
        for(auto & pt : salientPointPoly)
        {
          pt.x() *= numImgCols;
          pt.y() *= numImgRows;
        }
        const f32 IOU = faceRect.ComputeOverlapScore(Rectangle<f32>(salientPointPoly));
        
        if(DEBUG_FACE_METADATA)
        {
          // Deliberately Info so it shows up in Release buids
          LOG_INFO("FaceMetaDataStorage.Update.CorrespondenceCheck",
                   "IOU:%.3f Description:%s", IOU, salientPoint.description.c_str());
        }
        
        if(IOU > maxIOU)
        {
          maxIOU = IOU;
          matchingSalientPoint = &salientPoint;
        }
      }
    }
    
    if(matchingSalientPoint != nullptr)
    {
      // Assume the description field is a Json string containing the meta data
      // Need to unescape the quotes though:
      std::string metaDataString(matchingSalientPoint->description);
      Util::StringReplace(metaDataString, "\\\"", "\""); // Replace '\"' with '"'
      Util::StringReplace(metaDataString, "\\", "\\\\"); // Replace '\' with '\\'
      
      Json::Value jsonMetaData;
      const bool success = reader.parse(metaDataString, jsonMetaData);
      if(!success)
      {
        LOG_ERROR("FaceMetaDataStorage.Update.FailedToParseSalientPointDescription", "");
        continue;
      }
      
      MetaData metaData(jsonMetaData);
      
      if(DEBUG_FACE_METADATA)
      {
        // Deliberately Info so it shows up in Release builds
        LOG_INFO("FaceMetaDataStorage.Update.AddMetaData",
                 "ID:%d Name:%s Age:%u", faceID, metaData.name.c_str(), metaData.age);
      }
      _data.emplace(faceID, std::move(metaData));
    }
  }
  
  _currentFacesBeingRecognized.clear();
}

} // namespace Vector
} // namespace Anki
