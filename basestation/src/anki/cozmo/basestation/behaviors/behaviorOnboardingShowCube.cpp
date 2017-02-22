/**
 * File: behaviorOnboardingShowCubes.cpp
 *
 * Author: Molly Jameson
 * Created: 2016-09-01
 *
 * Description: SuperSpecificOnboarding behavior since it requires a lot of cube information, needs to be in engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/actions/retryWrapperAction.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/behaviorOnboardingShowCube.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/utils/timer.h"

#include "clad/externalInterface/messageGameToEngine.h"

#include "util/logging/logging.h"
#include "util/math/math.h"

#define SET_STATE(s,robot) SetState_internal(State::s, #s,robot)

namespace Anki {
namespace Cozmo {
using namespace ExternalInterface;
  
static const char* kMaxErrorsTotalKey  = "MaxErrorsTotal";
static const char* kMaxErrorsPickupKey = "MaxErrorsPickup";
static const char* kMaxTimeBeforeTimeoutSecKey = "Timeout_Sec";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingShowCube::BehaviorOnboardingShowCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  SetDefaultName("OnboardingShowCubes");
  
  SubscribeToTags({
    MessageGameToEngineTag::TransitionToNextOnboardingState
  });
  SubscribeToTags({{
    EngineToGameTag::ReactionaryBehaviorTransition,
    EngineToGameTag::RobotObservedObject,
  }});
  
  JsonTools::GetValueOptional(config, kMaxErrorsTotalKey, _maxErrorsTotal);
  JsonTools::GetValueOptional(config, kMaxErrorsPickupKey, _maxErrorsPickup);
  JsonTools::GetValueOptional(config, kMaxTimeBeforeTimeoutSecKey, _maxTimeBeforeTimeout_Sec);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingShowCube::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  // behavior will be killed by unity, the only thing that can start it...
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorOnboardingShowCube::InitInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::OnboardingDriveStart,
    AnimationTrigger::OnboardingDriveLoop,
    AnimationTrigger::OnboardingDriveEnd});
  EnableSpecificReactionaryBehavior(robot,false);
  // Some reactionary behaviors don't trigger resume like cliff react followed by "react to on back"
  // So just handle init doesn't always reset to "inactive"
  PRINT_CH_INFO("Behaviors","BehaviorOnboardingShowCube::InitInternal", " %hhu ",_state);
  if( _state != State::ErrorCozmo && _state != State::ErrorFinal )
  {
    _numErrors = 0;
    _timesPickedUpCube = 0;
    SET_STATE(WaitForShowCube,robot);
  }
  
  return Result::RESULT_OK;
}
  
void BehaviorOnboardingShowCube::StopInternal(Robot& robot)
{
  robot.GetDrivingAnimationHandler().PopDrivingAnimations();
  EnableSpecificReactionaryBehavior(robot, true);
  robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Onboarding);
  PRINT_CH_INFO("Behaviors","BehaviorOnboardingShowCube::StopInternal", " %hhu ",_state);
}

void BehaviorOnboardingShowCube::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  if( event.GetData().GetTag() == MessageEngineToGameTag::ReactionaryBehaviorTransition )
  {
    // Only react when the behavior is running ( not inactive )
    const ExternalInterface::ReactionaryBehaviorTransition& msg = event.GetData().Get_ReactionaryBehaviorTransition();
    if( msg.behaviorStarted &&  _state != State::ErrorCozmo && _state != State::Inactive && _state != State::ErrorFinal)
    {
      switch (msg.reactionaryBehaviorTrigger)
      {
        case ReactionTrigger::CliffDetected:
        case ReactionTrigger::RobotPickedUp:
        case ReactionTrigger::RobotOnSide:
        case ReactionTrigger::RobotOnBack:
        case ReactionTrigger::RobotOnFace:
        case ReactionTrigger::UnexpectedMovement:
        {
          TransitionToErrorState(State::ErrorCozmo,robot);
          break;
        }
        default:
          break;
      }
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingShowCube::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageEngineToGameTag::RobotObservedObject:
    {
      HandleObjectObserved(robot, event.GetData().Get_RobotObservedObject());
      break;
    }
    case MessageEngineToGameTag::ReactionaryBehaviorTransition:
      // Handled by AlwaysHandle, but don't want error printing...
    break;
    default: {
      PRINT_NAMED_ERROR("BehaviorOnboardingShowCube.HandleWhileRunning.InvalidEvent", "");
      break;
    }
  }

}
  
void BehaviorOnboardingShowCube::HandleWhileRunning(const GameToEngineEvent& event, Robot& robot)
{
  switch (event.GetData().GetTag())
  {
    case MessageGameToEngineTag::TransitionToNextOnboardingState: {
      // No parameters: Context sensitive...
      TransitionToNextState(robot);
      break;
    }
      
    default: {
      PRINT_NAMED_ERROR("BehaviorOnboardingShowCube.HandleWhileRunning.InvalidEvent", "");
      break;
    }
  }
}

// This behavior is killed by unity switching to none
IBehavior::Status BehaviorOnboardingShowCube::UpdateInternal(Robot& robot)
{
  if( !IsActing() && !IsSequenceComplete() )
  {
    float timeRunning = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() - GetTimeStartedRunning_s();
    if( timeRunning > _maxTimeBeforeTimeout_Sec )
    {
      SET_STATE(ErrorFinal,robot);
    }
  }
  return Status::Running;
}
  
void BehaviorOnboardingShowCube::SetState_internal(State state, const std::string& stateName,const Robot& robot)
{
  _state = state;
  PRINT_CH_INFO("Behaviors","BehaviorOnboardingShowCube.TransitionTo", "%s", stateName.c_str());
  SetDebugStateName(stateName);
  // because this is called from some places where Robot is const, uses const safe version rather than just robot.Broadcast
  robot.GetContext()->GetExternalInterface()->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::OnboardingState(_state)));
}
  
void BehaviorOnboardingShowCube::TransitionToErrorState(State state, const Robot& robot)
{
  // Something might be wrong with the robot and they're stuck. By design let them continue.
  if( _numErrors < _maxErrorsTotal )
  {
    // Debug macros :(
    if( state == State::ErrorCozmo)
    {
      SET_STATE(ErrorCozmo,robot);
    }
    else if( state == State::ErrorCubeMoved)
    {
      SET_STATE(ErrorCubeMoved,robot);
    }
  }
  else
  {
    SET_STATE(ErrorFinal,robot);
  }
  ++_numErrors;
}
  
void BehaviorOnboardingShowCube::TransitionToNextState(Robot& robot)
{
  switch (_state)
  {
    case State::WaitForOKCubeDiscovered:
    {
      TimeStamp_t startWaitTime = robot.GetLastImageTimeStamp();
      const int kNumFramesWaitForImages = 3;
      StartActing(new WaitForImagesAction(robot, kNumFramesWaitForImages, VisionMode::DetectingMarkers),
                  [this, &robot, startWaitTime](ActionResult result) {
                    bool blockError = true;
                    bool lightsError = false;
                    if( _targetBlock.IsSet() )
                    {
                      ObservableObject* block = robot.GetBlockWorld().GetLocatedObjectByID(_targetBlock);
                      // Block is still visible if we saw it last processed frame.
                      if( block != nullptr && startWaitTime < block->GetLastObservedTime() )
                      {
                        blockError = false;
                        // The Light access message is sometimes wrong, so trust the vision system since we only get here if we've seen it.
                        if( block->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != AxisName::Z_POS)
                        {
                          lightsError = true;
                        }
                      }
                    }
                    if( blockError )
                    {
                      TransitionToErrorState(State::ErrorCubeMoved,robot);
                    }
                    else if( lightsError )
                    {
                      // Lights don't count towards cummulative max errors because they don't indicate a robot failure.
                      // so just set the state to change the UI.
                      SET_STATE(ErrorCubeWrongSideUp,robot);
                    }
                    else
                    {
                      TransitionToWaitToInspectCube(robot);
                    }
                  } );
    }
    break;
    case State::WaitForFinalContinue:
    case State::ErrorFinal:
      SET_STATE(Inactive,robot);
    break;
    case State::ErrorCozmo:
      if( _timesPickedUpCube > 0 )
      {
        SET_STATE(WaitForFinalContinue,robot);
      }
      else
      {
        SET_STATE(WaitForShowCube,robot);
      }
      break;
    case State::ErrorCubeMoved:
    case State::ErrorCubeWrongSideUp:
      TransitionToWaitToInspectCube(robot);
    break;
    default:
      break;
  }
}
  
void BehaviorOnboardingShowCube::TransitionToWaitToInspectCube(Robot& robot)
{
  SET_STATE(WaitForInspectCube,robot);
  // AnimationTrigger.OnboardingReactToCube
  // DriveToAndPickupBlock
  //  on error: AnimationTrigger.RollBlockRealign, retry
  // on success: AnimationTrigger.OnboardingInteractWithCube
  // Move lift up so can safely put down, since place object on ground does unpredictable things in the middle
  // PlaceObjectOnGroundHere
  // AnimationTrigger.OnboardingReactToCubePutDown
  StartActing(new TriggerAnimationAction(robot,AnimationTrigger::OnboardingReactToCube),
              [this, &robot](const ActionResult& result){
                if(result == ActionResult::SUCCESS)
                {
                  StartSubStatePickUpBlock(robot);
                }
              });
}
  
void BehaviorOnboardingShowCube::StartSubStatePickUpBlock(Robot& robot)
{
  // Manually set lights to Interacting (green) lights
  robot.GetCubeLightComponent().PlayLightAnim(_targetBlock, CubeAnimationTrigger::Onboarding);
  
  DriveToPickupObjectAction* driveAndPickupAction = new DriveToPickupObjectAction(robot, _targetBlock);
  driveAndPickupAction->SetPostDockLiftMovingAnimation(AnimationTrigger::OnboardingSoundOnlyLiftEffortPickup);
  RetryWrapperAction::RetryCallback retryCallback = [this, driveAndPickupAction](const ExternalInterface::RobotCompletedAction& completion, const u8 retryCount, AnimationTrigger& animTrigger)
  {
    animTrigger = AnimationTrigger::Count;
    
    const ActionResultCategory resCat = IActionRunner::GetActionResultCategory(completion.result);
    if(resCat == ActionResultCategory::ABORT ||
       resCat == ActionResultCategory::RETRY)
    {
      animTrigger = AnimationTrigger::OnboardingCubeDockFail;
      return true;
    }
    return false;
  };
  
  RetryWrapperAction* retryWrapperAction = new RetryWrapperAction(robot, driveAndPickupAction, retryCallback, _maxErrorsPickup);
  StartActing(retryWrapperAction,
              [this,&robot](const ExternalInterface::RobotCompletedAction& msg)
              {
                if(msg.result == ActionResult::SUCCESS)
                {
                  _timesPickedUpCube++;
                  StartSubStateCelebratePickup(robot);
                }
                else
                {
                  // Play failure animation
                  if (msg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_MOVING || msg.result == ActionResult::PICKUP_OBJECT_UNEXPECTEDLY_NOT_MOVING)
                  {
                    StartActing(new TriggerAnimationAction(robot, AnimationTrigger::OnboardingCubeDockFail));
                  }
                  // During an interrupt behavior is cancelled only for a max_num_retries do we assume that the
                  // pickup is just not working for some other reason than them messing with the robot.
                  if( msg.result == ActionResult::REACHED_MAX_NUM_RETRIES )
                  {
                    SET_STATE(ErrorFinal,robot);
                  }
                }
              });
}

void BehaviorOnboardingShowCube::StartSubStateCelebratePickup(Robot& robot)
{
  CompoundActionSequential* action = new CompoundActionSequential(robot,
  {
    new TriggerAnimationAction(robot, AnimationTrigger::OnboardingInteractWithCube),
    new MoveLiftToHeightAction(robot,MoveLiftToHeightAction::Preset::CARRY),
  });
  
  // because PutDown isn't a real docking action, it doesn't have any sounds associated with it.
  // So just play a sound animation at the same time.
  CompoundActionParallel* parallelAction = new CompoundActionParallel(robot,{
    new TriggerAnimationAction(robot, AnimationTrigger::OnboardingSoundOnlyLiftEffortPlaceLow),
    new PlaceObjectOnGroundAction(robot)
  });
  action->AddAction(parallelAction);
  
  action->AddAction(new TriggerAnimationAction(robot, AnimationTrigger::OnboardingReactToCubePutDown));
  
  StartActing(action,
              [this,&robot](const ExternalInterface::RobotCompletedAction& msg)
              {
                robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::Onboarding);
                SET_STATE(WaitForFinalContinue,robot);
              });
}

bool IsBlockFacingUp(const ObservableObject* block)
{
  if( block != nullptr )
  {
    return block->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() == AxisName::Z_POS;
  }
  return false;
}

void BehaviorOnboardingShowCube::HandleObjectObserved(Robot& robot, const ExternalInterface::RobotObservedObject& msg)
{
  const ObservableObject* block = robot.GetBlockWorld().GetLocatedObjectByID(msg.objectID);

  if(nullptr == block)
  {
    PRINT_NAMED_WARNING("BehaviorOnboardingShowCube.HandleObjectObserved.NullObject",
                        "ObjectID=%d", msg.objectID);
    return;
  }
  
  // In this state it's okay to change blocks. After this Cozmo will be trying to drive too it so it's too late.
  // after generic fails we loop back to this state so they can try again.
  if( ( _state == State::WaitForShowCube || _state == State::WaitForOKCubeDiscovered ||
       _state == State::ErrorCubeWrongSideUp || _state == State::ErrorCubeMoved) &&
        robot.CanPickUpObjectFromGround(*block) )
  {
    _targetBlock = msg.objectID;
    if( _state == State::WaitForShowCube)
    {
      SET_STATE(WaitForOKCubeDiscovered,robot);
    }
    else if( _state == State::ErrorCubeMoved )
    {
      if(IsBlockFacingUp(block))
      {
        TransitionToNextState(robot);
      }
      else
      {
        SET_STATE(ErrorCubeWrongSideUp,robot);
      }
    }
    else if( _state == State::ErrorCubeWrongSideUp)
    {
      if(IsBlockFacingUp(block))
      {
        TransitionToNextState(robot);
      }
    }
  }
}

// If we're just waiting for them to hit a final continue, don't need to pop any errors.
bool BehaviorOnboardingShowCube::IsSequenceComplete()
{
  return _state == State::Inactive || _state == State::ErrorFinal || _state == State::WaitForFinalContinue;
}
  
void BehaviorOnboardingShowCube::EnableSpecificReactionaryBehavior(Robot& robot, bool enable)
{
  BehaviorManager& mgr = robot.GetBehaviorManager();
  mgr.RequestEnableReactionTrigger("onboarding", ReactionTrigger::FacePositionUpdated, enable);
  mgr.RequestEnableReactionTrigger("onboarding", ReactionTrigger::ObjectPositionUpdated, enable);
  mgr.RequestEnableReactionTrigger("onboarding", ReactionTrigger::CubeMoved, enable);
  mgr.RequestEnableReactionTrigger("onboarding", ReactionTrigger::Frustration, enable);
  mgr.RequestEnableReactionTrigger("onboarding", ReactionTrigger::PetInitialDetection, enable);
}

  
}
}

