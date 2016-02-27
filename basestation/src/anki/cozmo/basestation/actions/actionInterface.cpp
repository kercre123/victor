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
    
    u32 IActionRunner::sTagCounter = 10000;
    std::set<u32> IActionRunner::sInUseTagSet;
    
    IActionRunner::IActionRunner(Robot& robot)
    : _robot(robot)
    {
      // Assign every action a unique tag that is not currently in use
      while (IActionRunner::sTagCounter == static_cast<u32>(ActionConstants::INVALID_TAG) ||
             !IActionRunner::sInUseTagSet.insert(IActionRunner::sTagCounter).second) {
        ++IActionRunner::sTagCounter;
      }
      
      _idTag = IActionRunner::sTagCounter++;
      _customTag = _idTag;
    }
    
    IActionRunner::~IActionRunner()
    {
      // Erase the tags as they are no longer in use
      IActionRunner::sInUseTagSet.erase(_customTag);
      IActionRunner::sInUseTagSet.erase(_idTag);
      
      if (_emitCompletionSignal && ActionResult::INTERRUPTED != _state)
      {
        // Notify any listeners about this action's completion.
        // TODO: Populate the signal with any action-specific info?
        _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotCompletedAction(_robot.GetID(), GetTag(), _type, _state, _completionUnion)));
      }
    
      if(!_suppressTrackLocking && _state != ActionResult::FAILURE_NOT_STARTED)
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

    
    bool IActionRunner::SetTag(u32 tag)
    {
      // Probably a bad idea to be able to change the tag while the action is running
      if(_state == ActionResult::RUNNING)
      {
        PRINT_NAMED_WARNING("IActionRunner.SetTag", "Action %s [%d] is running unable to set tag to %d",
                            GetName().c_str(),
                            GetTag(),
                            tag);
        _state = ActionResult::FAILURE_BAD_TAG;
        return false;
      }
      
      // If the tag has already been set and the action is not running then erase the current tag in order to
      // set the new one
      if(_customTag != _idTag)
      {
        IActionRunner::sInUseTagSet.erase(_customTag);
      }
      // If this is an invalid tag or is currently in use
      if(tag == static_cast<u32>(ActionConstants::INVALID_TAG) ||
         !IActionRunner::sInUseTagSet.insert(tag).second)
      {
        PRINT_NAMED_WARNING("IActionRunner.SetTag.InvalidTag", "Tag [%d] is invalid", tag);
        _state = ActionResult::FAILURE_BAD_TAG;
        return false;
      }
      _customTag = tag;
      return true;
    }
    
    bool IActionRunner::Interrupt()
    {
      if(InterruptInternal())
      {
        // Only need to unlock tracks if this is running because Update() locked tracks
        if(!_suppressTrackLocking && _state == ActionResult::RUNNING)
        {
          u8 tracks = GetTracksToLock();
#         if DEBUG_ANIM_TRACK_LOCKING
          PRINT_NAMED_INFO("IActionRunner.Interrupt.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                           tracks,
                           AnimTrackFlagToString((AnimTrackFlag)tracks),
                           _name.c_str(),
                           _idTag);
#         endif

          _robot.GetMoveComponent().UnlockTracks(tracks);
        }
        Reset(false);
        _state = ActionResult::INTERRUPTED;
        return true;
      }
      return false;
    }
    
    ActionResult IActionRunner::Update()
    {
      switch(_state)
      {
        case ActionResult::FAILURE_RETRY:
        case ActionResult::FAILURE_NOT_STARTED:
        case ActionResult::INTERRUPTED:
        {
          _state = ActionResult::RUNNING;
          if(!_suppressTrackLocking)
          {
            // When the ActionRunner first starts, lock any specified subsystems
            u8 tracksToLock = GetTracksToLock();
            
            if(_robot.GetMoveComponent().AreAnyTracksLocked(tracksToLock))
            {
              PRINT_NAMED_WARNING("IActionRunner.Update",
                                  "Action %s [%d] not running because required tracks (0x%x) %s are locked",
                                  GetName().c_str(),
                                  GetTag(),
                                  tracksToLock,
                                  AnimTrackFlagToString((AnimTrackFlag)tracksToLock));
              _state = ActionResult::FAILURE_TRACKS_LOCKED;
              return ActionResult::FAILURE_TRACKS_LOCKED;
            }
            
#           if DEBUG_ANIM_TRACK_LOCKING
            PRINT_NAMED_INFO("IActionRunner.Update.LockTracks", "locked: (0x%x) %s by %s [%d]",
                             tracksToLock,
                             AnimTrackFlagToString((AnimTrackFlag)tracksToLock),
                             GetName().c_str(),
                             GetTag());
#           endif
            
            _robot.GetMoveComponent().LockTracks(tracksToLock);
          }
          
          if( DEBUG_ACTION_RUNNING && _displayMessages )
          {
            PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s running",
                              GetTag(),
                              GetName().c_str());
          }
        }
        case ActionResult::RUNNING:
        {
          _state = UpdateInternal();
          
          if(_state == ActionResult::RUNNING)
          {
            // Still running dont fall through
            break;
          }
          // UpdateInternal() returned something other than running so fall through to handle action
          // completion
        }
        // Every other case is a completion case (ie the action is no longer running due to success, failure, or
        // cancel)
        default:
        {
          if(_displayMessages) {
            PRINT_NAMED_INFO("IActionRunner.Update.ActionCompleted",
                             "%s [%d] %s.", GetName().c_str(),
                             GetTag(),
                             (_state==ActionResult::SUCCESS ? "succeeded" :
                              _state==ActionResult::CANCELLED ? "was cancelled" : "failed"));
          }
          
          PrepForCompletion();
          
          if( DEBUG_ACTION_RUNNING && _displayMessages ) {
            PRINT_NAMED_DEBUG("IActionRunner.Update.IsRunning", "Action [%d] %s NOT running",
                              GetTag(),
                              GetName().c_str());
          }
        }
      }
      return _state;
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
      return (u8)AnimTrackFlag::HEAD_TRACK | (u8)AnimTrackFlag::LIFT_TRACK | (u8)AnimTrackFlag::BODY_TRACK;
    }
    
    void IActionRunner::UnlockTracks()
    {
      // Tracks aren't locked until the action starts so don't unlock them until then
      if(!_suppressTrackLocking && _state != ActionResult::FAILURE_NOT_STARTED)
      {
        u8 tracks = GetTracksToLock();
#         if DEBUG_ANIM_TRACK_LOCKING
        PRINT_NAMED_INFO("IActionRunner.UnlockTracks", "unlocked: (0x%x) %s by %s [%d]",
                         tracks,
                         AnimTrackFlagToString((AnimTrackFlag)tracks),
                         _name.c_str(),
                         _idTag);
#         endif
        _robot.GetMoveComponent().UnlockTracks(tracks);
      }
    }
    
#pragma mark ---- IAction ----
    
    IAction::IAction(Robot& robot)
    : IActionRunner(robot)
    {
      Reset();
    }
    
    void IAction::Reset(bool shouldUnlockTracks)
    {
      _preconditionsMet = false;
      _waitUntilTime = -1.f;
      _timeoutTime = -1.f;
      if(shouldUnlockTracks)
      {
        UnlockTracks();
      }
      ResetState();
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
        
        // Don't unlock the tracks if retrying
        Reset(false);
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
