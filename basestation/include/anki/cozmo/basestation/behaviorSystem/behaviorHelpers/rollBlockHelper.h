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
public:
  RollBlockHelper(Robot& robot,
                  IBehavior* behavior,
                  BehaviorHelperFactory& helperFactory,
                  const ObjectID& targetID,
                  bool rollToUpright);
  virtual ~RollBlockHelper();

protected:
  // IHelper functions
  virtual bool ShouldCancelDelegates(const Robot& robot) const override;
  virtual BehaviorStatus Init(Robot& robot,
                              DelegateProperties& delegateProperties) override;
  virtual BehaviorStatus UpdateWhileActiveInternal(Robot& robot,
                                                   DelegateProperties& delegateProperties) override;
private:
  ObjectID _targetID;
  
  void StartRollingAction(Robot& robot);
  void RespondToRollingResult(ActionResult result, Robot& robot);

  bool _shouldRoll = true;
  const bool _shouldUpright;

};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

