/**
 * File: anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgePet.h
 *
 * Author: Kevin M. Karol
 * Created: 12/09/16
 *
 * Description: Defines prerequisites that can be passed into 
 * AcknowledgePet's behavior's isRunnable()
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgePet_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgePet_H__


namespace Anki {
namespace Cozmo {

class BehaviorPreReqAcknowledgePet {
public:
  BehaviorPreReqAcknowledgePet(const std::set<Vision::FaceID_t>& faceIDs) {
    _targets = faceIDs;
  }
  
  // Return set of targets that satisfy trigger condition
  const std::set<Vision::FaceID_t> & GetTargets() const { return _targets; }
  
private:
    std::set<Vision::FaceID_t> _targets;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeFace_H__

