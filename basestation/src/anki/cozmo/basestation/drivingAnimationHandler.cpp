/**
 * File: drivingAnimationHandler.cpp
 *
 * Author: Al Chaussee
 * Date:   5/6/2016
 *
 * Description: Handles playing animations while driving
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "drivingAnimationHandler.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
  namespace Cozmo {

    // Which docking method actions should use
    CONSOLE_VAR(bool, kEnableDrivingAnimations, "DrivingAnimationHandler", true);
    
    
    DrivingAnimationHandler::DrivingAnimationHandler(Robot& robot)
    : _robot(robot)
    {
      if(_robot.HasExternalInterface())
      {
        _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
        [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
        {
          ASSERT_NAMED(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotCompletedAction,
                       "Wrong event type from callback");
          HandleActionCompleted(event.GetData().Get_RobotCompletedAction());
        } ));
        
        _signalHandles.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetDrivingAnimations,
        [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
        {
          auto const& payload = event.GetData().Get_SetDrivingAnimations();
          SetDrivingAnimations(payload.drivingStartAnim, payload.drivingLoopAnim, payload.drivingEndAnim);
        }));
      }
    }
    
    void DrivingAnimationHandler::SetDrivingAnimations(const std::string& drivingStartAnim,
                                                       const std::string& drivingLoopAnim,
                                                       const std::string& drivingEndAnim)
    {
      _drivingStartAnim = drivingStartAnim;
      _drivingLoopAnim = drivingLoopAnim;
      _drivingEndAnim = drivingEndAnim;
    }

    void DrivingAnimationHandler::ResetDrivingAnimations()
    {
      SetDrivingAnimations(kDefaultDrivingStartAnim, kDefaultDrivingLoopAnim, kDefaultDrivingEndAnim);
    }
    
    void DrivingAnimationHandler::HandleActionCompleted(const ExternalInterface::RobotCompletedAction& msg)
    {
      // Only start playing drivingLoop if start successfully completes
      if(msg.idTag == _drivingStartAnimTag && msg.result == ActionResult::SUCCESS)
      {
        if(!_drivingLoopAnim.empty())
        {
          PlayDrivingLoopAnim();
        }
      }
      else if(msg.idTag == _drivingLoopAnimTag)
      {
        if(_robot.IsTraversingPath() && msg.result == ActionResult::SUCCESS)
        {
          PlayDrivingLoopAnim();
        }
        else
        {
          // Unlock our tracks so that endAnim can use them
          // This should be safe since we have finished driving
          _robot.GetMoveComponent().UnlockTracks(_tracksToUnlock);
          
          PlayDrivingEndAnim();
          _endAnimStarted = true;
        }
      }
      else if(msg.idTag == _drivingEndAnimTag)
      {
        _endAnimStarted = false;
        _endAnimCompleted = true;
        _startedPlayingAnimation = false;
        
        // Relock tracks like nothing ever happend
        _robot.GetMoveComponent().LockTracks(_tracksToUnlock);
      }
    }
    
    void DrivingAnimationHandler::ActionIsBeingDestroyed()
    {
      _startedPlayingAnimation = false;
      
      _robot.GetActionList().Cancel(_drivingStartAnimTag);
      _robot.GetActionList().Cancel(_drivingLoopAnimTag);
      _robot.GetActionList().Cancel(_drivingEndAnimTag);
    }
    
    void DrivingAnimationHandler::PlayStartAnim(u8 tracksToUnlock)
    {
      if (!kEnableDrivingAnimations) {
        return;
      }
      
      // If we have already been inited do nothing
      if(_startedPlayingAnimation)
      {
        return;
      }
      
      _endAnimCompleted = false;
      _endAnimStarted = false;
      _drivingStartAnimTag = -1;
      _drivingLoopAnimTag = -1;
      _drivingEndAnimTag = -1;
      _tracksToUnlock = tracksToUnlock;
      
      if(!_drivingStartAnim.empty())
      {
        PlayDrivingStartAnim();
        _startedPlayingAnimation = true;
      }
      else if(!_drivingLoopAnim.empty())
      {
        PlayDrivingLoopAnim();
        _startedPlayingAnimation = true;
      }
    }
    
    bool DrivingAnimationHandler::PlayEndAnim()
    {
      if (!kEnableDrivingAnimations) {
        return false;
      }
      
      _robot.GetActionList().Cancel(_drivingStartAnimTag);
      _robot.GetActionList().Cancel(_drivingLoopAnimTag);
      
      if(!_drivingEndAnim.empty() && !_endAnimStarted && !_endAnimCompleted)
      {
        // Unlock our tracks so that endAnim can use them
        // This should be safe since we have finished driving
        _robot.GetMoveComponent().UnlockTracks(_tracksToUnlock);
        
        PlayDrivingEndAnim();
        _endAnimStarted = true;
      }
      return _endAnimStarted;
    }
  
    void DrivingAnimationHandler::PlayDrivingStartAnim()
    {
      IActionRunner* animAction = new PlayAnimationGroupAction(_robot, _drivingStartAnim, 1, true);
      _drivingStartAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingLoopAnim()
    {
      IActionRunner* animAction = new PlayAnimationGroupAction(_robot, _drivingLoopAnim, 1, true);
      _drivingLoopAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
    
    void DrivingAnimationHandler::PlayDrivingEndAnim()
    {
      IActionRunner* animAction = new PlayAnimationGroupAction(_robot, _drivingEndAnim, 1, true);
      _drivingEndAnimTag = animAction->GetTag();
      _robot.GetActionList().QueueAction(QueueActionPosition::IN_PARALLEL, animAction);
    }
  }
}
