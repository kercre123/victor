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
#define  __Cozmo_Basestation_Behaviors_BehaviorRollBlock_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include <set>
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
  
public:

  virtual bool IsRunnable(const Robot& robot) const override;

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual Result InterruptInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  virtual float  EvaluateRunningScoreInternal(const Robot& robot) const override;

private:

  std::string _initialAnimGroup = "rollCube_initial";
  std::string _realignAnimGroup = "rollCube_realign";
  std::string _retryRollAnimGroup = "rollCube_retry";
  std::string _successAnimGroup = "rollCube_success";
  
  ObjectID _targetBlock;

  std::unique_ptr<BlockWorldFilter>  _blockworldFilter;

  const Robot& _robot;

  enum class State {
    SelectingTargetBlock,
    RollingBlock
  };

  State _state = State::SelectingTargetBlock;

  void TransitionToSelectingTargetBlock(Robot& robot);
  void TransitionToRollingBlock(Robot& robot);

  void SetState_internal(State state, const std::string& stateName);
  void ResetBehavior(Robot& robot);

  bool FilterBlocks(ObservableObject* obj) const;
  bool IsBlockInTargetRestingPosition(const Robot& robot, ObjectID objectID) const;
  bool IsBlockInTargetRestingPosition(const ObservableObject* obj) const;
  bool HasValidTargetBlock(const Robot& robot) const;
};

}
}

#endif
