/**
 * File: BehaviorPreReqRespondPossiblyRoll.h
 *
 * Author: Kevin M. Karol
 * Created: 2/17/16
 *
 * Description: Defines prerequisites for responding to a block in the very
 * specific way we need to for building a pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqs_BehaviorPreReqRespondPossiblyRoll_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqs_BehaviorPreReqRespondPossiblyRoll_H__

namespace Anki {
namespace Cozmo {

class BehaviorPreReqRespondPossiblyRoll {
public:
  BehaviorPreReqRespondPossiblyRoll(u32 objectID, u32 uprightAnimIndex, u32 onSideAnimIndex)
  : _objectID(objectID)
  , _uprightAnimIndex(uprightAnimIndex)
  , _onSideAnimIndex(onSideAnimIndex) {};
  
  u32 GetObjectID() const         { return _objectID;}
  u32 GetUprightAnimIndex() const { return _uprightAnimIndex;}
  u32 GetOnSideAnimIndex() const  { return _onSideAnimIndex;}
  
private:
  u32 _objectID;
  u32 _uprightAnimIndex;
  u32 _onSideAnimIndex;
  
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqs_BehaviorPreReqRespondPossiblyRoll_H__


