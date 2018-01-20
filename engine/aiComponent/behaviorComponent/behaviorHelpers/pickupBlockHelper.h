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
#include "coretech/common/engine/objectIDs.h"
#include "clad/types/animationTrigger.h"

namespace Anki {
namespace Cozmo {


class PickupBlockHelper : public IHelper{
public:
  PickupBlockHelper(ICozmoBehavior& behavior,
                    BehaviorHelperFactory& helperFactory,
                    const ObjectID& targetID,
                    const PickupBlockParamaters& parameters = {});
  virtual ~PickupBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates() const override;
  virtual HelperStatus InitBehaviorHelper() override;
  virtual HelperStatus UpdateWhileActiveInternal() override;
  
private:
  ObjectID _targetID;
  PickupBlockParamaters _params;
  
  u32 _dockAttemptCount;
  bool _hasTriedOtherPose;
  
  void StartPickupAction(bool ignoreCurrentPredockPose = false);
  void RespondToPickupResult(const ExternalInterface::RobotCompletedAction& rca);

  void MarkTargetAsFailedToPickup();
  
};


  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PickupBlockHelper_H__

