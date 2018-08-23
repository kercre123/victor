/**
 * File: BehaviorFindCubeAndThen.cpp
 *
 * Author: Sam Russell
 * Created: 2018-08-20
 *
 * Description: If Vector thinks he know's where a cube is, visually verify that its there. Otherwise, 
 *   search quickly for a cube. Go to the pre-dock pose if found/known, delegate to next behavior if specified
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCubeAndThen.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/components/sensors/proxSensorComponent.h"

#include "coretech/common/engine/utils/timer.h"

#define SET_STATE(s) do{ \
                          _dVars.state = FindCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
const char* const kFollowUpBehaviorKey = "followUpBehaviorID";

static const float kDistThreshold_mm = 2.0f;
static const float kAngleThreshold_deg = 2.0f;
static const float kProxBackupThreshold_mm = 50.0f;
static const float kMinSearchAngle = 300.0f;
static const int   kMaxNumSearchTurns = 10;
static const int   kNumImagesToWaitDuringSearch = 2;
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::InstanceConfig::InstanceConfig()
: cubesFilter(std::make_unique<BlockWorldFilter>())
{
  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::DynamicVariables::DynamicVariables()
: state(FindCubeState::GetIn)
, cubePtr(nullptr)
, cubeState(CubeObservationState::Unreliable)
, lastPoseCheckTimestamp(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::BehaviorFindCubeAndThen(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kFollowUpBehaviorKey, _iConfig.followUpBehaviorID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindCubeAndThen::~BehaviorFindCubeAndThen()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindCubeAndThen::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = false; 
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.connectToCubeBehavior.get());
  if(nullptr != _iConfig.followUpBehavior){
    delegates.insert(_iConfig.followUpBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kFollowUpBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerCube));
  DEV_ASSERT(nullptr != _iConfig.driveOffChargerBehavior,
             "BehaviorFindCubeAndThen.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.connectToCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ConnectToCube));
  DEV_ASSERT(nullptr != _iConfig.connectToCubeBehavior,
             "BehaviorFindCubeAndThen.InitBehavior.NullConnectToCubeBehavior");

  if(!_iConfig.followUpBehaviorID.empty()){
    _iConfig.followUpBehavior = FindBehavior(_iConfig.followUpBehaviorID);
    DEV_ASSERT(nullptr != _iConfig.followUpBehavior,
              "BehaviorFindCubeAndThen.InitBehavior.NullFollowUpBehavior");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::OnBehaviorActivated() 
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
void BehaviorFindCubeAndThen::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }


  if(FindCubeState::QuickSearchForCube == _dVars.state){
    auto& proxSensor = GetBEI().GetRobotInfo().GetProxSensorComponent();
    uint16_t proxDist_mm = 0;
    if( proxSensor.GetLatestDistance_mm(proxDist_mm) && ( proxDist_mm < kProxBackupThreshold_mm ) ){
      BackUpAndCheck();
    }

    // If we don't have a target yet, check if a cube has been seen in the last frame 
    if(nullptr == _dVars.cubePtr){
      UpdateTargetCube();
      // If we just got a target for the first time, move on to approaching it
      if(nullptr != _dVars.cubePtr){
        CancelDelegates(false);
        TransitionToReactToCube();
      }
    }
    else{
      // Otherwise, we already had a target but it wasn't found where expected during VisuallyVerify.
      // Watch for new observations and pose updates
      if(_dVars.cubePtr->GetLastObservedTime() > _dVars.lastPoseCheckTimestamp){
        _dVars.lastPoseCheckTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
        Pose3d cubePose = _dVars.cubePtr->GetPose();
        if( !cubePose.IsSameAs(_dVars.cubePoseAtSearchStart, kDistThreshold_mm, DEG_TO_RAD(kAngleThreshold_deg)) ){
          CancelDelegates(false);
          TransitionToReactToCube();
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);

  if(_iConfig.driveOffChargerBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(),
                        &BehaviorFindCubeAndThen::TransitionToVisuallyCheckLastKnown);
  }
  else{
    PRINT_NAMED_ERROR("BehaviorFindCubeAndThen.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.driveOffChargerBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToVisuallyCheckLastKnown()
{
  SET_STATE(VisuallyCheckLastKnown);

  if(UpdateTargetCube()){
    bool visuallyVerify = true;
    TurnTowardsObjectAction* visualCheckAction = new TurnTowardsObjectAction(_dVars.cubePtr->GetID(),
                                                                             Radians{M_PI_F},
                                                                             visuallyVerify);
    DelegateIfInControl(visualCheckAction, [this](ActionResult result){
      if(ActionResult::SUCCESS == result){
        TransitionToReactToCube();
      }
      else{
        TransitionToQuickSearchForCube();
      }
    });
  }
  else{
    TransitionToQuickSearchForCube();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToQuickSearchForCube()
{
  SET_STATE(QuickSearchForCube);

  if(nullptr != _dVars.cubePtr){
    _dVars.cubePoseAtSearchStart = _dVars.cubePtr->GetPose();
    _dVars.lastPoseCheckTimestamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  }
  StartNextTurn();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToReactToCube()
{
  SET_STATE(ReactToCube);

  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FindCubeReactToCube),
                      &BehaviorFindCubeAndThen::TransitionToDriveToPredockPose);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToDriveToPredockPose()
{
  SET_STATE(DriveToPredockPose);

  DelegateIfInControl(new DriveToObjectAction(_dVars.cubePtr->GetID(), PreActionPose::ActionType::DOCKING),
                      &BehaviorFindCubeAndThen::TransitionToAttemptConnection);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToAttemptConnection()
{
  SET_STATE(AttemptConnection);

  if(_iConfig.connectToCubeBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.connectToCubeBehavior.get(),
      [this](){
        auto& ccc = GetBEI().GetCubeConnectionCoordinator();
        if(ccc.IsConnectedToCube()){
          // Convert the background subscription to foreground
          ccc.SubscribeToCubeConnection(this);
        }

        TransitionToFollowUpBehavior();
      });
  }
  else{
    PRINT_NAMED_ERROR("BehaviorFindCubeAndThen.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.connectToCubeBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToFollowUpBehavior()
{
  SET_STATE(FollowUpBehavior);
  if(nullptr != _iConfig.followUpBehavior && _iConfig.followUpBehavior->WantsToBeActivated()){
    // Let the behavior end when the followup does
    DelegateIfInControl(_iConfig.followUpBehavior.get());
  }
  // If there is no followUp behavior, end now
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::TransitionToGetOutFailure()
{
  SET_STATE(GetOutFailure);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFailure));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::StartNextTurn()
{
  // Have we turned enough to sweep the required angle?
  // Or have we exceeded the max number of turns?
  const bool sweptEnough = (_dVars.angleSwept_deg > kMinSearchAngle);
  const bool exceededMaxTurns = (_dVars.numTurnsCompleted >= kMaxNumSearchTurns);

  if (sweptEnough || exceededMaxTurns) {
    // If we made it all the way through the search without finding anything, call it a failure
    TransitionToGetOutFailure();
    PRINT_CH_INFO("Behaviors", "BehaviorFindHome.TransitionToSearchTurn.CompletedQuickSearch",
                  "We have completed a full search. Played %d turn animations (max is %d), and swept an angle of %.2f deg (min required sweep angle %.2f deg)",
                  _dVars.numTurnsCompleted,
                  kMaxNumSearchTurns,
                  _dVars.angleSwept_deg,
                  kMinSearchAngle);
  } else {
    auto* action = new CompoundActionSequential();

    // Always do the "search turn"
    action->AddAction(new TriggerAnimationAction(AnimationTrigger::FindCubeTurns/*ChargerDockingSearchSingleTurn*/));

    auto* loopAndWaitAction = new CompoundActionParallel();
    loopAndWaitAction->AddAction(new TriggerAnimationAction(AnimationTrigger::FindCubeWaitLoop/*ChargerDockingSearchWaitForImages*/));
    loopAndWaitAction->AddAction(new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::DetectingMarkers));
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
          PRINT_NAMED_WARNING("BehaviorQuickSearchForCube.TransitionToSearchTurn.TurnedTooMuch",
                              "The last turn swept an angle of %.2f deg - may have missed seeing the charger due to the camera's limited FOV",
                              std::abs(angleSwept_deg));
        }
        _dVars.angleSwept_deg += std::abs(angleSwept_deg);
        StartNextTurn();
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindCubeAndThen::BackUpAndCheck()
{
  CancelDelegates(false);

  CompoundActionSequential* backupAndCheckAction = new CompoundActionSequential();
  backupAndCheckAction->AddAction(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToObstacle));

  auto* loopAndWaitAction = new CompoundActionParallel();
  loopAndWaitAction->AddAction(new TriggerAnimationAction(AnimationTrigger::ChargerDockingSearchWaitForImages));
  loopAndWaitAction->AddAction(new WaitForImagesAction(kNumImagesToWaitDuringSearch, VisionMode::DetectingMarkers));
  backupAndCheckAction->AddAction(loopAndWaitAction);

  DelegateIfInControl(backupAndCheckAction, &BehaviorFindCubeAndThen::StartNextTurn);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindCubeAndThen::UpdateTargetCube()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  _dVars.cubePtr = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.cubesFilter);
  if(nullptr == _dVars.cubePtr){
    return false;
  }

  return true;
}

} // namespace Vector
} // namespace Anki
