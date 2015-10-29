
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

#define DEBUG_FIDGET_BEHAVIOR 1

namespace Anki {
namespace Cozmo {

  BehaviorFidget::BehaviorFidget(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _totalProb(0)
  {
    _name = "Fidget";
    
    // TODO: Add fidget behaviors based on config
    // TODO: Set parameters (including selection frequencies) from config
    
    _minWait_sec = 2.f;
    _maxWait_sec = 5.f;
    
    AddFidget("HeadTwitch",
              [](){return new MoveHeadToAngleAction(0, DEG_TO_RAD(2), 30);}
              , 1);
    
    AddFidget("LiftWiggle", [](){return new CompoundActionSequential({
      new MoveLiftToHeightAction(32, 2, 5),
      new MoveLiftToHeightAction(32, 2, 5),
      new MoveLiftToHeightAction(32, 2, 5)});},
              1);
    
    AddFidget("LiftTap", [this](){return new PlayAnimationAction("firstTap", GetRNG().RandIntInRange(1, 3));}, 1);
    
    AddFidget("TurnInPlace", [](){return new TurnInPlaceAction(0, false, DEG_TO_RAD(90));}, 2);
    
    // TODO: Make probabilities non-zero once we have these animations available
    AddFidget("Yawn", [](){return new PlayAnimationAction("Yawn");}, 0);
    AddFidget("Stretch", [](){return new PlayAnimationAction("Stretch");}, 0);
    AddFidget("Sneeze", [](){return new PlayAnimationAction("Sneeze");}, 0);
    
    // Bored -> Fidget
    AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.1f}}), false));
  }
  
  void BehaviorFidget::AddFidget(const std::string& name, MakeFidgetAction fcn,
                                 s32 frequency)
  {
    if(frequency > 0) {
      _totalProb += frequency;
      _fidgets[_totalProb] = std::make_pair(name, fcn);
    }
  }
  
  BehaviorFidget::~BehaviorFidget()
  {

  }
  
  Result BehaviorFidget::InitInternal(Robot& robot, double currentTime_sec)
  {
    _interrupted = false;
    _nextFidgetWait_sec = 0.f;
    _lastFidgetTime_sec = 0.f;
    
    return RESULT_OK;
  }
  
  IBehavior::Status BehaviorFidget::UpdateInternal(Robot& robot, double currentTime_sec)
  {
    if(_interrupted) {
      // TODO: Do we need to cancel the last commanded fidget action?
      return Status::Complete;
    }
    
    if(currentTime_sec > _lastFidgetTime_sec + _nextFidgetWait_sec) {
    
      // Pick another random fidget action
      s32 prob = GetRNG().RandIntInRange(0, _totalProb);

      auto fidgetIter = _fidgets.begin();
      while(prob > fidgetIter->first) {
        ++fidgetIter;
      }
      
#     if DEBUG_FIDGET_BEHAVIOR
      PRINT_NAMED_INFO("BehaviorFidget.Update.Selection",
                       "Random prob=%d out of totalProb=%d, selected '%s'.\n",
                       prob, _totalProb, fidgetIter->second.first.c_str());
#     endif
      
      // Set the name based on the selected fidget and then queue the action
      // returned by its MakeFidgetAction function
      _stateName = fidgetIter->second.first;
      robot.GetActionList().QueueActionNext(IBehavior::sActionSlot,
                                             fidgetIter->second.second());
      
      // Set next time to fidget
      // TODO: Get min/max wait times from Json config
      _lastFidgetTime_sec = currentTime_sec;
      _nextFidgetWait_sec += GetRNG().RandDblInRange(_minWait_sec, _maxWait_sec);
      
#     if DEBUG_FIDGET_BEHAVIOR
      PRINT_NAMED_INFO("BehaviorFidget.Update.Times",
                       "Setting lastFidgetTime=%f and wait to next = %f\n",
                       _lastFidgetTime_sec, _nextFidgetWait_sec);
#     endif
    }
    
    return Status::Running;
  } // Update()
  
  Result BehaviorFidget::InterruptInternal(Robot& robot, double currentTime_sec)
  {
    // Mark the behavior as interrupted so it will return COMPLETE on next update
    _interrupted = true;
    
    // TODO: Is there any cleanup that needs to happen?
    
    return RESULT_OK;
  }
  
  

} // namespace Cozmo
} // namespace Anki