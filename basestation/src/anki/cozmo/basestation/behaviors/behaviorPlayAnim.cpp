/**
 * File: behaviorPlayAnim
 *
 * Author: Mark Wesley
 * Created: 11/03/15
 *
 * Description: Simple Behavior to play an animation
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"


namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
  
static const char* kAnimNameKey = "animName";
  

BehaviorPlayAnim::BehaviorPlayAnim(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _animationName()
  , _loopCount(1)
  , _loopsLeft(0)
  , _lastActionTag(0)
  , _isInterruptable(false)
  , _isInterrupted(false)
  , _isActing(false)
{
  SetDefaultName("PlayAnim");
  
  if (!config.isNull())
  {
    {
      const Json::Value& valueJson = config[kAnimNameKey];
      if (valueJson.isString())
      {
        _animationName = valueJson.asCString();
      }
    }
  }
  
  SubscribeToTags({
    EngineToGameTag::RobotCompletedAction
  });
}
  
  
BehaviorPlayAnim::~BehaviorPlayAnim()
{  
}

  
void BehaviorPlayAnim::SetAnimationName(const std::string& inName)
{
  _animationName = inName;
}

  
bool BehaviorPlayAnim::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  const bool retVal = true;
  return retVal;
}
  

Result BehaviorPlayAnim::InitInternal(Robot& robot, double currentTime_sec)
{
  _isInterrupted = false;
  _isActing = false;
  _loopsLeft = _loopCount;
  _lastActionTag = 0;
  
  return Result::RESULT_OK;
}

  
BehaviorPlayAnim::Status BehaviorPlayAnim::UpdateInternal(Robot& robot, double currentTime_sec)
{
  if (!_isActing && !_isInterrupted && ((_loopsLeft > 0) || (_loopCount < 0)))
  {
    PlayAnimation(robot, _animationName);
  }
  
  Status retval = (_isInterrupted || !_isActing) ? Status::Complete : Status::Running;
  return retval;
}


Result BehaviorPlayAnim::InterruptInternal(Robot& robot, double currentTime_sec)
{
  if (_isInterruptable)
  {
    _isInterrupted = true;
  }
  
  return Result::RESULT_OK;
}

  
void BehaviorPlayAnim::StopInternal(Robot& robot, double currentTime_sec)
{
}


void BehaviorPlayAnim::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotCompletedAction:
      {
        const RobotCompletedAction& msg = event.GetData().Get_RobotCompletedAction();
        
        if (msg.idTag == _lastActionTag)
        {
          _isActing = false;
        }
      }
      break;
      
    default:
      PRINT_NAMED_ERROR("BehaviorPlayAnim.HandleWhileRunning.InvalidTag",
                        "Received event with unhandled tag %hhu.",
                        event.GetData().GetTag());
      break;
  }

}


void BehaviorPlayAnim::PlayAnimation(Robot& robot, const std::string& animName)
{
  --_loopsLeft;  
  SetStateName("Play" + std::to_string(_loopsLeft));
  
  PlayAnimationAction* animAction = new PlayAnimationAction(animName);
  robot.GetActionList().QueueActionNow(animAction);
  _lastActionTag = animAction->GetTag();
  _isActing = true;
}
  

} // namespace Cozmo
} // namespace Anki
