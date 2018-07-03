/**
 * File: drivingAnimationHandler.cpp
 *
 * Author: Al Chaussee
 * Date:   5/6/2016
 *
 * Description: Handles playing animations while driving
 *              Whatever tracks are locked by the action will stay locked while the start and loop
 *              animations but the tracks will be unlocked while the end animation plays
 *              The end animation will always play and will cancel the start/loop animations if needed
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "engine/drivingAnimationHandler.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/actions/animActions.h"
#include "engine/components/movementComponent.h"
#include "engine/components/pathComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"

#include "util/console/consoleInterface.h"
#include "util/helpers/boundedWhile.h"

namespace Anki {
  namespace Cozmo {

    // Which docking method actions should use
    CONSOLE_VAR(bool, kEnableDrivingAnimations, "DrivingAnimationHandler", true);
    
    DrivingAnimationHandler::DrivingAnimationHandler()
    : IDependencyManagedComponent(this, RobotComponentID::DrivingAnimationHandler)
    , _moodBasedDrivingAnims( { { SimpleMoodType::Default,    { AnimationTrigger::DriveStartDefault,
                                                                AnimationTrigger::DriveLoopDefault,
                                                                AnimationTrigger::DriveEndDefault}},
                                { SimpleMoodType::HighStim,   { AnimationTrigger::DriveStartHappy,
                                                                AnimationTrigger::DriveLoopHappy,
                                                                AnimationTrigger::DriveEndHappy}},
                                { SimpleMoodType::Frustrated, { AnimationTrigger::DriveStartAngry,
                                                                AnimationTrigger::DriveLoopAngry,
                                                                AnimationTrigger::DriveEndAngry}} })
    , _tracksToUnlock( (u8)AnimTrackFlag::NO_TRACKS )
    , _drivingStartAnimTag( ActionConstants::INVALID_TAG )
    , _drivingLoopAnimTag(  ActionConstants::INVALID_TAG )
    , _drivingEndAnimTag(   ActionConstants::INVALID_TAG )
    {
      _currDrivingAnimations = _moodBasedDrivingAnims.at(SimpleMoodType::Default);
    }

    void DrivingAnimationHandler::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps) 
    {
      _robot = robot;
      if(_robot->HasExternalInterface()){
        _signalHandles.push_back(_robot->GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
        [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
        {
          DEV_ASSERT(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
                     "Wrong event type from callback");
          HandleActionCompleted(event.GetData().Get_RobotCompletedAction());
        } ));
        
        _signalHandles.push_back(_robot->GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageGameToEngineTag::PushDrivingAnimations,
        [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
        {
          auto const& payload = event.GetData().Get_PushDrivingAnimations();
          PushDrivingAnimations({payload.drivingStartAnim, payload.drivingLoopAnim, payload.drivingEndAnim},
                                payload.lockName);
        }));
        
        _signalHandles.push_back(_robot->GetExternalInterface()->Subscribe(
                                   ExternalInterface::MessageGameToEngineTag::RemoveDrivingAnimations,
        [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
        {
          RemoveDrivingAnimations(event.GetData().Get_RemoveDrivingAnimations().lockName);
        }));
      }
    }
    
    void DrivingAnimationHandler::PushDrivingAnimations(const DrivingAnimations& drivingAnimations, const std::string& lockName)
    {
      if(_state != AnimState::ActionDestroyed)
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.PushDrivingAnimations",
                            "Pushing new animations while currently playing");
      }
      _drivingAnimationStack.push_back(std::make_pair(drivingAnimations, lockName));
    }
    
    void DrivingAnimationHandler::RemoveDrivingAnimations(const std::string& lockName)
    {
      if(_state != AnimState::ActionDestroyed)
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.RemoveDrivingAnimations",
                            "Popping animations while currently playing");
      }
    
      if (_drivingAnimationStack.empty())
      {
        PRINT_NAMED_WARNING("DrivingAnimationHandler.RemoveDrivingAnimations",
                            "Tried to pop animations but the stack is empty!");
      }
      else
      {
        // find the driving animation with the matching lock name in the driving animation stack (top down)
        auto drivingAnimReverseIter = std::find_if(_drivingAnimationStack.rbegin(),
                                                   _drivingAnimationStack.rend(),
                                                   [&lockName](const std::pair<DrivingAnimations, std::string>& stackEntry) {
                                                     return (stackEntry.second == lockName);
                                                   });
        
        if (drivingAnimReverseIter != _drivingAnimationStack.rend()) {
          // convert back to forward iterator and erase the element
          auto forwardIt = std::next(drivingAnimReverseIter).base();
          _drivingAnimationStack.erase(forwardIt);
        } else {
          PRINT_NAMED_WARNING("DrivingAnimationHandler.RemoveDrivingAnimations.NotFound",
                              "Could not find driving animation with name '%s'",
                              lockName.c_str());
        }
      }
    }

    void DrivingAnimationHandler::UpdateCurrDrivingAnimations()
    {
      if( _drivingAnimationStack.empty() ) {
        // use mood and needs to determine which anims to play
        auto it = _moodBasedDrivingAnims.find(_robot->GetMoodManager().GetSimpleMood());
        if( it == _moodBasedDrivingAnims.end() ) {
          // fall back to default
          it = _moodBasedDrivingAnims.find( SimpleMoodType::Default );
        }

        if( ANKI_VERIFY(it != _moodBasedDrivingAnims.end(),
                        "DrivingAnimationHandler.UpdateCurrDrivingAnimations.MoodBased.Missing",
                        "Missing driving animation! Must specify a default") ) {
          _currDrivingAnimations = it->second;
        }
      }
      else {
        _currDrivingAnimations = _drivingAnimationStack.back().first;
      }
    }

    void DrivingAnimationHandler::HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg)
    {
      const auto& pathComponent = _robot->GetPathComponent();
      // Only start playing drivingLoop if start successfully completes
      if(msg.idTag == _drivingStartAnimTag && msg.result == ActionResult::SUCCESS)
      {
        if(_currDrivingAnimations.drivingLoopAnim != AnimationTrigger::Count)
        {
          PlayDrivingLoopAnim();
        }
      }
      else if(msg.idTag == _drivingLoopAnimTag)
      {
        const bool stillDrivingPath = pathComponent.HasPathToFollow() && !pathComponent.HasStoppedBeforeExecuting();
        const bool keepLooping = (_keepLoopingWithoutPath || stillDrivingPath);
        if(keepLooping && msg.result == ActionResult::SUCCESS)
        {
          PlayDrivingLoopAnim();
        }
        else
        {
          // Unlock our tracks so that endAnim can use them
          // This should be safe since we have finished driving
          if(_isActionLockingTracks)
          {
            _robot->GetMoveComponent().UnlockTracks(_tracksToUnlock, _actionTag);
          }
          
          EndDrivingAnim();
        }
      }
      else if(msg.idTag == _drivingEndAnimTag)
      {
        _state = AnimState::FinishedDriving;
        
        // Relock tracks like nothing ever happend
        if(_isActionLockingTracks)
        {
          _robot->GetMoveComponent().LockTracks(_tracksToUnlock, _actionTag, "DrivingAnimations");
        }
      }
    }
    
    void DrivingAnimationHandler::ActionIsBeingDestroyed()
    {
      _state = AnimState::ActionDestroyed;
      
      _robot->GetActionList().Cancel(_drivingStartAnimTag);
      _robot->GetActionList().Cancel(_drivingLoopAnimTag);
      _robot->GetActionList().Cancel(_drivingEndAnimTag);
    }
    
    void DrivingAnimationHandler::Init(const u8 tracksToUnlock,
                                       const u32 tag,
                                       const bool isActionSuppressingLockingTracks,
                                       const bool keepLoopingWithoutPath)
    {
      UpdateCurrDrivingAnimations();
      
      _state = AnimState::Waiting;
      _drivingStartAnimTag = ActionConstants::INVALID_TAG;
      _drivingLoopAnimTag = ActionConstants::INVALID_TAG;
      _drivingEndAnimTag = ActionConstants::INVALID_TAG;
      _tracksToUnlock = tracksToUnlock;
      _actionTag = tag;
      _isActionLockingTracks = !isActionSuppressingLockingTracks;
      _keepLoopingWithoutPath = keepLoopingWithoutPath;
    }
    
    void DrivingAnimationHandler::StartDrivingAnim()
    {
      if (!kEnableDrivingAnimations) {
        return;
      }
      
      // Don't do anything until Init is called, or it finished the last driving animation
      if(_state != AnimState::Waiting && _state != AnimState::FinishedDriving)
      {
        return;
      }

      if(_currDrivingAnimations.drivingStartAnim != AnimationTrigger::Count)
      {
        PlayDrivingStartAnim();
      }
      else if(_currDrivingAnimations.drivingLoopAnim != AnimationTrigger::Count)
      {
        PlayDrivingLoopAnim();
      }
    }
    
    bool DrivingAnimationHandler::EndDrivingAnim()
    {
      if (!kEnableDrivingAnimations) {
        return false;
      }
      
      // The end anim can interrupt the start and loop animations
      // If we are currently playing the end anim or have already completed it don't play it again
      if(_state == AnimState::DrivingEnd ||
         _state == AnimState::FinishedDriving ||
         _state == AnimState::ActionDestroyed)
      {
        return false;
      }
      
      _robot->GetActionList().Cancel(_drivingStartAnimTag);
      _robot->GetActionList().Cancel(_drivingLoopAnimTag);
      
      if(_currDrivingAnimations.drivingEndAnim != AnimationTrigger::Count)
      {
        // Unlock our tracks so that endAnim can use them
        // This should be safe since we have finished driving
        if(_isActionLockingTracks)
        {
          _robot->GetMoveComponent().UnlockTracks(_tracksToUnlock, _actionTag);
        }
        
        PlayDrivingEndAnim();
        return true;
      }
      return false;
    }
  
    void DrivingAnimationHandler::PlayDrivingStartAnim()
    {
      _state = AnimState::DrivingStart;
      IActionRunner* animAction = new TriggerLiftSafeAnimationAction(_currDrivingAnimations.drivingStartAnim,
                                                                     1, true);
      _drivingStartAnimTag = animAction->GetTag();
      _robot->GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingLoopAnim()
    {
      _state = AnimState::DrivingLoop;
      IActionRunner* animAction = new TriggerLiftSafeAnimationAction(_currDrivingAnimations.drivingLoopAnim,
                                                                     1, true);
      _drivingLoopAnimTag = animAction->GetTag();
      _robot->GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingEndAnim()
    {
      if(_state == AnimState::DrivingEnd ||
         _state == AnimState::FinishedDriving ||
         _state == AnimState::ActionDestroyed)
      {
        return;
      }
      
      _state = AnimState::DrivingEnd;
      IActionRunner* animAction = new TriggerLiftSafeAnimationAction(_currDrivingAnimations.drivingEndAnim,
                                                                     1, true);
      _drivingEndAnimTag = animAction->GetTag();
      _robot->GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
  }
}
