/**
 * File: behaviorRequestGameZeroBlocks.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-01-27
 *
 * Description: re-usable behavior to request to play a game by turning to a face and playing an
 *              animation. This handles the case where there are no known blocks
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/trackingActions.h"
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorRequestGameZeroBlocks.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;

static const char* kRequestAnimNameKey = "request_animName";
static const char* kDenyAnimNameKey = "deny_animName";
static const char* kMaxFaceAgeKey = "maxFaceAge_ms";
static const char* kMinRequestDelay = "minRequestDelay_s";


BehaviorRequestGameZeroBlocks::BehaviorRequestGameZeroBlocks(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetDefaultName("RequestGameZeroBlocks");

  if( config.isNull() ) {
    PRINT_NAMED_WARNING("BehaviorRequestGameZeroBlocks.Config.Error",
                        "Empty json config! This behavior will not function correctly");
  }
  else {
    {
      const Json::Value& val = config[kRequestAnimNameKey];
      if( val.isString() ) {
        _requestAnimationName = val.asCString();
      }
      else {
        PRINT_NAMED_WARNING("BehaviorRequestGameZeroBlocks.Config.MissingKey",
                            "Missing key '%s'",
                            kRequestAnimNameKey);
      }
    }

    {
      const Json::Value& val = config[kDenyAnimNameKey];
      if( val.isString() ) {
        _denyAnimationName = val.asCString();
      }
      else {
        PRINT_NAMED_WARNING("BehaviorRequestGameZeroBlocks.Config.MissingKey",
                            "Missing key '%s'",
                            kRequestAnimNameKey);
      }
    }

    {
      const Json::Value& val = config[kMaxFaceAgeKey];
      if( val.isUInt() ) {
        _maxFaceAge_ms = val.asUInt();
      }
    }

    {
      const Json::Value& val = config[kMinRequestDelay];
      if( val.isDouble() ) {
        _minRequestDelay_s = val.asFloat();
      }
    }
  }

  SubscribeToTags({{
    EngineToGameTag::RobotCompletedAction,
    EngineToGameTag::RobotObservedFace,
    EngineToGameTag::RobotDeletedFace,
  }});

  SubscribeToTags({
    GameToEngineTag::DenyGameStart
  });
}

BehaviorRequestGameZeroBlocks::~BehaviorRequestGameZeroBlocks()
{
}
  
bool BehaviorRequestGameZeroBlocks::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  const u32 currTime_ms = Util::numeric_cast<u32>( std::floor( currentTime_sec * 0.001 ) );

  Pose3d waste;
  TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(waste);
  
  const bool hasFace = lastObservedFaceTime > 0 && lastObservedFaceTime + _maxFaceAge_ms > currTime_ms;
  const bool doingSoemthing = _isActing;

  const bool ret = hasFace || doingSoemthing;

  return ret;
}

Result BehaviorRequestGameZeroBlocks::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  if( ! _isActing ) {
    const u32 currTime_ms = Util::numeric_cast<u32>( std::floor( currentTime_sec * 0.001 ) );

    Pose3d lastFacePose;
    TimeStamp_t lastObservedFaceTime = robot.GetFaceWorld().GetLastObservedFace(lastFacePose);
    if( lastObservedFaceTime == 0 || lastObservedFaceTime + _maxFaceAge_ms <= currTime_ms ) {
      PRINT_NAMED_ERROR("BehaviorRequestGameZeroBlocks.InitInternal.NoFace",
                        "Trying to update, should have a face, but last observed face time is %dms",
                        currTime_ms);
      return RESULT_FAIL;
    }

    PRINT_NAMED_DEBUG("BehaviorRequestGameZeroBlocks.TurningTowardsFace", "");
    
    FacePoseAction* action = new FacePoseAction(lastFacePose, PI_F);
    StartActing(robot, action);

    _state = State::LookingAtFace;

    _requestTime_s = -1.0f;
  }
  
  return RESULT_OK;
}

IBehavior::Status BehaviorRequestGameZeroBlocks::UpdateInternal(Robot& robot, double currentTime_sec)
{
  if( _isActing ) {
    return Status::Running;
  }

  PRINT_NAMED_DEBUG("BehaviorRequestGameZeroBlocks.Compelte", "no current actions, so finishing");

  return Status::Complete;
}

Result BehaviorRequestGameZeroBlocks::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  // if we are playing an animation, we don't want to be interrupted
  if( _isActing ) {
    switch( _state ) {
      case State::LookingAtFace:
      case State::PlayingRequstAnim:
      case State::PlayingDenyAnim:
        return Result::RESULT_FAIL;

      case State::TrackingFace:
        if( _requestTime_s >= 0.0f && currentTime_sec < _requestTime_s + _minRequestDelay_s ) {
          return Result::RESULT_FAIL;
        }
        // handled in StopInternal
        break;
    }
  }

  // TODO:(bn) don't send deny if it's a short interrupt?
  StopInternal(robot, currentTime_sec);

  return Result::RESULT_OK;
}

void BehaviorRequestGameZeroBlocks::StopInternal(Robot& robot, double currentTime_sec)
{
  if( _state == State::TrackingFace ) {
    // this means we have send up the game request, so now we should send a Deny to cancel it
    PRINT_NAMED_INFO("BehaviorRequestGameZeroBlocks.DenyRequest",
                     "behavior is denying it's own request");
    robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::DenyGameStart() ) );

    if( _isActing ) {
      robot.GetActionList().Cancel( _lastActionTag );
      _isActing = false;
    }
  }
  
  _state = State::LookingAtFace;
}
  
void BehaviorRequestGameZeroBlocks::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{

  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::RobotCompletedAction:
      HandleActionCompleted(robot, event.GetData().Get_RobotCompletedAction());
      break;

    case EngineToGameTag::RobotObservedFace:
      HandleObservedFace(robot, event.GetData().Get_RobotObservedFace());
      break;
        
    case EngineToGameTag::RobotDeletedFace:
      HandleDeletedFace(event.GetData().Get_RobotDeletedFace());
      break;

    default:
      PRINT_NAMED_ERROR("BehaviorRequestGameZeroBlocks.InvalidTag",
                        "Received unexpected event with tag %hhu.", event.GetData().GetTag());
      break;
  }
}

void BehaviorRequestGameZeroBlocks::HandleActionCompleted(Robot& robot,
                                                          const ExternalInterface::RobotCompletedAction& msg)
{
  if( _isActing ) {
    if( msg.idTag == _lastActionTag ) {
      switch( _state ) {
        case State::LookingAtFace: {
          if( msg.result == ActionResult::SUCCESS ) {

            IActionRunner* animationAction = new PlayAnimationAction(_requestAnimationName);
            
            StartActing( robot, animationAction );
            _state = State::PlayingRequstAnim;
            PRINT_NAMED_DEBUG("BehaviorRequestGameZeroBlocks.PlayRequestAnim",
                              "playing request animation '%s'",
                              _requestAnimationName.c_str());
          }
          else {
            PRINT_NAMED_INFO("BehaviorRequestGameZeroBlocks.HandleActionComplete.ActionFailed",
                             "Look at face action failed, giving up on behavior");
            _isActing = false;
          }
          
          break;
        }

        case State::PlayingRequstAnim: {
          // even if the animation action failed / was canceled, still do the request, and advance to tracking face
          PRINT_NAMED_INFO("BehaviorRequestGameZeroBlocks.Request",
                           "behavior is sending a game start request");
          robot.Broadcast( ExternalInterface::MessageEngineToGame( ExternalInterface::RequestGameStart() ) );

          _minRequestDelay_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

          _state = State::TrackingFace;

          if( _faceID == Face::UnknownFace ) {
            PRINT_NAMED_WARNING("BehaviorRequestGameZeroBlocks.NoValidFace",
                                "Can't do face tracking because there is no valid face!");
            // use an action that just hangs to simulate the track face logic
            StartActing( robot, new HangAction() );
          }
          else {
            StartActing( robot, new TrackFaceAction(_faceID) );
          }
          break;
        }
              
        case State::TrackingFace: {
          PRINT_NAMED_INFO("BehaviorRequestGameZeroBlocks.HandleActionComplete.TrackingActionEnded",
                           "Someone ended the tracking action, behavior will terminate");
          _isActing = false;
          break;
        }

        case State::PlayingDenyAnim: {
          // end the behavior after this completes
          _isActing = false;
          break;
        }
      }
    }
  }
}

void BehaviorRequestGameZeroBlocks::HandleObservedFace(const Robot& robot,
                                           const ExternalInterface::RobotObservedFace& msg)
{
  // If faceID not already set or we're not currently tracking the update the faceID
  if (_faceID == Face::UnknownFace) {
    _faceID = static_cast<Face::ID_t>(msg.faceID);
  }
}
  
void BehaviorRequestGameZeroBlocks::HandleDeletedFace(const ExternalInterface::RobotDeletedFace& msg)
{
  if (_faceID == msg.faceID) {
    _faceID = Face::UnknownFace;
  }
}

void BehaviorRequestGameZeroBlocks::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  if( event.GetData().GetTag() == GameToEngineTag::DenyGameStart ) {
    // user said no! Cancel action if we're acting
    if( _isActing ) {
      robot.GetActionList().Cancel( _lastActionTag );
      _isActing = false;
    }

    StartActing( robot, new PlayAnimationAction( _denyAnimationName ) );
    _state = State::PlayingDenyAnim;
  }
}


void BehaviorRequestGameZeroBlocks::StartActing(Robot& robot, IActionRunner* action)
{
  _lastActionTag = action->GetTag();
  robot.GetActionList().QueueActionNow(action);
  _isActing = true;
}

}
}
