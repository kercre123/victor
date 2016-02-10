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
#include "util/helpers/templateHelpers.h"

#define DEBUG_ANIM_TRACK_LOCKING 0

#define DEBUG_ACTION_RUNNING 0

namespace Anki {
  
  namespace Cozmo {
    
    u32 IActionRunner::sTagCounter = 100000;
    
    IActionRunner::IActionRunner(Robot& robot)
    : _robot(robot)
    {
      // Assign every action a unique tag
      if (++IActionRunner::sTagCounter == static_cast<u32>(ActionConstants::INVALID_TAG)) {
        ++IActionRunner::sTagCounter;
      }
      
      _idTag = IActionRunner::sTagCounter;
    }
    
    IActionRunner::~IActionRunner()
    {
      if (_emitCompletionSignal && ActionResult::INTERRUPTED != _result)
      {
        // Notify any listeners about this action's completion.
        // TODO: Populate the signal with any action-specific info?
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCompletedAction(_robot.GetID(), _idTag, _type, _result, _completionUnion)));
      }
    
      if(!_suppressTrackLocking)
      {
#       if DEBUG_ANIM_TRACK_LOCKING
        PRINT_NAMED_INFO("IActionRunner.Destroy.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                         _tracks,
                         AnimTrackFlagToString((AnimTrackFlag)_tracks),
                         _name.c_str(),
                         _idTag);
#       endif
        _robot.GetMoveComponent().UnlockTracks(_tracks);
      }
    }

    
    void IActionRunner::SetTag(u32 tag)
    {
      if (tag == static_cast<u32>(ActionConstants::INVALID_TAG)) {
        PRINT_NAMED_ERROR("IActionRunner.SetTag.InvalidTag", "INVALID_TAG==%d", ActionConstants::INVALID_TAG);
      } else {
        _idTag = tag;
      }
    }
    
    bool IActionRunner::Interrupt()
    {
      _isInterrupted = InterruptInternal();
      if(_isInterrupted) {
        Reset();
        
        // Only need to unlock tracks if this is running because Init() locked tracks
        if(!_suppressTrackLocking && _isRunning)
        {
          uint8_t tracks = GetTracksToLock();
#         if DEBUG_ANIM_TRACK_LOCKING
          PRINT_NAMED_INFO("IActionRunner.Interrupt.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                           tracks,
                           AnimTrackFlagToString((AnimTrackFlag)tracks),
                           _name.c_str(),
                           _idTag);
#         endif

          _robot.GetMoveComponent().UnlockTracks(tracks);
        }
        
        _isRunning = false;
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
        uint8_t tracksToLock = GetTracksToLock();
        
        if(_robot.GetMoveComponent().IsTrackLocked((AnimTrackFlag)tracksToLock))
        {
          PRINT_NAMED_WARNING("IActionRunner.Update",
                           "Action %s [%d] not running because required tracks (0x%x) %s are locked",
                           GetName().c_str(),
                           GetTag(),
                           tracksToLock,
                           AnimTrackFlagToString((AnimTrackFlag)tracksToLock));
          _result = ActionResult::FAILURE_TRACKS_LOCKED;
          return ActionResult::FAILURE_TRACKS_LOCKED;
        }
        
#       if DEBUG_ANIM_TRACK_LOCKING
        PRINT_NAMED_INFO("IActionRunner.Update.LockTracks", "locked: (0x%x) %s by %s [%d]",
                         tracksToLock,
                         AnimTrackFlagToString((AnimTrackFlag)tracksToLock),
                         GetName().c_str(),
                         GetTag());

#       endif

        _robot.GetMoveComponent().LockTracks(tracksToLock);

      }

      if( ! _isRunning ) {
        if( DEBUG_ACTION_RUNNING && _displayMessages ) {
          PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s running",
                            GetTag(),
                            GetName().c_str());
        }

        _isRunning = true;
      }
      _result = ActionResult::RUNNING;
      if(_isCancelled) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.CancelAction",
                           "Cancelling [%d] %s.", _idTag, GetName().c_str());
        }
        _result = ActionResult::CANCELLED;
      } else if(_isInterrupted) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.InterruptAction",
                           "Interrupting [%d] %s", _idTag, GetName().c_str());
        }
        _result = ActionResult::INTERRUPTED;
      } else {
        _result = UpdateInternal();
      }
      
      if(_result != ActionResult::RUNNING) {
        if(_displayMessages) {
          PRINT_NAMED_INFO("IActionRunner.Update.ActionCompleted",
                           "%s [%d] %s.", GetName().c_str(),
                           _idTag,
                           (_result==ActionResult::SUCCESS ? "succeeded" :
                            _result==ActionResult::CANCELLED ? "was cancelled" : "failed"));
        }

        if( DEBUG_ACTION_RUNNING && _displayMessages ) {
          PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s NOT running",
                            GetTag(),
                            GetName().c_str());
        }
        _isRunning = false;
      }

      return _result;
    }
    
    void IActionRunner::PrepForCompletion()
    {
      GetCompletionUnion(_completionUnion);
      _type = GetType();
      _name = GetName();
      _tracks = GetTracksToLock();
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

    u8 IActionRunner::GetTracksToLock() const
    {
      return (uint8_t)AnimTrackFlag::HEAD_TRACK | (uint8_t)AnimTrackFlag::LIFT_TRACK | (uint8_t)AnimTrackFlag::BODY_TRACK;
    }
    
    void IActionRunner::UnlockTracks()
    {
      if(!_suppressTrackLocking)
      {
#         if DEBUG_ANIM_TRACK_LOCKING
        PRINT_NAMED_INFO("IActionRunner.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                         _tracks,
                         AnimTrackFlagToString((AnimTrackFlag)_tracks),
                         _name.c_str(),
                         _idTag);
#         endif
        _robot.GetMoveComponent().UnlockTracks(_tracks);
      }
    }
    
#pragma mark ---- IAction ----
    
    IAction::IAction(Robot& robot)
    : IActionRunner(robot)
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
                           _robot.GetID(), GetName().c_str());
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
