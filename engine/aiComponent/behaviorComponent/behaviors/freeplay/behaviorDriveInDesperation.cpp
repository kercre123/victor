/**
 * File: behaviorDriveInDesperation.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-07-11
 *
 * Description: Behavior to randomly drive around in desperation, to be used in severe low needs states
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorDriveInDesperation.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorHelpers/behaviorHelperParameters.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/pathComponent.h"
#include "engine/robot.h"
#include "clad/types/pathMotionProfile.h"
#include "util/console/consoleInterface.h"


#define SET_STATE(s) SetState_internal(State::s, #s)

namespace Anki {
namespace Cozmo {

namespace {

#define CONSOLE_GROUP "Behavior.DriveInDesperation"

CONSOLE_VAR(u32, kMinTimesToDrive, CONSOLE_GROUP, 1);
CONSOLE_VAR(u32, kMaxTimesToDrive, CONSOLE_GROUP, 3);

CONSOLE_VAR(f32, kMinTurnAngle_deg, CONSOLE_GROUP, 40.0f);
CONSOLE_VAR(f32, kMaxTurnAngle_deg, CONSOLE_GROUP, 100.0f);

CONSOLE_VAR(f32, kMinDriveDistance_mm, CONSOLE_GROUP, 50.0f);
CONSOLE_VAR(f32, kMaxDriveDistance_mm, CONSOLE_GROUP, 150.0f);

// if using cubes, this is how often to re-visit them
CONSOLE_VAR(f32, kMinTimeToRevisitCubes_s, CONSOLE_GROUP, 15.0f);
CONSOLE_VAR(f32, kMaxTimeToRevisitCubes_s, CONSOLE_GROUP, 30.0f);

CONSOLE_VAR(f32, kDriveToCubeDistTolerance, CONSOLE_GROUP, 30.0f);
CONSOLE_VAR(f32, kDriveToCubeAngleTolerance_deg, CONSOLE_GROUP, 15);


static const char* kAnimTriggerKey = "requestAnimTrigger";
static const char* kMotionProfileKey = "motionProfile";
static const char* kUseCubesKey = "useCubes";
static const char* kMinTimeToIdleKey = "minTimeToIdle";
static const char* kMaxTimeToIdleKey = "maxTimeToIdle";

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BehaviorDriveInDesperation::Params
{
  AnimationTrigger _requestAnimTrigger;
  PathMotionProfile _motionProfile;
  bool _useCubes;

  float _minTimeToIdle;
  float _maxTimeToIdle;
  
  Params(const Json::Value& config) {
    std::string animTriggerStr = JsonTools::ParseString(config,
                                                        kAnimTriggerKey,
                                                        "BehaviorDriveInDesperation.Params.AnimTrigger");
    _requestAnimTrigger = AnimationTriggerFromString( animTriggerStr );

    ANKI_VERIFY(config.isMember(kMotionProfileKey),
                "BehaviorDriveInDesperation.Params.MotionProfile.NotSpecified",
                "'%s' is not a valid json group",
                kMotionProfileKey);
                
    const bool res = _motionProfile.SetFromJSON(config[kMotionProfileKey]);
    ANKI_VERIFY(res,
                "BehaviorDriveInDesperation.Params.MotionProfile.ParseFail",
                "Could not parse motion profile from config");

    _useCubes = JsonTools::ParseBool(config, kUseCubesKey, "BehaviorDriveInDesperation.Params.UseCubes");

    _minTimeToIdle = JsonTools::ParseFloat(config, kMinTimeToIdleKey, "BehaviorDriveInDesperation.Params.MinTimeToIdle");
    _maxTimeToIdle = JsonTools::ParseFloat(config, kMaxTimeToIdleKey, "BehaviorDriveInDesperation.Params.MaxTimeToIdle");

  }
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveInDesperation::BehaviorDriveInDesperation(const Json::Value& config)
  : super(config)
{
  _params = std::make_unique<Params>(config);

  if( _params->_useCubes ) {
    _validCubesFilter = std::make_unique<BlockWorldFilter>();
    _validCubesFilter->SetAllowedFamilies({ ObjectFamily::LightCube });
    _validCubesFilter->AddFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
  }
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveInDesperation::~BehaviorDriveInDesperation()
{
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) {
  // start with a randomized driving value so we don't always request right away
  RandomizeNumDrivingRounds();
};


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveInDesperation::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  SmartSetMotionProfile( _params->_motionProfile );

  _targetCube.UnSet();

  TransitionToIdle(behaviorExternalInterface);

  return Result::RESULT_OK;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface)
{
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorDriveInDesperation::UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( behaviorExternalInterface.GetOffTreadsState() != OffTreadsState::OnTreads ) {
    if( !_wasPickedUp ) {
      // this behavior can run while in the air, in which case it just does nothing. This is because the severe
      // low needs activities don't react to pickup. Cancel any actions first
      StopActing(false);
      _wasPickedUp = true;
    }
    return Status::Running;
  }
  else if( _wasPickedUp ) {
    // we were picked up, but are no longer, so go back to the idle state
    _wasPickedUp = false;
    _targetCube.UnSet();
    TransitionToIdle(behaviorExternalInterface);
  }

  return Base::UpdateInternal_WhileRunning(behaviorExternalInterface);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveInDesperation::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  return true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToIdle(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(Idle);

  // decide how long to idle
  const float timeToIdle = GetRNG().RandDblInRange(_params->_minTimeToIdle, _params->_maxTimeToIdle);
  PRINT_CH_INFO("Behaviors",
                (GetIDStr() + ".Idle").c_str(),
                "idling for %f sec",
                timeToIdle);
  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  DelegateIfInControl(new WaitAction(robot, timeToIdle), &BehaviorDriveInDesperation::TransitionFromIdle);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToDriveRandom(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(DriveRandom);

  Pose3d randomPose;
  GetRandomDrivingPose(behaviorExternalInterface, randomPose);

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  // drive somewhere and then idle
  const bool lookDown = true;
  DriveToPoseAction* driveAction = new DriveToPoseAction(robot, randomPose, lookDown);
  
  DelegateIfInControl(driveAction, [this](BehaviorExternalInterface& behaviorExternalInterface) {
      // if we are using cubes, do a search now at this location. Otherwise, leave the behavior
      if( _params->_useCubes ) {
        TransitionToSearchForCube(behaviorExternalInterface);
      }
      // we're done for now, if we are re-selected, we'll idle
    });
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::RandomizeNumDrivingRounds()
{
  // finished driving, first figure out how many times to drive next time
  _numTimesToDriveBeforeRequest = GetRNG().RandIntInRange(kMinTimesToDrive, kMaxTimesToDrive);
  PRINT_CH_INFO("Behaviors",
                (GetIDStr() + ".Drive").c_str(),
                "driving to %d random points next time we drive",
                _numTimesToDriveBeforeRequest);  
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToRequest(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(Request);
  // TODO:(bn) turn towards previous face instead of most recent (similar to "hey cozmo" behavior)

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  IActionRunner* animAction = new TriggerAnimationAction(robot, _params->_requestAnimTrigger);
  TurnTowardsFaceWrapperAction* faceAction = new TurnTowardsFaceWrapperAction(robot, animAction);
  
  DelegateIfInControl(faceAction, [this, &robot](BehaviorExternalInterface& behaviorExternalInterface) {
      // if we were visiting a cube before this request, turn back to it before finishing (and likely looping
      // back to the idle state). Otherwise just finish now
      if( _targetCube.IsSet() ) {
        TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, _targetCube);
        DelegateIfInControl(turnAction);
        // we are done with this cube now.
        _targetCube.UnSet();
      }
    });
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToSearchForCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(SearchForCube);

  auto& factory = behaviorExternalInterface.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  SearchParameters params;
  params.numberOfBlocksToLocate = 1; // stop if we find a single block
  HelperHandle searchHelper = factory.CreateSearchForBlockHelper(behaviorExternalInterface, *this, params);

  // start the helper and automatically drive to the cube if one is found. Otherwise, let the behavior end
  SmartDelegateToHelper(behaviorExternalInterface,
                        searchHelper,
                        &BehaviorDriveInDesperation::TransitionToDriveToCube);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToDriveToCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(DriveToCube);
  
  // if no cube is set, then we want to drive to the one we saw most recently
  if( !_targetCube.IsSet() ) {
    const ObservableObject* recentObj =
      behaviorExternalInterface.GetBlockWorld().FindMostRecentlyObservedObject( *_validCubesFilter );
    
    if( nullptr != recentObj ) {
      _targetCube = recentObj->GetID();
    }
    else {
      PRINT_NAMED_WARNING("BehaviorDriveInDesperation.DriveToCube.NoCube",
                          "%s: Trying to drive to cube, but no target set and no recent cube seen",
                          GetIDStr().c_str());
      TransitionToIdle(behaviorExternalInterface);
      return;
    }
  }

  // cube must be set by this point
  DEV_ASSERT(_targetCube.IsSet(), "BehaviorDriveInDesperation.DriveToCube.CubeNotSetLogicBug");

  // manually grab pre-action poses to drive to (so we don't use the normal machinery with retires and cube
  // lights) use rolling action since it works from every angle


  const ActionableObject* obj = dynamic_cast<const ActionableObject*>(
            behaviorExternalInterface.GetBlockWorld().GetLocatedObjectByID(_targetCube) );
  if( !ANKI_VERIFY( obj != nullptr,
                    "BehaviorDriveInDesperation.TransitionToDriveToCube.NullCube",
                    "%s: Have a set cube id of %d, but that cube isn't located by blockworld, or isn't actionable",
                    GetIDStr().c_str(),
                    _targetCube.GetValue()) ) {
    _targetCube.UnSet();
    TransitionToIdle(behaviorExternalInterface);
    return;
  }
  
  IDockAction::PreActionPoseInput preActionPoseInput(obj,
                                                     PreActionPose::ActionType::ROLLING,
                                                     false, // don't fail based on predock check
                                                     DEG_TO_RAD(kDriveToCubeAngleTolerance_deg),
                                                     0.0f, // no docking offset
                                                     false, // no approach angle
                                                     0.0f);
  IDockAction::PreActionPoseOutput preActionPoseOutput;
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    IDockAction::GetPreActionPoses(robot, preActionPoseInput, preActionPoseOutput);
  }

  const bool hasPoseToDriveTo = preActionPoseOutput.actionResult == ActionResult::SUCCESS &&
    ! preActionPoseOutput.robotAtClosestPreActionPose;

  if( hasPoseToDriveTo ) {
    std::vector<Pose3d> poses;
    for( const auto& preActionPose : preActionPoseOutput.preActionPoses ) {
      poses.emplace_back(preActionPose.GetPose());
    }
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    DriveToPoseAction* driveAction = new DriveToPoseAction(robot,
                                                           poses,
                                                           false, // don't force head down
                                                           false, // no manual speed
                                                           Point3f{kDriveToCubeDistTolerance},
                                                           DEG_TO_RAD(kDriveToCubeAngleTolerance_deg));
    // don't bother with retries or anything, always just look at the cube once this is done
    DelegateIfInControl(driveAction, &BehaviorDriveInDesperation::TransitionToLookAtCube);
  }
  else {
    // already at a predock pose, or couldn't get any poses, so just turn towards the object
    TransitionToLookAtCube(behaviorExternalInterface);
  }

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToLookAtCube(BehaviorExternalInterface& behaviorExternalInterface)
{
  SET_STATE(LookAtCube);

  if( !ANKI_VERIFY(_targetCube.IsSet(),
                   "BehaviorDriveInDesperation.TransitionToLookAtCube.NoCube",
                   "No target cube to look at!") ) {
    TransitionToIdle(behaviorExternalInterface);
    return;
  }

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  Robot& robot = behaviorExternalInterface.GetRobot();
  
  const bool verifyWhenDone = true;
  const bool headTrackWhenDone = false;
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot,
                                                                    _targetCube,
                                                                    Radians{M_PI_F},
                                                                    verifyWhenDone,
                                                                    headTrackWhenDone);
  DelegateIfInControl(turnAction, [this](ActionResult res, BehaviorExternalInterface& behaviorExternalInterface) {
      if( res == ActionResult::SUCCESS ) {
        // there's a cube here, so do a request to the user
        TransitionToRequest(behaviorExternalInterface);
      }
      // else, didn't find a cube, so end the behavior (which will go to idle if the behavior is re-selected)
    });
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _params->_useCubes ) {
    TransitionFromIdle_WithCubes(behaviorExternalInterface);
  }
  else {
    TransitionFromIdle_NoCubes(behaviorExternalInterface);
  }
}
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle_NoCubes(BehaviorExternalInterface& behaviorExternalInterface)
{
  if( _numTimesToDriveBeforeRequest > 0 ) {
    _numTimesToDriveBeforeRequest--;
    TransitionToDriveRandom(behaviorExternalInterface);
  }
  else {
    RandomizeNumDrivingRounds();
    TransitionToRequest(behaviorExternalInterface);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle_WithCubes(BehaviorExternalInterface& behaviorExternalInterface)
{
  // we shouldn't have a target yet (or it should have been cleared by now)
  DEV_ASSERT( !_targetCube.IsSet(), "BehaviorDriveInDesperation.TransitionFromIdle_WithCubes.TargetAlreadySet" );

  // first check if it's time to visit a cube
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( _nextTimeToRevisitCube < 0.0f || currTime_s >= _nextTimeToRevisitCube ) {

    // update the next time we should try to drive to a cube
    _nextTimeToRevisitCube = currTime_s + GetRNG().RandDblInRange(kMinTimeToRevisitCubes_s, kMaxTimeToRevisitCubes_s);

    // if we have any cubes, now's the time to visit them. Always visit the _furthest_ cube
    std::vector< const ObservableObject* > cubes;
    behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(*_validCubesFilter, cubes);

    if( !cubes.empty() ) {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should
      // be removed
      const Robot& robot = behaviorExternalInterface.GetRobot();
      
      float maxDistanceSq = -1.0f;
      ObjectID furthestObject;
      for( const auto* cube : cubes ) {
        float distSq = std::numeric_limits<float>::max();
        const bool distanceOK = ComputeDistanceSQBetween(robot.GetPose(), cube->GetPose(), distSq);
        if( distanceOK && distSq > maxDistanceSq ) {
          maxDistanceSq = distSq;
          furthestObject = cube->GetID();
        }
      }

      if( ANKI_VERIFY( furthestObject.IsSet(),
                       "BehaviorDriveInDesperation.FindTargetCube.NoValidDistance",
                       "Got %zu cubes, but none has valid distance >= 0",
                       cubes.size() ) ) {
        _targetCube = furthestObject;
        TransitionToDriveToCube(behaviorExternalInterface);
        return;
      }
    }
  }

  // we aren't driving to a cube, so consider driving to a random location or doing a request, just like if we
  // weren't using cubes
  TransitionFromIdle_NoCubes(behaviorExternalInterface);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::GetRandomDrivingPose(const BehaviorExternalInterface& behaviorExternalInterface, Pose3d& outPose)
{
  // TODO:(bn) use memory map! avoid obstacles!!

  const float turnAngle_deg = GetRNG().RandDblInRange(kMinTurnAngle_deg, kMaxTurnAngle_deg);
  const float sign = GetRNG().RandDbl() < 0.5 ? -1.0f : 1.0f;

  const float turnAngle = sign * DEG_TO_RAD(turnAngle_deg);

  const float distance_mm = GetRNG().RandDblInRange(kMinDriveDistance_mm, kMaxDriveDistance_mm);

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should
  // be removed
  const Robot& robot = behaviorExternalInterface.GetRobot();
  
  outPose = robot.GetPose();

  Radians newAngle = outPose.GetRotationAngle<'Z'>() + Radians(turnAngle);
  outPose.SetRotation( outPose.GetRotation() * Rotation3d{ newAngle, Z_AXIS_3D() } );
  outPose.TranslateForward(distance_mm);

  PRINT_CH_DEBUG("Behaviors", "BehaviorDriveInDesperation.GetRandomDrivingPose",
                 "%s: angle=%fdeg, dist=%fmm, currPose = (%f, %f, %f) R=%fdeg newPose = (%f, %f, %f) R=%fdeg",
                 GetIDStr().c_str(),
                 RAD_TO_DEG(turnAngle),
                 distance_mm,
                 robot.GetPose().GetTranslation().x(),
                 robot.GetPose().GetTranslation().y(),
                 robot.GetPose().GetTranslation().z(),
                 robot.GetPose().GetRotationAngle<'Z'>().getDegrees(),
                 outPose.GetTranslation().x(),
                 outPose.GetTranslation().y(),
                 outPose.GetTranslation().z(),
                 outPose.GetRotationAngle<'Z'>().getDegrees());
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::SetState_internal(State state, const std::string& stateName)
{
  _currentState = state;
  SetDebugStateName(stateName);
}

}
}
