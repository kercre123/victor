/**
 * File: actionInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements interfaces for action states for a robot.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/components/animTrackHelpers.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/types/animationKeyFrames.h"
#include "util/helpers/templateHelpers.h"

#define DEBUG_ANIM_TRACK_LOCKING 0

#define DEBUG_ACTION_RUNNING 0

namespace Anki {
  
  namespace Cozmo {
    
    u32 IActionRunner::sTagCounter = 0;
    std::map<u32, u32> IActionRunner::sUnityToEngineTagMap;
    
    IActionRunner::IActionRunner()
    {
      // Assign every action a unique tag
      if (++IActionRunner::sTagCounter == static_cast<u32>(ActionConstants::INVALID_TAG)) {
        ++IActionRunner::sTagCounter;
      }
      
      // If this tag already exists find one that doesn't
      while(!IActionRunner::sUnityToEngineTagMap.emplace(IActionRunner::sTagCounter, IActionRunner::sTagCounter).second)
      {
        ++IActionRunner::sTagCounter;
      }
      
      _idTag = IActionRunner::sTagCounter;
    }
    
    void IActionRunner::SetTag(u32 tag)
    {
      if (tag == static_cast<u32>(ActionConstants::INVALID_TAG)) {
        PRINT_NAMED_ERROR("IActionRunner.SetTag.InvalidTag", "INVALID_TAG==%d", ActionConstants::INVALID_TAG);
      } else {
        auto pair = IActionRunner::sUnityToEngineTagMap.emplace(tag, _idTag);
        if(!pair.second) {
          PRINT_NAMED_WARNING("IActionRunner.SetTag.InvalidTag", "Tag [%d] already exists", tag);
        }
      }
    }
    
    u32 IActionRunner::GetUnityTag() const
    {
      for(auto pair : IActionRunner::sUnityToEngineTagMap)
      {
        // Since the mapping _idTag -> _idTag exists skip it by comparing pair.first to _idTag
        if(pair.second == _idTag && pair.first != _idTag)
        {
          return pair.first;
        }
      }
      return _idTag;
    }
    
    bool IActionRunner::Interrupt()
    {
      _isInterrupted = InterruptInternal();
      if(_isInterrupted) {
        Reset();
      }
      return _isInterrupted;
    }
    
    // NOTE: THere should be no way for Update() to fail independently of its
    // call to UpdateInternal(). Otherwise, there's a possibility for an
    // IAction's Cleanup() method not be called on failure.
    ActionResult IActionRunner::Update()
    {
      if(!_isRunning && !_suppressTrackLocking) {
        // When the ActionRunner first starts, lock any specified subsystems
        uint8_t disableTracks = GetAnimTracksToDisable();
#       if DEBUG_ANIM_TRACK_LOCKING
        PRINT_NAMED_INFO("IActionRunner.Update.LockTracks", "locked: (0x%x) %s by %s [%d]",
                         disableTracks,
                         AnimTrackHelpers::AnimTrackFlagsToString(disableTracks).c_str(),
                         GetName().c_str(),
                         GetTag());

#       endif
        _robot->GetMoveComponent().LockAnimTracks(disableTracks);
        _robot->GetMoveComponent().IgnoreTrackMovement(GetMovementTracksToIgnore());
      }

      if( ! _isRunning ) {
        if( DEBUG_ACTION_RUNNING && _displayMessages ) {
          PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s running",
                            GetTag(),
                            GetName().c_str());
        }

        _isRunning = true;
      }
      ActionResult result = ActionResult::RUNNING;
      if(_isCancelled) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.CancelAction",
                           "Cancelling [%d] %s.", _idTag, GetName().c_str());
        }
        result = ActionResult::CANCELLED;
      } else if(_isInterrupted) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.InterruptAction",
                           "Interrupting [%d] %s", _idTag, GetName().c_str());
        }
        result = ActionResult::INTERRUPTED;
      } else {
        result = UpdateInternal();
      }
      
      if(result != ActionResult::RUNNING) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.ActionCompleted",
                           "%s [%d] %s.", GetName().c_str(),
                           _idTag,
                           (result==ActionResult::SUCCESS ? "succeeded" :
                            result==ActionResult::CANCELLED ? "was cancelled" : "failed"));
        }

        // Clean up after any registered sub actions and the action itself
        CancelAndDeleteSubActions();
        Cleanup();

        if (_emitCompletionSignal && ActionResult::INTERRUPTED != result)
        {
          // Notify any listeners about this action's completion.
          // Note that I do this here so that compound actions only emit one signal,
          // not a signal for each constituent action.
          // TODO: Populate the signal with any action-specific info?
          EmitCompletionSignal(result);
        }

        if(!_suppressTrackLocking) {
          uint8_t disableTracks = GetAnimTracksToDisable();
#         if DEBUG_ANIM_TRACK_LOCKING
          PRINT_NAMED_INFO("IActionRunner.Update.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                           disableTracks,
                           AnimTrackHelpers::AnimTrackFlagsToString(disableTracks).c_str(),
                           GetName().c_str(),
                           GetTag());
#         endif

          _robot->GetMoveComponent().UnlockAnimTracks(disableTracks);
          _robot->GetMoveComponent().UnignoreTrackMovement(GetMovementTracksToIgnore());
        }

        if( DEBUG_ACTION_RUNNING && _displayMessages ) {
          PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s NOT running",
                            GetTag(),
                            GetName().c_str());
        }
        _isRunning = false;
      }

      return result;
    }
    
    void IActionRunner::EmitCompletionSignal(ActionResult result) const
    {
      ActionCompletedUnion completionUnion;

      GetCompletionUnion(completionUnion);
      
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCompletedAction(_robot->GetID(), GetUnityTag(), GetType(), result, completionUnion)));
    }
    
    bool IActionRunner::RetriesRemain()
    {
      if(_numRetriesRemaining > 0) {
        --_numRetriesRemaining;
        return true;
      } else {
        return false;
      }
    }
    
    const std::string& IActionRunner::GetName() const {
      static const std::string name("UnnamedAction");
      return name;
    }
    
#   if USE_ACTION_CALLBACKS
    void IActionRunner::AddCompletionCallback(ActionCompletionCallback callback)
    {
      _completionCallbacks.emplace_back(callback);
    }
    
    void IActionRunner::RunCallbacks(ActionResult result) const
    {
      for(auto callback : _completionCallbacks) {
        callback(result);
      }
    }
#   endif // USE_ACTION_CALLBACKS
    
    u8 IActionRunner::GetMovementTracksToIgnore() const
    {
      return  (uint8_t)AnimTrackFlag::HEAD_TRACK | (uint8_t)AnimTrackFlag::LIFT_TRACK | (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    
    bool IActionRunner::RegisterSubAction(IActionRunner* &subAction)
    {
      PRINT_NAMED_DEBUG("IActionRunner.CreatedSubAction", "Parent action [%d] %s created a sub action [%d] %s",
                        GetTag(),
                        GetName().c_str(),
                        subAction->GetTag(),
                        subAction->GetName().c_str());
      
      _subActions.push_back(&subAction);
      if(nullptr == GetRobot())
      {
        PRINT_NAMED_INFO("IActionRunner.RegisterSubAction",
                         "Parent action's robot pointer is null, returning Failure_Abort");
        return false;
      }
      subAction->SetRobot(*GetRobot());
      return true;
    }
    
    void IActionRunner::CancelAndDeleteSubActions()
    {
      if(!_subActions.empty())
      {
        for(auto subAction : _subActions) {
          if( nullptr != (*subAction) ) {
            if( (*subAction)->_isRunning ) {
              if( DEBUG_ACTION_RUNNING && _displayMessages ) {
                PRINT_NAMED_DEBUG("IActionRunner.CancelAndDeleteSubActions",
                                  "Removing subAction %s [%d] (parent action is %s [%d])",
                                  (*subAction)->GetName().c_str(), (*subAction)->GetTag(),
                                  GetName().c_str(), GetTag());
              }
            
              (*subAction)->Cancel();
              (*subAction)->Update();
            } // if running
            else if( DEBUG_ACTION_RUNNING && _displayMessages ) {
              PRINT_NAMED_DEBUG("IActionRunner.CancelAndDeleteSubActions.Skip",
                                "skipping sub action  %s [%d] because it isn't running (parent action is %s [%d])",
                                (*subAction)->GetName().c_str(), (*subAction)->GetTag(),
                                GetName().c_str(), GetTag());
            }
            
            Util::SafeDelete(*subAction);
          } // if not null
        }
      }
    }
    
    void IActionRunner::SetRobot(Robot& robot)
    {
      _robot = &robot;
    }
    
    
#pragma mark ---- IAction ----
    
    IAction::IAction()
    {
      Reset();
    }
    
    void IAction::Reset()
    {
      _preconditionsMet = false;
      _waitUntilTime = -1.f;
      _timeoutTime = -1.f;
    }
    
    ActionResult IAction::UpdateInternal()
    {
      ActionResult result = ActionResult::RUNNING;
      SetStatus(GetName());
      
      // On first call to Update(), figure out the waitUntilTime
      const f32 currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_waitUntilTime < 0.f) {
        _waitUntilTime = currentTimeInSeconds + GetStartDelayInSeconds();
      }
      if(_timeoutTime < 0.f) {
        _timeoutTime = currentTimeInSeconds + GetTimeoutInSeconds();
      }

      // Fail if we have exceeded timeout time
      if(currentTimeInSeconds >= _timeoutTime) {
        if(IsMessageDisplayEnabled()) {
          PRINT_NAMED_INFO("IAction.Update.TimedOut",
                           "%s timed out after %.1f seconds.",
                           GetName().c_str(), GetTimeoutInSeconds());
        }
        result = ActionResult::FAILURE_TIMEOUT;
      }
      
      // Don't do anything until we have reached the waitUntilTime
      else if(currentTimeInSeconds >= _waitUntilTime)
      {
        if(!_preconditionsMet) {
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking preconditions.", GetName().c_str());
          SetStatus(GetName() + ": check preconditions");
          
          // Before calling Init(), clean up any subactions, in case this is not
          // the first call to Init() -- i.e., if this is a retry or resume after
          // being interrupted.
          CancelAndDeleteSubActions();

          // Note that derived classes will define what to do when pre-conditions
          // are not met: if they return RUNNING, then the action will effectively
          // just wait for the preconditions to be met. Otherwise, a failure
          // will get propagated out as the return value of the Update method.
          result = Init();

          if(result == ActionResult::SUCCESS) {
            if(IsMessageDisplayEnabled()) {
              PRINT_NAMED_INFO("IAction.Update.PreconditionsMet",
                               "Preconditions for %s [%d] successfully met.",
                               GetName().c_str(),
                               GetTag());
            }
            
            // If preconditions were successfully met, switch result to RUNNING
            // so that we don't think the entire action is completed. (We still
            // need to do CheckIfDone() calls!)
            // TODO: there's probably a tidier way to do this.
            _preconditionsMet = true;
            result = ActionResult::RUNNING;
            
            // Don't check if done until a sufficient amount of time has passed
            // after preconditions are met
            _waitUntilTime = currentTimeInSeconds + GetCheckIfDoneDelayInSeconds();
          }
        }

        // Re-check if preconditions are met, since they could have _just_ been met
        if(_preconditionsMet && currentTimeInSeconds >= _waitUntilTime) {
          //PRINT_NAMED_INFO("IAction.Update", "Updating %s: checking if done.", GetName().c_str());
          SetStatus(GetName() + ": check if done");
          
          // Pre-conditions already met, just run until done
          result = CheckIfDone();
        }
      } // if(currentTimeInSeconds > _waitUntilTime)
      
      if(result == ActionResult::FAILURE_RETRY && RetriesRemain()) {
        if(IsMessageDisplayEnabled()) {
          PRINT_NAMED_INFO("IAction.Update.CurrentActionFailedRetrying",
                           "Robot %d failed running action %s. Retrying.",
                           _robot->GetID(), GetName().c_str());
        }
        
        Reset();
        result = ActionResult::RUNNING;
      }

#     if USE_ACTION_CALLBACKS
      if(result != ActionResult::RUNNING) {
        RunCallbacks(result);
      }
#     endif
      return result;
    } // UpdateInternal()
    
    
  } // namespace Cozmo
} // namespace Anki
