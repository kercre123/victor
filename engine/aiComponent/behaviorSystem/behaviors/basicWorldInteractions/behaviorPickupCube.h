/**
 * File: behaviorPickupCube.h
 *
 * Author: Molly Jameson / Kevin M. Karol
 * Created: 2016-07-26   / 2017-04-07
 *
 * Description:  Behavior which picks up a cube
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPickUpCube_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPickUpCube_H__

#include "anki/common/basestation/objectIDs.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
namespace BlockConfigurations{
enum class ConfigurationType;
}

class CompoundActionSequential;
class ObservableObject;

  
class BehaviorPickUpCube : public IBehavior
{
using super = IBehavior;
protected:  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPickUpCube(const Json::Value& config);

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  
  virtual void UpdateTargetBlocksInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  virtual std::set<ObjectInteractionIntention>
        GetBehaviorObjectInteractionIntentions() const override {
           return {ObjectInteractionIntention::PickUpObjectNoAxisCheck};
        }
  
private:
  enum class DebugState
  {
    DoingInitialReaction,
    PickingUpCube,
    DoingFinalReaction
  };
  
  mutable ObjectID    _targetBlockID;
  std::vector<BlockConfigurations::ConfigurationType> _configurationsToIgnore;
  
  void TransitionToDoingInitialReaction(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToPickingUpCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToSuccessReaction(BehaviorExternalInterface& behaviorExternalInterface);


}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif
