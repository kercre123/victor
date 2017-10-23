/**
* File: behaviorTurnToFace.h
*
* Author: Kevin M. Karol
* Created: 6/6/17
*
* Description: Simple behavior to turn toward a face - face can either be passed
* in as part of WantsToBeActivated, or selected using internal criteria using WantsToBeActivated
* with a robot
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorTurnToFace : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorTurnToFace(const Json::Value& config);
  
public:
  virtual ~BehaviorTurnToFace() override {}
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override{ return false;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  mutable SmartFaceID _targetFace;
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorTurnToFace_H__
