/**
 * File: retryWrapperAction.cpp
 *
 * Author: Al Chaussee
 * Date:   7/21/16
 *
 * Description: A wrapper action for handling retrying an action and playing retry animations
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/actionWatcher.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/compoundActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {
  
  RetryWrapperAction::RetryWrapperAction(Robot& robot,
                                         IAction* action,
                                         RetryCallback retryCallback,
                                         u8 numRetries)
  : IAction(robot,
            "RetryWrapper",
            RobotActionType::UNKNOWN,
            (u8)AnimTrackFlag::NO_TRACKS)
  , _subAction(action)
  , _retryCallback(retryCallback)
  , _numRetries(numRetries)
  {
    if(action == nullptr)
    {
      PRINT_NAMED_WARNING("RetryWrapperAction.Constructor", "Null action passed to constructor");
      return;
    }
    
    SetType(action->GetType());
    SetName("Retry["+action->GetName()+"]");
  }
  
  RetryWrapperAction::RetryWrapperAction(Robot& robot,
                                         ICompoundAction* action,
                                         RetryCallback retryCallback,
                                         u8 numRetries)
  : IAction(robot,
            "RetryWrapper",
            RobotActionType::UNKNOWN,
            (u8)AnimTrackFlag::NO_TRACKS)
  , _subAction(action)
  , _retryCallback(retryCallback)
  , _numRetries(numRetries)
  {
    if(action == nullptr)
    {
      PRINT_NAMED_WARNING("RetryWrapperAction.Constructor", "Null action passed to constructor");
      return;
    }
    
    // Don't delete actions from the compound action on completion so that they can be retried
    action->SetDeleteActionOnCompletion(false);
    
    SetType(action->GetType());
    SetName("Retry["+action->GetName()+"]");
  }

  RetryWrapperAction::RetryWrapperAction(Robot& robot, IAction* action, AnimationTrigger retryTrigger, u8 numRetries)
    : RetryWrapperAction(robot, action, RetryCallback{}, numRetries)
  {
    _retryCallback = [retryTrigger](const ExternalInterface::RobotCompletedAction&,
                                    const u8 retryCount,
                                    AnimationTrigger& retryAnimTrigger) {
      retryAnimTrigger = retryTrigger;
      return true;
    };
  }

  RetryWrapperAction::RetryWrapperAction(Robot& robot,
                                         ICompoundAction* action,
                                         AnimationTrigger retryTrigger,
                                         u8 numRetries)
    : RetryWrapperAction(robot, action, RetryCallback{}, numRetries)
  {
    _retryCallback = [retryTrigger](const ExternalInterface::RobotCompletedAction&,
                                    const u8 retryCount,
                                    AnimationTrigger& retryAnimTrigger) {
      retryAnimTrigger = retryTrigger;
      return true;
    };
  }
  
  RetryWrapperAction::~RetryWrapperAction()
  {
    if(_subAction != nullptr)
    {
      _subAction->PrepForCompletion();
    }
    
    if(_animationAction != nullptr)
    {
      _animationAction->PrepForCompletion();
    }
  }
  
  f32 RetryWrapperAction::GetTimeoutInSeconds() const
  {
    // Add 1 to account for the initial run
    return (_numRetries+1) * 20.f;
  }
  
  ActionResult RetryWrapperAction::Init()
  {
    if(_subAction == nullptr)
    {
      return ActionResult::NULL_SUBACTION;
    }
    return ActionResult::SUCCESS;
  }
  
  ActionResult RetryWrapperAction::CheckIfDone()
  {
    // Animation action is null so run the subAction
    if(_animationAction == nullptr)
    {
      const ActionResult res = _subAction->Update();
      
      // Update the retryWrapperAction's type to match the subAction's type in case
      // it is changing at runtime
      SetType(_subAction->GetType());
      
      // Only attempt to retry on failure results
      // TODO Could be updated to use ActionResultCategory
      if(res != ActionResult::RUNNING &&
         res != ActionResult::SUCCESS &&
         res != ActionResult::CANCELLED_WHILE_RUNNING &&
         res != ActionResult::INTERRUPTED)
      {
        ActionCompletedUnion completionUnion;
        _subAction->GetCompletionUnion(completionUnion);
        
        std::vector<ActionResult> subActionResults;
        _robot.GetActionList().GetActionWatcher().GetSubActionResults(_subAction->GetTag(), subActionResults);
        
        using RCA = ExternalInterface::RobotCompletedAction;
        RCA robotCompletedAction = RCA(_robot.GetID(),
                                       _subAction->GetTag(),
                                       _subAction->GetType(),
                                       _subAction->GetState(),
                                       subActionResults,
                                       completionUnion);
        
        // Reset the subAction before calling the retryCallback in case the callback modifies
        // things that would be reset by reset (ie action's state)
        _subAction->Reset(true);
        
        PRINT_NAMED_DEBUG("RetryWrapperAction.CheckIfDone", "Calling retry callback");
        AnimationTrigger animTrigger;
        bool shouldRetry = _retryCallback(robotCompletedAction, _retryCount, animTrigger);
        
        // If the action shouldn't retry return whatever its update returned
        if(!shouldRetry)
        {
          return res;
        }
        
        // If the animationTrigger to play is Count (indicates None/No animation) check retry count
        // and don't new the animation action
        if(animTrigger == AnimationTrigger::Count)
        {
          PRINT_NAMED_DEBUG("RetryWrapperAction.CheckIfDone.NoAnimation",
                            "RetryCallback returned AnimationTrigger::Count so not playing animation");
          if(_retryCount++ >= _numRetries)
          {
            PRINT_NAMED_INFO("RetryWrapperAction.CheckIfDone",
                             "Reached max num retries returning failure");
            return ActionResult::REACHED_MAX_NUM_RETRIES;
          }
          return ActionResult::RUNNING;
        }
        else
        {
          _animationAction.reset(new TriggerLiftSafeAnimationAction(_robot, animTrigger));
        }
      }
      else
      {
        return res;
      }
    }
    
    // Animation action not null so run it
    // Note: Doing an if here instead of an else so we can "fallthrough" to here if we just
    // newed the _animationAction
    if(_animationAction != nullptr)
    {
      ActionResult res = _animationAction->Update();
      if(res != ActionResult::RUNNING)
      {
        PRINT_NAMED_DEBUG("RetryWrapperAction.CheckIfDone", "Retry animation finished");
        _animationAction->PrepForCompletion();
        _animationAction.reset();
        
        // Check retry count here so that if we end up reaching our max number of retries
        // this action ends when the animation does
        if(_retryCount++ >= _numRetries)
        {
          PRINT_NAMED_INFO("RetryWrapperAction.CheckIfDone",
                           "Reached max num retries returning failure");
          return ActionResult::REACHED_MAX_NUM_RETRIES;
        }
      }
      return ActionResult::RUNNING;
    }
    
    // Should not be possible to get here
    PRINT_NAMED_WARNING("RetryWrapperAction.CheckIfDone", "Reached supposedly unreachable code");
    return ActionResult::ABORT;
  }
  
  void RetryWrapperAction::GetCompletionUnion(ActionCompletedUnion& completionUnion) const
  {
    if(_subAction != nullptr)
    {
      _subAction->GetCompletionUnion(completionUnion);
    }
  }
  
}
}
