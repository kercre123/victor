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

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/common/basestation/objectIDs.h"


namespace Anki {
namespace Cozmo {


class PickupBlockHelper : public IHelper{
public:
  PickupBlockHelper(Robot& robot, IBehavior* behavior, BehaviorHelperFactory& helperFactory, const ObjectID& targetID);
  virtual ~PickupBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot,
                              DelegateProperties& delegateProperties) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot,
                                                   DelegateProperties& delegateProperties) override;
  
private:
  ObjectID _targetID;
  
  void StartPickupAction(Robot& robot);
  void RespondToPickupResult(ActionResult result, Robot& robot);
  void RespondToSearchResult(ActionResult result, Robot& robot);

};


  
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_PickupBlockHelper_H__

