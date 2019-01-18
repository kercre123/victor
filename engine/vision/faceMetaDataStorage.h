/**
 * File: visionSystem.cpp [Basestation]
 *
 * Author: Andrew Stein
 * Date:   1/8/2019
 *
 * Description: Container for storing additional meta data about faces that comes from another source (such
 *              as a neural net (including offboard).
 *
 * Copyright: Anki, Inc. 2019
 **/


#ifndef __Anki_Vector_Engine_Vision_FaceMetaDataStorage_H__
#define __Anki_Vector_Engine_Vision_FaceMetaDataStorage_H__

#include "clad/types/salientPointTypes.h"
#include "clad/types/facialExpressions.h"
#include "coretech/common/engine/math/rect.h"
#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include <list>
#include <map>
#include <string>

namespace Anki {
  
namespace Vision {
  class TrackedFace;
}

namespace Vector {

class FaceMetaDataStorage
{
public:
  
  // Should be called whenever bookkeeping updates an old ID to a new one
  void OnUpdatedID(const Vision::FaceID_t oldID, const Vision::FaceID_t newID);
  
  // Returns true and updates available fields in forFace if the face's ID is known to the container
  // Returns false if the face is not known
  bool GetMetaData(Vision::TrackedFace& forFace) const;
  
  // Mark a face as one whose metadata is being computed. Can be called multiple times.
  // Expected to be paired with a subsequent call to Update()
  void SetFaceBeingProcessed(const Vision::TrackedFace& face);
  
  // Look for any Face type salient points and update stored meta data for faces marked
  // with SetFaceBeingProcessed(), using position in the image to establish correspondence
  void Update(const std::list<Vision::SalientPoint>& salientPoints, const s32 numImgRows, const s32 numImgCols);
  
private:
  
  // TODO: Consider making this a CLAD structure to get Json parsing for free
  struct MetaData
  {
    std::string name;
    u32 age = 0;
    
    std::map<Vision::FacialExpression, u8> expressionValues;
    
    MetaData() : expressionValues{} { }
    MetaData(const Json::Value& json);
  };
  
  std::map<Vision::FaceID_t, Rectangle<f32>> _currentFacesBeingRecognized;
  
  std::map<Vision::FaceID_t, MetaData> _data;
};

} // namespace Vector
} // namespace Anki

#endif /* __Anki_Vector_Engine_Vision_FaceMetaDataStorage_H__ */
