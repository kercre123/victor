
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

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/types/actionTypes.h"

#define DEBUG_FIDGET_BEHAVIOR 1

namespace Anki {
namespace Cozmo {

  BehaviorFidget::BehaviorFidget(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _queuedActionTag((uint32_t)ActionConstants::INVALID_TAG)
  {
    SetDefaultName("Fidget");
    
    // TODO: Add fidget behaviors based on config
    // TODO: Set parameters (including selection frequencies) from config
    
    _minWait_sec = 2.f;
    _maxWait_sec = 5.f;

    AddFidget("Brickout", [this](){return new PlayAnimationAction("ID_idle_brickout_02");}, 1, 60, true);
    
    // TODO: Make probabilities non-zero once we have these animations available
    /*
    AddFidget("Yawn", [](){return new PlayAnimationAction("Yawn");}, 0);
    AddFidget("Stretch", [](){return new PlayAnimationAction("Stretch");}, 0);
    AddFidget("Sneeze", [](){return new PlayAnimationAction("Sneeze");}, 0);
     */
    
    SubscribeToTags({EngineToGameTag::RobotCompletedAction});
    
    if (GetEmotionScorerCount() == 0)
    {
      // Bored -> Fidget
      AddEmotionScorer(EmotionScorer(EmotionType::Excited, Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.1f}}), false));
    }
  }
  
  void BehaviorFidget::AddFidget(const std::string& name,
                                 MakeFidgetAction fcn,
                                 s32 frequency,
                                 float minTimeBetween/* = 0*/,
                                 bool mustComplete/* = false*/)
  {
    if(frequency > 0) {
      _fidgets.push_back({._name=name,
                          ._function=fcn,
                          ._frequency=frequency,
                          ._minTimeBetween_sec=minTimeBetween,
                          ._mustComplete=mustComplete});
    }
  }
  
  BehaviorFidget::~BehaviorFidget()
  {

  }
  
  Result BehaviorFidget::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
  {
    _interrupted = false;
    
    // Note we don't clear out the _lastFidgetTime and fidgetWait here, as we want the waiting
    // to persist between switching to other behaviors and coming back
    
    return RESULT_OK;
  }
  
  IBehavior::Status BehaviorFidget::UpdateInternal(Robot& robot, double currentTime_sec)
  {
    if(_interrupted)
    {
      // If we don't have any actions in progress we can be done
      if (_queuedActionTag == (uint32_t)ActionConstants::INVALID_TAG)
      {
        return Status::Complete;
      }
      
      // If we have actions in progress that can't be interrupted, keep running
      if (_currentActionMustComplete)
      {
        return Status::Running;
      }
      
      // Otherwise cancel our running action and complete
      robot.GetActionList().Cancel(_queuedActionTag);
      return Status::Complete;
    }
    
    // If we already have an action queued up don't bother looking for more, just keep running
    if (_queuedActionTag != (uint32_t)ActionConstants::INVALID_TAG)
    {
      return Status::Running;
    }
    
    if(currentTime_sec > _lastFidgetTime_sec + _nextFidgetWait_sec)
    {
      // Make a map of the fidgets that aren't on cooldown to their probability slice to randomly choose between
      int32_t totalProb = 0;
      std::map<int32_t, FidgetData*> fidgetChances;
      for (auto& fidget : _fidgets)
      {
        if (fidget._lastTimeUsed_sec == 0 || (fidget._lastTimeUsed_sec + fidget._minTimeBetween_sec) < currentTime_sec)
        {
          totalProb += fidget._frequency;
          fidgetChances[totalProb] = &fidget;
        }
      }
      
      // Assuming we have any to choose from, pick a random fidget to use
      if (totalProb > 0)
      {
        // Pick another random fidget action
        s32 prob = GetRNG().RandIntInRange(0, totalProb);

        auto fidgetIter = fidgetChances.begin();
        while(prob > fidgetIter->first) {
          ++fidgetIter;
        }
        
  #     if DEBUG_FIDGET_BEHAVIOR
        PRINT_NAMED_INFO("BehaviorFidget.Update.Selection",
                         "Random prob=%d out of totalProb=%d, selected '%s'.\n",
                         prob, totalProb, fidgetIter->second->_name.c_str());
  #     endif
        
        // Set the name based on the selected fidget and then queue the action
        // returned by its MakeFidgetAction function
        SetStateName(fidgetIter->second->_name);
        IActionRunner* fidgetAction = fidgetIter->second->_function();
        _queuedActionTag = fidgetAction->GetTag();
        _currentActionMustComplete = fidgetIter->second->_mustComplete;
        
        robot.GetActionList().QueueActionNext(IBehavior::sActionSlot, fidgetAction);
        
        fidgetIter->second->_lastTimeUsed_sec = currentTime_sec;
      }
      
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
  
  Result BehaviorFidget::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
  {
    // Mark the behavior as interrupted so it will handle on next update
    _interrupted = true;
    
    return RESULT_OK;
  }
  
  void BehaviorFidget::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
  {
    switch (event.GetData().GetTag())
    {
      case EngineToGameTag::RobotCompletedAction:
      {
        if (_queuedActionTag != (uint32_t)ActionConstants::INVALID_TAG)
        {
          auto& msg = event.GetData().Get_RobotCompletedAction();
          if (msg.idTag == _queuedActionTag)
          {
            _queuedActionTag = (uint32_t)ActionConstants::INVALID_TAG;
            SetStateName("");
            _currentActionMustComplete = false;
          }
        }
      }
      break;
      default:
        PRINT_NAMED_DEBUG("BehaviorFidget::AlwaysHandle::UnexpectedEvent",
                          "Not handling unexpected event type %s",
                          ExternalInterface::MessageEngineToGameTagToString(event.GetData().GetTag()));
        
    }
  }
  

} // namespace Cozmo
} // namespace Anki