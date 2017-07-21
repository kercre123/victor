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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorDriveInDesperation.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/behaviorHelperComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorHelpers/behaviorHelperParameters.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/pathComponent.h"
#include "anki/cozmo/basestation/robot.h"
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
BehaviorDriveInDesperation::BehaviorDriveInDesperation(Robot& robot, const Json::Value& config)
  : super(robot, config)
{
  _params = std::make_unique<Params>(config);

  if( _params->_useCubes ) {
    _validCubesFilter = std::make_unique<BlockWorldFilter>();
    _validCubesFilter->SetAllowedFamilies({ ObjectFamily::LightCube });
    _validCubesFilter->AddFilterFcn(&BlockWorldFilter::ActiveObjectsFilter);
  }

  // start with a randomized driving value so we don't always request right away
  RandomizeNumDrivingRounds();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDriveInDesperation::~BehaviorDriveInDesperation()
{
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorDriveInDesperation::InitInternal(Robot& robot)
{
  SmartSetMotionProfile( _params->_motionProfile );

  _targetCube.UnSet();

  TransitionToIdle(robot);

  return Result::RESULT_OK;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::StopInternal(Robot& robot)
{
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status BehaviorDriveInDesperation::UpdateInternal(Robot& robot)
{
  if( robot.GetOffTreadsState() != OffTreadsState::OnTreads ) {
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
    TransitionToIdle(robot);
  }

  return Base::UpdateInternal(robot);
}


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorDriveInDesperation::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  return true;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToIdle(Robot& robot)
{
  SET_STATE(Idle);

  // decide how long to idle
  const float timeToIdle = GetRNG().RandDblInRange(_params->_minTimeToIdle, _params->_maxTimeToIdle);
  PRINT_CH_INFO("Behaviors",
                (GetIDStr() + ".Idle").c_str(),
                "idling for %f sec",
                timeToIdle);

  StartActing(new WaitAction(robot, timeToIdle), &BehaviorDriveInDesperation::TransitionFromIdle);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToDriveRandom(Robot& robot)
{
  SET_STATE(DriveRandom);

  Pose3d randomPose;
  GetRandomDrivingPose(robot, randomPose);

  // drive somewhere and then idle
  const bool lookDown = true;
  DriveToPoseAction* driveAction = new DriveToPoseAction(robot, randomPose, lookDown);
  
  StartActing(driveAction, [this](Robot& robot) {
      // if we are using cubes, do a search now at this location. Otherwise, leave the behavior
      if( _params->_useCubes ) {
        TransitionToSearchForCube(robot);
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
void BehaviorDriveInDesperation::TransitionToRequest(Robot& robot)
{
  SET_STATE(Request);
  // TODO:(bn) turn towards previous face instead of most recent (similar to "hey cozmo" behavior)

  IActionRunner* animAction = new TriggerAnimationAction(robot, _params->_requestAnimTrigger);
  TurnTowardsFaceWrapperAction* faceAction = new TurnTowardsFaceWrapperAction(robot, animAction);
  
  StartActing(faceAction, [this](Robot& robot) {
      // if we were visiting a cube before this request, turn back to it before finishing (and likely looping
      // back to the idle state). Otherwise just finish now
      if( _targetCube.IsSet() ) {
        TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot, _targetCube);
        StartActing(turnAction);
        // we are done with this cube now.
        _targetCube.UnSet();
      }
    });
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToSearchForCube(Robot& robot)
{
  SET_STATE(SearchForCube);

  auto& factory = robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
  SearchParameters params;
  params.numberOfBlocksToLocate = 1; // stop if we find a single block
  HelperHandle searchHelper = factory.CreateSearchForBlockHelper(robot, *this, params);

  // start the helper and automatically drive to the cube if one is found. Otherwise, let the behavior end
  SmartDelegateToHelper(robot,
                        searchHelper,
                        &BehaviorDriveInDesperation::TransitionToDriveToCube);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToDriveToCube(Robot& robot)
{
  SET_STATE(DriveToCube);
  
  // if no cube is set, then we want to drive to the one we saw most recently
  if( !_targetCube.IsSet() ) {
    const ObservableObject* recentObj =
      robot.GetBlockWorld().FindMostRecentlyObservedObject( *_validCubesFilter );
    
    if( nullptr != recentObj ) {
      _targetCube = recentObj->GetID();
    }
    else {
      PRINT_NAMED_WARNING("BehaviorDriveInDesperation.DriveToCube.NoCube",
                          "%s: Trying to drive to cube, but no target set and no recent cube seen",
                          GetIDStr().c_str());
      TransitionToIdle(robot);
      return;
    }
  }

  // cube must be set by this point
  DEV_ASSERT(_targetCube.IsSet(), "BehaviorDriveInDesperation.DriveToCube.CubeNotSetLogicBug");

  // manually grab pre-action poses to drive to (so we don't use the normal machinery with retires and cube
  // lights) use rolling action since it works from every angle


  ActionableObject* obj = dynamic_cast<ActionableObject*>( robot.GetBlockWorld().GetLocatedObjectByID(_targetCube) );
  if( !ANKI_VERIFY( obj != nullptr,
                    "BehaviorDriveInDesperation.TransitionToDriveToCube.NullCube",
                    "%s: Have a set cube id of %d, but that cube isn't located by blockworld, or isn't actionable",
                    GetIDStr().c_str(),
                    _targetCube.GetValue()) ) {
    _targetCube.UnSet();
    TransitionToIdle(robot);
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
  
  IDockAction::GetPreActionPoses(robot, preActionPoseInput, preActionPoseOutput);

  const bool hasPoseToDriveTo = preActionPoseOutput.actionResult == ActionResult::SUCCESS &&
    ! preActionPoseOutput.robotAtClosestPreActionPose;

  if( hasPoseToDriveTo ) {
    std::vector<Pose3d> poses;
    for( const auto& preActionPose : preActionPoseOutput.preActionPoses ) {
      poses.emplace_back(preActionPose.GetPose());
    }
    
    DriveToPoseAction* driveAction = new DriveToPoseAction(robot,
                                                           poses,
                                                           false, // don't force head down
                                                           false, // no manual speed
                                                           Point3f{kDriveToCubeDistTolerance},
                                                           DEG_TO_RAD(kDriveToCubeAngleTolerance_deg));
    // don't bother with retries or anything, always just look at the cube once this is done
    StartActing(driveAction, &BehaviorDriveInDesperation::TransitionToLookAtCube);
  }
  else {
    // already at a predock pose, or couldn't get any poses, so just turn towards the object
    TransitionToLookAtCube(robot);
  }

}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionToLookAtCube(Robot& robot)
{
  SET_STATE(LookAtCube);

  if( !ANKI_VERIFY(_targetCube.IsSet(),
                   "BehaviorDriveInDesperation.TransitionToLookAtCube.NoCube",
                   "No target cube to look at!") ) {
    TransitionToIdle(robot);
    return;
  }

  const bool verifyWhenDone = true;
  const bool headTrackWhenDone = false;
  TurnTowardsObjectAction* turnAction = new TurnTowardsObjectAction(robot,
                                                                    _targetCube,
                                                                    Radians{M_PI_F},
                                                                    verifyWhenDone,
                                                                    headTrackWhenDone);
  StartActing(turnAction, [this](ActionResult res, Robot& robot) {
      if( res == ActionResult::SUCCESS ) {
        // there's a cube here, so do a request to the user
        TransitionToRequest(robot);
      }
      // else, didn't find a cube, so end the behavior (which will go to idle if the behavior is re-selected)
    });
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle(Robot& robot)
{
  if( _params->_useCubes ) {
    TransitionFromIdle_WithCubes(robot);
  }
  else {
    TransitionFromIdle_NoCubes(robot);
  }
}
    

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle_NoCubes(Robot& robot)
{
  if( _numTimesToDriveBeforeRequest > 0 ) {
    _numTimesToDriveBeforeRequest--;
    TransitionToDriveRandom(robot);
  }
  else {
    RandomizeNumDrivingRounds();
    TransitionToRequest(robot);
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::TransitionFromIdle_WithCubes(Robot& robot)
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
    robot.GetBlockWorld().FindLocatedMatchingObjects(*_validCubesFilter, cubes);

    if( !cubes.empty() ) {
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
        TransitionToDriveToCube(robot);
        return;
      }
    }
  }

  // we aren't driving to a cube, so consider driving to a random location or doing a request, just like if we
  // weren't using cubes
  TransitionFromIdle_NoCubes(robot);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDriveInDesperation::GetRandomDrivingPose(const Robot& robot, Pose3d& outPose)
{
  // TODO:(bn) use memory map! avoid obstacles!!

  const float turnAngle_deg = GetRNG().RandDblInRange(kMinTurnAngle_deg, kMaxTurnAngle_deg);
  const float sign = GetRNG().RandDbl() < 0.5 ? -1.0f : 1.0f;

  const float turnAngle = sign * DEG_TO_RAD(turnAngle_deg);

  const float distance_mm = GetRNG().RandDblInRange(kMinDriveDistance_mm, kMaxDriveDistance_mm);

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
