/**
 * File: pickupBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles picking up a block with a given ID
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PickupBlockHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PickupBlockHelper_H__

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "anki/common/basestation/objectIDs.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {


class PickupBlockHelper : public IHelper{
public:
  PickupBlockHelper(BehaviorExternalInterface& behaviorExternalInterface, ICozmoBehavior& behavior,
                    BehaviorHelperFactory& helperFactory,
                    const ObjectID& targetID,
                    const PickupBlockParamaters& parameters = {});
  virtual ~PickupBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual BehaviorStatus Init(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  ObjectID _targetID;
  PickupBlockParamaters _params;
  
  u32 _dockAttemptCount;
  bool _hasTriedOtherPose;
  
  void StartPickupAction(BehaviorExternalInterface& behaviorExternalInterface, bool ignoreCurrentPredockPose = false);
  void RespondToPickupResult(const ExternalInterface::RobotCompletedAction& rca, BehaviorExternalInterface& behaviorExternalInterface);

  void MarkTargetAsFailedToPickup(BehaviorExternalInterface& behaviorExternalInterface);
  
};


  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PickupBlockHelper_H__

