/**
 * File: behaviorPreReqAcknowledgeFace.h
 *
 * Author: Kevin M. Karol
 * Created: 12/09/16
 *
 * Description: Defines prerequisites that can be passed into 
 * AcknowledgeFace behavior's isRunnable()
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeFace_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeFace_H__

#include "anki/vision/basestation/faceIdTypes.h"

#include <set>

namespace Anki {
namespace Cozmo {

class BehaviorPreReqAcknowledgeFace{
public:
  BehaviorPreReqAcknowledgeFace(const std::set<Vision::FaceID_t>& desiredTargets)
  {
    _desiredTargets = desiredTargets;
  }
  
  std::set<Vision::FaceID_t> GetDesiredTargets()const{return _desiredTargets;}
  
private:
  std::set< Vision::FaceID_t > _desiredTargets;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeFace_H__

