/**
 * File: behaviorPreReqAcknowledgeObject.h
 *
 * Author: Kevin M. Karol
 * Created: 12/09/16
 *
 * Description: Defines pre requisites that can be passed into
 * AcknowledgeObject behavior's isRunnable()
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeObject_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeObject_H__

#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

class BehaviorPreReqAcknowledgeObject{
public:
  BehaviorPreReqAcknowledgeObject(const s32 target){
    _targets.clear();
    _targets.emplace(target);
  }
  
  BehaviorPreReqAcknowledgeObject(const std::set<s32>& targets){
    _targets = targets;
  }
  
  std::set<s32> GetTargets() const { return _targets;}
  
private:
  std::set<s32> _targets;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqAcknowledgeObject_H__


