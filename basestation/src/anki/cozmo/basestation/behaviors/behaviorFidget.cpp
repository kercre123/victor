/**
 * File: behaviorFidget.cpp
 *
 * Author: Andrew Stein
 * Date:   8/4/15
 *
 * Description: Implements Cozmo's "Fidgeting" behavior, which is an idle animation
 *              that moves him around in little quick movements periodically.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"

#include "anki/cozmo/basestation/cozmoActions.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

  BehaviorFidget::BehaviorFidget(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  {
    
    // TODO: Add fidget behaviors based on config
    // TODO: Set parameters from config
    
    _minWait_sec = 2.f;
    _maxWait_sec = 5.f;
    
  }
  
  BehaviorFidget::~BehaviorFidget()
  {

  }
  
  Result BehaviorFidget::Init()
  {
    _interrupted = false;
    return RESULT_OK;
  }
  
  IBehavior::Status BehaviorFidget::Update(float currentTime_sec)
  {
    if(_interrupted) {
      // TODO: Do we need to cancel the last commanded fidget action?
      return Status::COMPLETE;
    }
    
    if(currentTime_sec > _lastFidgetTime_sec + _nextFidgetWait_sec) {
    
      // Pick another random fidget action
      FidgetType nextFidget = static_cast<FidgetType>(_rng.RandIntInRange(0, static_cast<s32>(FidgetType::NUM_FIDGETS)));
      
      switch(nextFidget)
      {
        case FidgetType::HEAD_TWITCH:
          _robot.GetActionList().QueueActionNext(IBehavior::sActionSlot,
                                                 new MoveHeadToAngleAction(0, DEG_TO_RAD(2), 30));
          break;
          
        case FidgetType::LIFT_WIGGLE:
        {
          CompoundActionSequential* liftWiggle = new CompoundActionSequential({
            new MoveLiftToHeightAction(32, 2, 5),
            new MoveLiftToHeightAction(32, 2, 5),
            new MoveLiftToHeightAction(32, 2, 5)
          });
          
          _robot.GetActionList().QueueActionNext(IBehavior::sActionSlot, liftWiggle);
          break;
        }
          
        case FidgetType::LIFT_TAP:
          _robot.GetActionList().QueueActionNext(IBehavior::sActionSlot, new PlayAnimationAction("firstTap"));
          break;
          
        case FidgetType::TURN_IN_PLACE:
          _robot.GetActionList().QueueActionNext(IBehavior::sActionSlot, new TurnInPlaceAction(0, DEG_TO_RAD(90)));
          break;
          
        case FidgetType::YAWN:
          PRINT_NAMED_WARNING("BehaviorFidget.Update.YawnUnimplemented", "\n");
          break;
          
        case FidgetType::SNEEZE:
          PRINT_NAMED_WARNING("BehaviorFidget.Update.SneezeUnimplemented", "\n");
          break;
          
        case FidgetType::STRETCH:
          PRINT_NAMED_WARNING("BehaviorFidget.Update.StretchUnimplemented", "\n");
          break;
       
        default:
          PRINT_NAMED_ERROR("BehaviorFidget.Update.InvalidFidgetType",
                            "Tried to execute invalid fidget %d.\n", nextFidget);
          return Status::FAILURE;
          
      } // switch(nextFidget)
      
      // Set next time to fidget
      // TODO: Get min/max wait times from Json config
      _lastFidgetTime_sec = currentTime_sec;
      _nextFidgetWait_sec += _rng.RandDblInRange(_minWait_sec, _maxWait_sec);
    }
    
    return Status::RUNNING;
  } // Update()
  
  Result BehaviorFidget::Interrupt()
  {
    // Mark the behavior as interrupted so it will return COMPLETE on next update
    _interrupted = true;
    
    // TODO: Is there any cleanup that needs to happen?
    
    return RESULT_OK;
  }
  
  bool BehaviorFidget::GetRewardBid(Reward& reward)
  {
    
    // TODO: Fill in reward value
    
    return true;
  }
  
  

} // namespace Cozmo
} // namespace Anki