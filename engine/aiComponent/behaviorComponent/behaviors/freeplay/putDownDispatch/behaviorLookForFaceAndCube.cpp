/**
 * File: behaviorLookForFaceAndCube
 *
 * Author: Raul
 * Created: 11/01/2016
 *
 * Description: Look for faces and cubes from the current position.
 *
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/putDownDispatch/behaviorLookForFaceAndCube.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

static constexpr const int kNumFramesToWaitForTrackingOnlyFace = 1;
static constexpr const int kNumFramesToWaitForUnNamedFace = 3;
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForFaceAndCube::BehaviorLookForFaceAndCube(const Json::Value& config)
: ICozmoBehavior(config)
, _configParams{}
, _currentSidePicksDone(0)
, _currentState(State::Done)
{  
  // parse all parameters now
  LoadConfig(config["params"]);

  if( _configParams.verifySeenFaces || _configParams.stopBehaviorOnAnyFace || _configParams.stopBehaviorOnNamedFace  ) {
    // if we need to do something with seen faces, then subscribe to the message
    SubscribeToTags({
      EngineToGameTag::RobotObservedFace,
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLookForFaceAndCube::~BehaviorLookForFaceAndCube()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLookForFaceAndCube::WantsToBeActivatedBehavior() const
{
  // can run as long as it's not carrying a cube (potentially it could, but may look weird), which is handled in base
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::LoadConfig(const Json::Value& config)
{
  using namespace JsonTools;
  const std::string& debugName = GetIDStr() + ".BehaviorLookForFaceAndCube.LoadConfig";

  // shared
  _configParams.bodyTurnSpeed_radPerSec = DEG_TO_RAD( ParseFloat(config, "bodyTurnSpeed_degPerSec", debugName) );
  _configParams.headTurnSpeed_radPerSec = DEG_TO_RAD( ParseFloat(config, "headTurnSpeed_degPerSec", debugName) );
  
  // face states
  _configParams.face_headAngleAbsRangeMin_rad = DEG_TO_RAD( ParseFloat(config, "face_headAngleAbsRangeMin_deg", debugName) );
  _configParams.face_headAngleAbsRangeMax_rad = DEG_TO_RAD( ParseFloat(config, "face_headAngleAbsRangeMax_deg", debugName) );
  _configParams.face_bodyAngleRelRangeMin_rad = DEG_TO_RAD( ParseFloat(config, "face_bodyAngleRelRangeMin_deg", debugName) );
  _configParams.face_bodyAngleRelRangeMax_rad = DEG_TO_RAD( ParseFloat(config, "face_bodyAngleRelRangeMax_deg", debugName) );
  _configParams.face_sidePicks = ParseUint8(config, "face_sidePicks", debugName);
  // cube states
  _configParams.cube_headAngleAbsRangeMin_rad = DEG_TO_RAD( ParseFloat(config, "cube_headAngleAbsRangeMin_rad", debugName) );;
  _configParams.cube_headAngleAbsRangeMax_rad = DEG_TO_RAD( ParseFloat(config, "cube_headAngleAbsRangeMax_rad", debugName) );;
  _configParams.cube_bodyAngleRelRangeMin_rad = DEG_TO_RAD( ParseFloat(config, "cube_bodyAngleRelRangeMin_rad", debugName) );;
  _configParams.cube_bodyAngleRelRangeMax_rad = DEG_TO_RAD( ParseFloat(config, "cube_bodyAngleRelRangeMax_rad", debugName) );;
  _configParams.cube_sidePicks = ParseUint8(config, "cube_sidePicks", debugName);
  
  _configParams.verifySeenFaces = ParseBool(config, "verifySeenFaces", debugName);
  _configParams.stopBehaviorOnAnyFace = ParseBool(config, "stopBehaviorOnAnyFace", debugName);
  _configParams.stopBehaviorOnNamedFace = ParseBool(config, "stopBehaviorOnNamedFace", debugName);
  
  // anim triggers
  std::string lookInPlaceAnimTriggerStr = ParseString(config, "lookInPlaceAnimTrigger", debugName);
  _configParams.lookInPlaceAnimTrigger = lookInPlaceAnimTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(lookInPlaceAnimTriggerStr.c_str());
  if ( _configParams.lookInPlaceAnimTrigger == AnimationTrigger::Count )
  {
    PRINT_NAMED_ERROR("BehaviorLookForFaceAndCube.LoadConfig.Invalid.lookInPlaceAnimTrigger",
      "[%s] Invalid animation trigger '%s'",
      GetIDStr().c_str(), lookInPlaceAnimTriggerStr.c_str());
  }
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::OnBehaviorActivated()
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".InitInternal").c_str(), "Starting to look for face at center");

  auto& robotInfo = GetBEI().GetRobotInfo();
  _startingBodyFacing_rad = robotInfo.GetPose().GetWithRespectToRoot().GetRotationAngle<'Z'>();
  _currentSidePicksDone = 0;
  _currentState = State::S0FaceOnCenter;
  _verifiedFaces.clear();
  _isVerifyingFace = false;
  
  CompoundActionParallel* initialActions = new CompoundActionParallel();

  // lift down
  {
    IAction* liftDownAction = new MoveLiftToHeightAction(MoveLiftToHeightAction::Preset::LOW_DOCK);
    initialActions->AddAction( liftDownAction );
  }
  
  // create head move action
  {
    IAction* centerLookForFace = CreateBodyAndHeadTurnAction(
      0, // no min body change
      0, // no max body change
      _startingBodyFacing_rad,
      _configParams.face_headAngleAbsRangeMin_rad,
      _configParams.face_headAngleAbsRangeMax_rad,
      _configParams.bodyTurnSpeed_radPerSec,
      _configParams.bodyTurnSpeed_radPerSec);
    initialActions->AddAction( centerLookForFace );
  }

  // look here
  DelegateIfInControl( initialActions, &BehaviorLookForFaceAndCube::TransitionToS1_FaceOnLeft );
  
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**void BehaviorLookForFaceAndCube::ResumeInternal()
{
  // reset side picks done because we always switch to next state
  _currentSidePicksDone = 0;
  _isVerifyingFace = false;

  if( _currentState == State::Done ) {
    PRINT_NAMED_ERROR("BehaviorLookForFaceAndCube.ResumeInternal.AlreadyDone", "Behavior was done but it's trying to resume.");
    return RESULT_FAIL;
  }
  
  ResumeCurrentState();

  return RESULT_OK;
}**/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::ResumeCurrentState()
{
  switch(_currentState)
  {
    case State::S0FaceOnCenter:
    {
      TransitionToS1_FaceOnLeft();
      break;
    }
    case State::S1FaceOnLeft:
    {
      TransitionToS2_FaceOnRight();
      break;
    }
    case State::S2FaceOnRight:
    {
      TransitionToS3_CubeOnRight();
      break;
    }
    case State::S3CubeOnRight:
    {
      TransitionToS4_CubeOnLeft();
      break;
    }
    case State::S4CubeOnLeft:
    {
      TransitionToS5_Center();
      break;
    }
    case State::S5Center:
    {
      // do not transition, we will resume, but we won't do anything, since first tick will finish due to no actions
      break;
    }
    case State::Done:
    {
      // behavior is done, so let it end
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::OnBehaviorDeactivated()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::HandleWhileActivated(const EngineToGameEvent& event)
{
  if( event.GetData().GetTag() == EngineToGameTag::RobotObservedFace) {

    if( !_configParams.verifySeenFaces &&
        !_configParams.stopBehaviorOnAnyFace &&
        !_configParams.stopBehaviorOnNamedFace  ) {
      PRINT_NAMED_ERROR("BehaviorLookForFaceAndCube.HandleFace.InvalidConfig",
                        "We are handling a face event, but we shouldn't have subscribed because we don't care");
      return;
    }  
    
    const auto& msg = event.GetData().Get_RobotObservedFace();
    
    if( _configParams.verifySeenFaces ) {

      if( !_isVerifyingFace ) {
        const bool haveAlreadyVerifiedFace = ( _verifiedFaces.find( msg.faceID ) != _verifiedFaces.end() );
        const bool trackingOnlyFace = ( msg.faceID < 0 );
    
        if( trackingOnlyFace || !haveAlreadyVerifiedFace ) {
          // if we have a tracking only face, we always want to turn to it so we can verify it and get a real id
          // out of it (hopefully). Otherwise, turn towards it if we haven't already. After verifying, we'll
          // stop the behavior if we need to
          CancelActionAndVerifyFace(msg.faceID);
        }
      }
    }
    else {
      // since we aren't verifying, stop the behavior now, if needed
      StopBehaviorOnFaceIfNeeded(msg.faceID);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::StopBehaviorOnFaceIfNeeded(FaceID_t observedID)
{
  const bool trackingOnlyFace = ( observedID < 0 );

  if(!trackingOnlyFace) {
    // here we want to stop on some faces (named or any recognizable). If we are verifying faces _and_ want
    // to stop, the CancelActionAndVerifyFace function will handle that for us _after_ the verify, so don't
    // need to handle that here. We never kill the behavior on a tracking only face

    if( _configParams.stopBehaviorOnAnyFace ) {
      PRINT_CH_INFO("Behavior", (GetIDStr() + ".SawFace.End").c_str(),
                    "Stopping behavior because we saw (any) face id %d",
                    observedID);
        
      const bool allowCallbacks = false;
      CancelDelegates(allowCallbacks);
      TransitionToS6_Done();
    }
    else if ( _configParams.stopBehaviorOnNamedFace )
    {
      // we need to check if the face has a name
      
      auto* facePtr = GetBEI().GetFaceWorld().GetFace(observedID);
      if( ANKI_VERIFY(facePtr != nullptr,
                      "BehaviorLookForFaceAndCube.NullObservedFace",
                      "Face '%d' observed but faceworld returns null",
                      observedID) ) {
        
        if( facePtr->HasName() ) {
          PRINT_CH_INFO("Behavior", (GetIDStr() + ".SawFace.End").c_str(),
                        "Stopping behavior because we saw (any) face id %d",
                        observedID);
        
          const bool allowCallbacks = false;
          CancelDelegates(allowCallbacks);
          TransitionToS6_Done();
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::CancelActionAndVerifyFace(FaceID_t observedFace)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".VerifyFace").c_str(),
                "Stopping current action to verify face %d",
                observedFace);

  const bool allowCallbacks = false;
  CancelDelegates(allowCallbacks);
  
  SmartFaceID smartID = GetBEI().GetFaceWorld().GetSmartFaceID(observedFace);
  CompoundActionSequential* action = new CompoundActionSequential();
  action->AddAction( new TurnTowardsFaceAction(smartID, M_PI_F) );

  const bool isTrackingOnly = (observedFace < 0);
  if( isTrackingOnly ) {    
    action->AddAction( new WaitForImagesAction(kNumFramesToWaitForTrackingOnlyFace) );
    // note that if this turns into a real face, it may trigger another "turn towards" for that face ID
  }
  else {
    // don't bother waiting for any frames if we already have a name. Otherwise, give it a few frames to give
    // it a chance to realize that this may be a named person. Note: it would be better to do this logic
    // _after_ the turn to action completes, because we may collect data while turning, but I'm lazy
    auto* facePtr = GetBEI().GetFaceWorld().GetFace(observedFace);
    if( facePtr && !facePtr->HasName() ) {
      action->AddAction( new WaitForImagesAction(kNumFramesToWaitForUnNamedFace) );
    }
  }

  _isVerifyingFace = true;
  
  DelegateIfInControl(action, [this, observedFace](ActionResult res) {
      const bool isTrackingOnly = observedFace < 0;
      if( !isTrackingOnly && res == ActionResult::SUCCESS ) {
        _verifiedFaces.insert(observedFace);
      }

      _isVerifyingFace = false;
                                                 
      // we might want to stop now that we've seen a face
      StopBehaviorOnFaceIfNeeded(observedFace);

      // resume the state machine
      ResumeCurrentState();
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS1_FaceOnLeft()
{
  _currentState = State::S1FaceOnLeft;
  ++_currentSidePicksDone;
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".S1FaceOnLeft").c_str(),
    "Looking for face to my left (%u out of %u)", _currentSidePicksDone, _configParams.face_sidePicks);
  
  // create head move action
  IAction* leftLookForFace = CreateBodyAndHeadTurnAction(
    _configParams.face_bodyAngleRelRangeMin_rad,
    _configParams.face_bodyAngleRelRangeMax_rad,
    _startingBodyFacing_rad,
    _configParams.face_headAngleAbsRangeMin_rad,
    _configParams.face_headAngleAbsRangeMax_rad,
    _configParams.bodyTurnSpeed_radPerSec,
    _configParams.bodyTurnSpeed_radPerSec);
  
  // look here
  if ( _currentSidePicksDone < _configParams.face_sidePicks ) {
    DelegateIfInControl( leftLookForFace, &BehaviorLookForFaceAndCube::TransitionToS1_FaceOnLeft );  // left again
  } else {
    _currentSidePicksDone = 0; // reset for next state
    DelegateIfInControl( leftLookForFace, &BehaviorLookForFaceAndCube::TransitionToS2_FaceOnRight ); // go to right
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS2_FaceOnRight()
{
  _currentState = State::S2FaceOnRight;
  ++_currentSidePicksDone;
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".S2FaceOnRight").c_str(),
    "Looking for face to my right (%u out of %u)", _currentSidePicksDone, _configParams.face_sidePicks);
  
  // create head move action
  IAction* rightLookForFace = CreateBodyAndHeadTurnAction(
    -1.0f*_configParams.face_bodyAngleRelRangeMin_rad,
    -1.0f*_configParams.face_bodyAngleRelRangeMax_rad,
    _startingBodyFacing_rad,
    _configParams.face_headAngleAbsRangeMin_rad,
    _configParams.face_headAngleAbsRangeMax_rad,
    _configParams.bodyTurnSpeed_radPerSec,
    _configParams.bodyTurnSpeed_radPerSec);
  
  // look here
  if ( _currentSidePicksDone < _configParams.face_sidePicks ) {
    DelegateIfInControl( rightLookForFace, &BehaviorLookForFaceAndCube::TransitionToS2_FaceOnRight ); // right again
  } else {
    DelegateIfInControl( rightLookForFace, &BehaviorLookForFaceAndCube::TransitionToS3_CubeOnRight ); // go on to cubes
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS3_CubeOnRight()
{
  _currentState = State::S3CubeOnRight;
  ++_currentSidePicksDone;
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".S3CubeOnRight").c_str(),
    "Looking for cube to my right (%u out of %u)", _currentSidePicksDone, _configParams.cube_sidePicks);

  // create head move action
  IAction* rightLookForCube = CreateBodyAndHeadTurnAction(
    _configParams.cube_bodyAngleRelRangeMin_rad,
    _configParams.cube_bodyAngleRelRangeMax_rad,
    _startingBodyFacing_rad,
    _configParams.cube_headAngleAbsRangeMin_rad,
    _configParams.cube_headAngleAbsRangeMax_rad,
    _configParams.bodyTurnSpeed_radPerSec,
    _configParams.bodyTurnSpeed_radPerSec);
  
  // look here
  if ( _currentSidePicksDone < _configParams.cube_sidePicks ) {
    DelegateIfInControl( rightLookForCube, &BehaviorLookForFaceAndCube::TransitionToS3_CubeOnRight ); // right again
  } else {
    _currentSidePicksDone = 0; // reset for next state
    DelegateIfInControl( rightLookForCube, &BehaviorLookForFaceAndCube::TransitionToS4_CubeOnLeft ); // go on
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS4_CubeOnLeft()
{
  _currentState = State::S4CubeOnLeft;
  ++_currentSidePicksDone;
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".S4CubeOnLeft").c_str(),
    "Looking for cube to my left (%u out of %u)", _currentSidePicksDone, _configParams.cube_sidePicks);

  // create head move action
  IAction* leftLookForCube = CreateBodyAndHeadTurnAction(
    -1.0f*_configParams.cube_bodyAngleRelRangeMin_rad,
    -1.0f*_configParams.cube_bodyAngleRelRangeMax_rad,
    _startingBodyFacing_rad,
    _configParams.cube_headAngleAbsRangeMin_rad,
    _configParams.cube_headAngleAbsRangeMax_rad,
    _configParams.bodyTurnSpeed_radPerSec,
    _configParams.bodyTurnSpeed_radPerSec);
  
  // look here
  if ( _currentSidePicksDone < _configParams.cube_sidePicks ) {
    DelegateIfInControl( leftLookForCube, &BehaviorLookForFaceAndCube::TransitionToS4_CubeOnLeft ); // left again
  } else {
    _currentSidePicksDone = 0; // reset for next state
    DelegateIfInControl( leftLookForCube, &BehaviorLookForFaceAndCube::TransitionToS5_Center ); // go on
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS5_Center()
{
  _currentState = State::S5Center;
  ++_currentSidePicksDone;
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".S4CubeOnLeft").c_str(),
    "Looking for cube to my left (%u out of %u)", _currentSidePicksDone, _configParams.cube_sidePicks);

  // create head move action
  IAction* centerLookForCube = CreateBodyAndHeadTurnAction(
    0,
    0,
    _startingBodyFacing_rad,
    _configParams.cube_headAngleAbsRangeMin_rad,
    _configParams.cube_headAngleAbsRangeMax_rad,
    _configParams.bodyTurnSpeed_radPerSec,
    _configParams.bodyTurnSpeed_radPerSec);

  // look here and do only 1 iteration
  _currentSidePicksDone = 0; // reset for next state
  DelegateIfInControl( centerLookForCube, &BehaviorLookForFaceAndCube::TransitionToS6_Done ); // done after
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLookForFaceAndCube::TransitionToS6_Done()
{
  _currentState = State::Done;
  // Note that this is specific to the PutDownDispatch activity. If this behavior needs to be generic for other activities, it
  // should store the start time on InitInternal and pass it here to compare timestamps. The reason why I don't implement
  // it that way is that I want to also to track objects seen during putdown reactions, which would not be included in
  // that case. We could also json config it, but I don't have a reason for it atm

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAction* BehaviorLookForFaceAndCube::CreateBodyAndHeadTurnAction(
  const Radians& bodyRelativeMin_rad, const Radians& bodyRelativeMax_rad,
  const Radians& bodyAbsoluteTargetAngle_rad,
  const Radians& headAbsoluteMin_rad, const Radians& headAbsoluteMax_rad,
  const Radians& bodyTurnSpeed_radPerSec,
  const Radians& headTurnSpeed_radPerSec)
{
  // [min,max] range for random body angle turn
  const double bodyRelativeRandomAngle_rad = GetRNG().RandDblInRange(bodyRelativeMin_rad.ToDouble(), bodyRelativeMax_rad.ToDouble());
  const Radians bodyTargetAngleAbs_rad = bodyAbsoluteTargetAngle_rad + bodyRelativeRandomAngle_rad;
  
  // [min,max] range for random head angle turn
  const Radians headTargetAngleAbs_rad = GetRNG().RandDblInRange(headAbsoluteMin_rad.ToDouble(), headAbsoluteMax_rad.ToDouble());

  // create proper action for body & head turn
  PanAndTiltAction* turnAction = new PanAndTiltAction(bodyTargetAngleAbs_rad, headTargetAngleAbs_rad, true, true);
  turnAction->SetMaxPanSpeed ( bodyTurnSpeed_radPerSec.ToDouble() );
  turnAction->SetMaxTiltSpeed( headTurnSpeed_radPerSec.ToDouble() );

  return turnAction;
}

} // namespace Cozmo
} // namespace Anki
