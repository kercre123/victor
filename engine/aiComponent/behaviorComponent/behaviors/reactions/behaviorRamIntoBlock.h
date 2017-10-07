/**
 * File: behaviorRamIntoBlock.h
 *
 * Author: Kevin M. Karol
 * Created: 2/21/17
 *
 * Description: Behavior to ram into a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "anki/common/basestation/objectIDs.h"
#include <vector>

namespace Anki {
namespace Cozmo {

namespace ExternalInterface {
struct RobotObservedObject;
}

class BehaviorRamIntoBlock : public ICozmoBehavior
{
  
private:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorRamIntoBlock(const Json::Value& config);


public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
  void SetBlockToRam(s32 targetID){_targetID = targetID;}

protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Result ResumeInternal(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  s32 _targetID;
  
  void TransitionToPuttingDownBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToTurningToBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToRammingIntoBlock(BehaviorExternalInterface& behaviorExternalInterface);

  
}; // class BehaviorRamIntoBlock

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorRamIntoBlock_H__
