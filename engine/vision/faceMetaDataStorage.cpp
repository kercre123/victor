#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/engine/trackedFace.h"
#include "engine/vision/faceMetaDataStorage.h"
#include "util/console/consoleInterface.h"
#include "util/string/stringUtils.h"
#include "util/helpers/templateHelpers.h"

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
  const char* kNameKey    = "name";
  const char* kAgeKey     = "age";
  const char* kEmotionKey = "emotion";
  
  JsonTools::GetValueOptional(json, kNameKey, name);
  JsonTools::GetValueOptional(json, kAgeKey, age);
  
  if(json.isMember(kEmotionKey))
  {
    const Json::Value& emotionJson = json[kEmotionKey];
    using Expr_t = std::underlying_type<Vision::FacialExpression>::type;
    const Expr_t N = Util::EnumToUnderlying(Vision::FacialExpression::Count);
    for(Expr_t i=0; i<N; ++i)
    {
      const Vision::FacialExpression expression = (Vision::FacialExpression)i;
      f32 emotionScore = 0.f;
      std::string key(EnumToString(expression));
      key[0] = std::tolower(key[0]); // Our enum values have a capitalized first letter. Json keys do not.
      if(JsonTools::GetValueOptional(emotionJson, key, emotionScore))
      {
        expressionValues[expression] = std::round(255.f * emotionScore); // Ours are u8, Json's are [0,1] float
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceMetaDataStorage::OnUpdatedID(const Vision::FaceID_t oldID, const Vision::FaceID_t newID)
{
  auto iter = _data.find(oldID);
  if(iter != _data.end())
  {
    if(DEBUG_FACE_METADATA)
    {
      LOG_INFO("FaceMetaDataStorage.OnUpdatedID.UpdatingID", "OldID:%d NewID:%d (%s)",
               oldID, newID, Util::HidePersonallyIdentifiableInfo(iter->second.name.c_str()));
    }
    
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
  
  for(const auto& expressionValue : metaData.expressionValues)
  {
    forFace.SetExpressionValue(expressionValue.first, expressionValue.second);
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
      Json::Value jsonMetaData;
      const bool success = reader.parse(matchingSalientPoint->description, jsonMetaData);
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
