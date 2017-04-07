/**
 * File: rollBlockHelper.h
 *
 * Author: Kevin M. Karol
 * Created: 2/1/17
 *
 * Description: Handles rolling a block
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_RollBlockHelper_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorHelpers_RollBlockHelper_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/iHelper.h"
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {

  
class RollBlockHelper : public IHelper{
protected:
  using PreDockCallback = std::function<void(Robot&)>;

public:
  RollBlockHelper(Robot& robot,
                  IBehavior& behavior,
                  BehaviorHelperFactory& helperFactory,
                  const ObjectID& targetID,
                  bool rollToUpright = true,
                  const RollBlockParameters& parameters = {});
  virtual ~RollBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot) override;
private:
  ObjectID _targetID;
  RollBlockParameters _params;

  
  void DetermineAppropriateAction(Robot& robot);
  void UnableToRollDelegate(Robot& robot);
  void DelegateToPutDown(Robot& robot);
  void StartRollingAction(Robot& robot);
  void RespondToRollingResult(ActionResult result, Robot& robot);
  
  void MarkTargetAsFailedToRoll(Robot& robot);

  bool _shouldRoll = true;
  const bool _shouldUpright;
  u32 _tmpRetryCounter;


};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

