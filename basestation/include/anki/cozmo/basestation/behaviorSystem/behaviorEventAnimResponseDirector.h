/**
 * File: behaviorEventAnimResponseDirector.h
 *
 * Author: Kevin M. Karol
 * Created: 2/24/17
 *
 * Description: Tracks the appropriate response that should be played if Cozmo
 * encounters a behavior event.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorEventAnimResponseDirector_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorEventAnimResponseDirector_H__

#include "clad/types/animationTrigger.h"
#include "clad/types/userFacingResults.h"

#include <map>

namespace Anki {
namespace Cozmo {

struct ActionResponseData{
public:
  ActionResponseData(UserFacingActionResult result, AnimationTrigger trigger)
  : _result(result)
  , _trigger(trigger){}

  UserFacingActionResult GetActionResult() const { return _result;}
  AnimationTrigger GetAnimationTrigger() const { return _trigger;}
private:
  UserFacingActionResult _result;
  AnimationTrigger _trigger;
};


class BehaviorEventAnimResponseDirector {
public:
  BehaviorEventAnimResponseDirector();
  virtual ~BehaviorEventAnimResponseDirector() {};
  AnimationTrigger GetAnimationToPlayForActionResult(const UserFacingActionResult result) const;
  
private:
  std::map<UserFacingActionResult, AnimationTrigger> _animationTriggerMap;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorEventAnimResponseDirector_H__
