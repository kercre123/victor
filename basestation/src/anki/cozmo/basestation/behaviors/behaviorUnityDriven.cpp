/**
 * File: behaviorUnityDriven
 *
 * Author: Mark Wesley
 * Created: 11/17/15
 *
 * Description: Unity driven behavior - a wrapper that allows Unity to drive behavior asynchronously via CLAD messages
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorUnityDriven.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

  
BehaviorUnityDriven::BehaviorUnityDriven(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _isScoredExternally(false)
  , _isRunnable(true)
  , _isComplete(false)
{
  SetDefaultName("UnityDriven");
  
  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
}

  
BehaviorUnityDriven::~BehaviorUnityDriven()
{  
}


Result BehaviorUnityDriven::InitInternal(Robot& robot)
{
  _isRunnable     = true;
  _isComplete     = false;
  
  return Result::RESULT_OK;
}

  
BehaviorUnityDriven::Status BehaviorUnityDriven::UpdateInternal(Robot& robot)
{
  const Status retval = _isComplete ? Status::Complete : Status::Running;
  return retval;
}
  
void BehaviorUnityDriven::StopInternal(Robot& robot)
{
}
  

float BehaviorUnityDriven::EvaluateScoreInternal(const Robot& robot) const
{
  if (_isScoredExternally)
  {
    return _externalScore;
  }
  else
  {
    return IBehavior::EvaluateScoreInternal(robot);
  }
}


void BehaviorUnityDriven::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
}
  

} // namespace Cozmo
} // namespace Anki
