/**
 * File: behaviorRollBlock.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-05
 *
 * Description: This behavior rolls blocks when it see's them not upright
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorRollBlock_H__
#define __Cozmo_Basestation_Behaviors_BehaviorRollBlock_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include <string>

namespace Anki {
namespace Cozmo {

class BlockWorldFilter;
class ObservableObject;
class Robot;

class BehaviorRollBlock : public IBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorRollBlock(Robot& robot, const Json::Value& config);

  virtual Result InitInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;

  virtual bool IsRunnableInternal(const Robot& robot) const override;
    
private:

  std::string _initialAnimGroup = "rollCube_initial";
  std::string _realignAnimGroup = "rollCube_realign";
  std::string _retryActionAnimGroup = "rollCube_retry";
  std::string _successAnimGroup = "rollCube_success";

  // TODO:(bn) a few behaviors have used this pattern now, maybe we should re-think having some kind of
  // UpdateWhileNotRunning
  mutable ObjectID _targetBlock;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilter;

  const Robot& _robot;

  enum class State {
    SettingDownBlock,
    ReactingToBlock,
    PerformingAction
  };

  State _state = State::ReactingToBlock;

  void TransitionToSettingDownBlock(Robot& robot);
  void TransitionToReactingToBlock(Robot& robot);
  void TransitionToPerformingAction(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);
  void ResetBehavior(Robot& robot);

  // This should return true if the block is valid for this action, false otherwise. Checks that the block is
  // a light cube with known position, not moving, resting flat, and not being carried
  virtual bool FilterBlocks(ObservableObject* obj) const;

  virtual void UpdateTargetBlock(const Robot& robot) const;
};

}
}

#endif
