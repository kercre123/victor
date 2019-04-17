/**
 * File: BehaviorFindCube.h
 *
 * Author: Sam Russell
 * Created: 2018-10-15
 *
 * Description: Quickly check the last known location of a cube and search nearby for one if none are
 *              known/found in the initial check
 *
 * Copyright: Anki, Inc. 2018
 *
 **/



#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCube.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
                          _dVars.state = FindCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{

static const float kDistThreshold_mm = 2.0f;
static const float kAngleThreshold_deg = 2.0f;
static const float kProxBackupThreshold_mm = 90.0f;
static const float kMinSearchAngle = 330.0f;
static const int   kMaxNumSearchTurns = 10;
static const int   kNumImagesToWaitDuringSearch = 2;

static const char* kSkipReactToCubeKey = "skipReactToCubeAnim";
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCube::InstanceConfig::InstanceConfig()
: cubesFilter(std::make_unique<BlockWorldFilter>())
, skipReactToCubeAnim(false) // play the cube reaction by default
{
  cubesFilter->AddFilterFcn(&BlockWorldFilter::IsLightCubeFilter);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCube::DynamicVariables::DynamicVariables()
: state(FindCubeState::GetIn)
, cubeID()
, lastPoseCheckTimestamp(0)
, angleSwept_deg(0.0f)
, numTurnsCompleted(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCube::BehaviorFindCube(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kSkipReactToCubeKey, _iConfig.skipReactToCubeAnim);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCube::~BehaviorFindCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindCube::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::Markers, EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::Markers_FullFrame,EVisionUpdateFrequency::High });
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = false; 
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSkipReactToCubeKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerCube));
  DEV_ASSERT(nullptr != _iConfig.driveOffChargerBehavior,
             "BehaviorFindCube.InitBehavior.NullDriveOffChargerBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  if(GetBEI().GetRobotInfo().IsOnChargerContacts() || GetBEI().GetRobotInfo().IsOnChargerPlatform()) {
    TransitionToDriveOffCharger();
  }
  else {
    TransitionToVisuallyCheckLastKnown();
  } 
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }


  if( FindCubeState::CheckForCubeInFront == _dVars.state || 
      FindCubeState::QuickSearchForCube  == _dVars.state ){
    const auto& proxSensor = GetBEI().GetRobotInfo().GetProxSensorComponent();
    const auto& proxData = proxSensor.GetLatestProxData();
    if( proxData.foundObject && ( proxData.distance_mm < kProxBackupThreshold_mm ) ){
      TransitionToBackUpAndCheck();
    }

    // If we don't have a target yet, check if a cube has been seen in the last frame 
    auto* targetCube = GetTargetCube();
    if(nullptr == targetCube){
      UpdateTargetCube();
      targetCube = GetTargetCube();
      // If we just got a target for the first time, move on to approaching it
      if(nullptr != targetCube){
        CancelDelegates(false);
        TransitionToReactToCube();
      }
    }
    else{
      // Otherwise, we already had a target but it wasn't found where expected during VisuallyVerify.
      // Watch for new observations and pose updates
      if(targetCube->GetLastObservedTime() > _dVars.lastPoseCheckTimestamp){
        _dVars.lastPoseCheckTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
        Pose3d cubePose = targetCube->GetPose();
        if( !cubePose.IsSameAs(_dVars.cubePoseAtSearchStart, kDistThreshold_mm, DEG_TO_RAD(kAngleThreshold_deg)) ){
          CancelDelegates(false);
          TransitionToReactToCube();
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);

  if(_iConfig.driveOffChargerBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(),
                        &BehaviorFindCube::TransitionToVisuallyCheckLastKnown);
  }
  else{
    LOG_WARNING("BehaviorFindCube.TransitionError", "Behavior %s did not want to be activated",
                _iConfig.driveOffChargerBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToVisuallyCheckLastKnown()
{
  SET_STATE(VisuallyCheckLastKnown);

  if(UpdateTargetCube()){
    bool visuallyVerify = true;
    TurnTowardsObjectAction* visualCheckAction = new TurnTowardsObjectAction(_dVars.cubeID,
                                                                             Radians{M_PI_F},
                                                                             visuallyVerify);
    DelegateIfInControl(visualCheckAction, [this](ActionResult result){
      if(ActionResult::SUCCESS == result){
        TransitionToReactToCube();
      }
      else{
        TransitionToCheckForCubeInFront();
      }
    });
  }
  else{
    TransitionToCheckForCubeInFront();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToCheckForCubeInFront()
{
  SET_STATE(CheckForCubeInFront);

  const bool isAbsolute = false;
  auto searchInFrontAction = new CompoundActionSequential({
    new MoveHeadToAngleAction(0.0f),
    new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::Markers),
    new TurnInPlaceAction(DEG_TO_RAD(30.0f), isAbsolute),
    new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::Markers),
    new TurnInPlaceAction(DEG_TO_RAD(-60.0f), isAbsolute),
    new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::Markers),
  });

  auto* targetCube = GetTargetCube();
  if(nullptr != targetCube){
    // If we made it here then the visual verification failed, so the cube pose is antequated and wrong
    // Make a note of it, and don't trust the cube pose until it is updated, indicating new observations.
    _dVars.cubePoseAtSearchStart = targetCube->GetPose();
    _dVars.lastPoseCheckTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  }

  DelegateIfInControl(searchInFrontAction, &BehaviorFindCube::TransitionToQuickSearchForCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToQuickSearchForCube()
{
  SET_STATE(QuickSearchForCube);
  StartNextTurn();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToBackUpAndCheck()
{
  SET_STATE(BackUpAndCheck);
  CancelDelegates(false);

  CompoundActionSequential* backupAndCheckAction = new CompoundActionSequential();
  backupAndCheckAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToObstacle));

  auto* loopAndWaitAction = new CompoundActionParallel();
  loopAndWaitAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ChargerDockingSearchWaitForImages));
  loopAndWaitAction->AddAction(new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::Markers));
  backupAndCheckAction->AddAction(loopAndWaitAction);

  DelegateIfInControl(backupAndCheckAction, &BehaviorFindCube::TransitionToQuickSearchForCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToReactToCube()
{
  if(_iConfig.skipReactToCubeAnim){
    CancelSelf();
  }
  else {
    SET_STATE(ReactToCube);
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FindCubeReactToCube));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::TransitionToGetOutFailure()
{
  SET_STATE(GetOutFailure);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFailure));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCube::StartNextTurn()
{
  // Have we turned enough to sweep the required angle?
  // Or have we exceeded the max number of turns?
  const bool sweptEnough = (_dVars.angleSwept_deg > kMinSearchAngle);
  const bool exceededMaxTurns = (_dVars.numTurnsCompleted >= kMaxNumSearchTurns);

  if (sweptEnough || exceededMaxTurns) {
    // If we made it all the way through the search without finding anything, call it a failure
    TransitionToGetOutFailure();
    LOG_INFO("BehaviorFindCube.TransitionToSearchTurn.QuickSearchFoundNothing",
             "We have completed a full search. Played %d turn animations (max is %d), "
             "and swept an angle of %.2f deg (min required sweep angle %.2f deg)",
             _dVars.numTurnsCompleted,
             kMaxNumSearchTurns,
             _dVars.angleSwept_deg,
             kMinSearchAngle);
  } else {
    auto* action = new CompoundActionSequential();

    // Always do the "search turn"
    action->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::FindCubeTurns));

    auto* loopAndWaitAction = new CompoundActionParallel();
    loopAndWaitAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::FindCubeWaitLoop));
    loopAndWaitAction->AddAction(new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::Markers));
    action->AddAction(loopAndWaitAction);

    // Keep track of the pose before the robot starts turning
    const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
    const auto startHeading = robotPose.GetRotation().GetAngleAroundZaxis();

    DelegateIfInControl(action,
      [this, startHeading]() {
        ++_dVars.numTurnsCompleted;
        const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
        const auto endHeading = robotPose.GetRotation().GetAngleAroundZaxis();
        const auto angleSwept_deg = (endHeading - startHeading).getDegrees();
        DEV_ASSERT(angleSwept_deg < 0.f, "BehaviorQuickSearchForCube.TransitionToSearchTurn.ShouldTurnClockwise");
        if (std::abs(angleSwept_deg) > 75.f) {
          LOG_WARNING("BehaviorQuickSearchForCube.TransitionToSearchTurn.TurnedTooMuch",
                      "The last turn swept an angle of %.2f deg - may have missed seeing the charger due to the camera's limited FOV",
                              std::abs(angleSwept_deg));
        }
        _dVars.angleSwept_deg += std::abs(angleSwept_deg);
        StartNextTurn();
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindCube::UpdateTargetCube()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  auto* targetCube = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.cubesFilter);
  if(nullptr == targetCube){
    return false;
  }

  _dVars.cubeID = targetCube->GetID();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObservableObject* BehaviorFindCube::GetTargetCube()
{
  return GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.cubeID);
}

} // namespace Vector
} // namespace Anki

