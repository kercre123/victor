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

#include "coretech/common/engine/objectIDs.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
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

  
class BehaviorPickUpCube : public ICozmoBehavior
{
public:
  void SetTargetID(const ObjectID& targetID){
    _dVars.targetBlockID = targetID;
    _dVars.idSetExternally = true;
  }

protected:  
  using super = ICozmoBehavior;

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorPickUpCube(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {}

  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  
  void CalculateTargetID(ObjectID& outTargetID) const;
  
private:
  enum class DebugState
  {
    DoingInitialReaction,
    PickingUpCube,
    DoingFinalReaction
  };

  struct InstanceConfig{
    int pickupRetryCount = 1;
  };

  struct DynamicVariables{
    ObjectID targetBlockID;
    bool idSetExternally = false;
    int pickupRetryCount = 0;
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToDoingInitialReaction();
  void TransitionToPickingUpCube();
  void TransitionToSuccessReaction();


}; // class BehaviorPickUpCube

} // namespace Cozmo
} // namespace Anki

#endif
