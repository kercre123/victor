/**
 * File: behaviorPreReqAcknowledgePet.h
 *
 * Author: Kevin M. Karol
 * Created: 12/09/16
 *
 * Description: Defines pre requisites that can be passed into 
 * AcknowledgePet's behavior's isRunnable()
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgePet_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgePet_H__


namespace Anki {
namespace Cozmo {

class BehaviorPreReqAcknowledgePet{
public:
  BehaviorPreReqAcknowledgePet(const std::set<Vision::FaceID_t>& faceIDs){
    _targets = faceIDs;
  }
  
private:
    std::set<Vision::FaceID_t> _targets;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeFace_H__

