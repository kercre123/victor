/**
* File: continuityComponent.h
*
* Author: Kevin M. Karol
* Created: 2/1/18
*
* Description: Component responsible for ensuring decisions by the behavior system 
* blend together well by the time they are sent to robot
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_AI_ContinuityComponent_H__
#define __Cozmo_Basestation_AI_ContinuityComponent_H__

#include "clad/types/animationTrigger.h"
#include "engine/actions/actionInterface.h"
#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

namespace Anki {
namespace Cozmo {

class Robot;

class ContinuityComponent : public IDependencyManagedComponent<AIComponentID>,  
                            private Util::noncopyable
{
public:
  ContinuityComponent(Robot& robot);
  
  // IDependencyManagedComponent<AIComponentID> functions
  virtual void UpdateDependent(const AICompMap& dependentComps) override;
  // end IDependencyManagedComponent<AIComponentID> functions

  
  // Inform the continuity component of the next desired action
  bool GetIntoAction(IActionRunner* action);

  // Ask the continuity component to stop performing an action
  bool GetOutOfAction(u32 idTag);

  // Allow behaviors to set emergency get outs that will be played before
  // the next action is queued
  void PlayEmergencyGetOut(AnimationTrigger anim);

private:
  Robot& _robot;
  bool _playingGetOut = false;
  IActionRunner* _nextActionToQueue = nullptr;

  bool QueueAction(IActionRunner* action);


};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_AI_ContinuityComponent_H__
