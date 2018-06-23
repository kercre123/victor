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

#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

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


class BehaviorEventAnimResponseDirector : public IDependencyManagedComponent<BCComponentID>, 
                                          private Util::noncopyable 
{
public:
  BehaviorEventAnimResponseDirector();
  virtual ~BehaviorEventAnimResponseDirector() {};

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Robot* robot, const BCCompMap& dependentComps) override {};
  virtual void UpdateDependent(const BCCompMap& dependentComps) override {};
  virtual void GetInitDependencies(BCCompIDSet& dependencies) const override {};
  virtual void GetUpdateDependencies(BCCompIDSet& dependencies) const override {};
  //////
  // end IDependencyManagedComponent functions
  //////


  AnimationTrigger GetAnimationToPlayForActionResult(const UserFacingActionResult result) const;
  
private:
  std::map<UserFacingActionResult, AnimationTrigger> _animationTriggerMap;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorEventAnimResponseDirector_H__
