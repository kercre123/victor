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

#include "engine/aiComponent/behaviorComponent/behaviorHelpers/iHelper.h"
#include "coretech/common/engine/objectIDs.h"

namespace Anki {
namespace Cozmo {

  
class RollBlockHelper : public IHelper{
protected:
  using PreDockCallback = std::function<void(Robot&)>;

public:
  RollBlockHelper(ICozmoBehavior& behavior,
                  BehaviorHelperFactory& helperFactory,
                  const ObjectID& targetID,
                  bool rollToUpright = true,
                  const RollBlockParameters& parameters = {});
  virtual ~RollBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates() const override;
  virtual HelperStatus InitBehaviorHelper() override;
  virtual HelperStatus UpdateWhileActiveInternal() override;
private:
  ObjectID _targetID;
  RollBlockParameters _params;

  
  void DetermineAppropriateAction();
  void UnableToRollDelegate();
  void DelegateToPutDown();
  void StartRollingAction();
  void RespondToRollingResult(ActionResult result);
  
  void MarkTargetAsFailedToRoll();

  bool _shouldRoll = true;
  const bool _shouldUpright;
  u32 _tmpRetryCounter;


};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

