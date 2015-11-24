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
  

static const std::string kBaseBehaviorName = "UnityDriven";
  
  
BehaviorUnityDriven::BehaviorUnityDriven(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _isShortInterruption(false)
  , _isScoredExternally(false)
  , _isRunnable(true)
  , _wasInterrupted(false)
  , _isComplete(false)
  , _isResuming(false)
{
  _name = kBaseBehaviorName;
  
  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
}

  
BehaviorUnityDriven::~BehaviorUnityDriven()
{  
}


Result BehaviorUnityDriven::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  _isRunnable     = true;
  _wasInterrupted = false;
  _isComplete     = false;
  _isResuming     = isResuming;
  
  return Result::RESULT_OK;
}

  
BehaviorUnityDriven::Status BehaviorUnityDriven::UpdateInternal(Robot& robot, double currentTime_sec)
{
  const Status retval = _isComplete ? Status::Complete : Status::Running;
  return retval;
}


Result BehaviorUnityDriven::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  _wasInterrupted = true;
  return Result::RESULT_OK;
}


float BehaviorUnityDriven::EvaluateScoreInternal(const Robot& robot, double currentTime_sec) const
{
  if (_isScoredExternally)
  {
    return _externalScore;
  }
  else
  {
    return IBehavior::EvaluateScoreInternal(robot, currentTime_sec);
  }
}


void BehaviorUnityDriven::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
}
  

} // namespace Cozmo
} // namespace Anki
