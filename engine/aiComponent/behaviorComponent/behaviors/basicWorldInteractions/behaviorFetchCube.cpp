/**
 * File: BehaviorFetchCube.cpp
 *
 * Author: Sam Russell
 * Created: 2018-08-10
 *
 * Description: Search for a cube(if not known), go get it, bring it to the face who sent you
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFetchCube.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorFindFaceAndThen.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/utils/timer.h"

#define SET_STATE(s) do{ \
                          _dVars.state = FetchState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
static const float kMaxExtensiveSearchTime_s = 20.0f;
static const float kMaxExasperatedSearchTime_s = 20.0f;

static const int kMaxPickUpAttempts = 2;
static const int kMaxDeliveryAttempts = 2;

static const float kProbTakeToStartIfChargerKnown = 0.25f; 
static const float kProbExitInsteadOfPlay = 0.5f;

static const Vec3f kCubeRelChargerTargetPos = {20.f, 100.f, 0.f};
static const float kTargetDistFromFace_mm = 150.0f;

static const bool kUseTimeStampInTargetCheck = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::InstanceConfig::InstanceConfig()
: cubesFilter(std::make_unique<BlockWorldFilter>())
, chargerFilter(std::make_unique<BlockWorldFilter>())
{
  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);

  chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::DynamicVariables::DynamicVariables()
: state(FetchState::GetIn)
, cubePtr(nullptr)
, attemptsAtCurrentAction(0)
, stopSearchTime_s(0.0f)
, startedOnCharger(false)
, searchStartTimeStamp(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::BehaviorFetchCube(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::~BehaviorFetchCube()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFetchCube::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers, EVisionUpdateFrequency::High });
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = false; 
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.pickUpCubeBehavior.get());
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.connectToCubeBehavior.get());
  delegates.insert(_iConfig.lookForUserBehavior.get());
  delegates.insert(_iConfig.lookBackAtUserBehavior.get());
  delegates.insert(_iConfig.extensiveSearchForCubeBehavior.get());
  delegates.insert(_iConfig.playWithCubeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(BEHAVIOR_ID(PickupCube),
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickUpCubeBehavior);
  DEV_ASSERT(_iConfig.pickUpCubeBehavior != nullptr,
             "BehaviorFetchCube.InitBehavior.NullPickupBehavior");

  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerFace));
  DEV_ASSERT(nullptr != _iConfig.driveOffChargerBehavior,
             "BehaviorFetchCube.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.connectToCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ConnectToCube));
  DEV_ASSERT(nullptr != _iConfig.connectToCubeBehavior,
             "BehaviorFetchCube.InitBehavior.NullConnectToCubeBehavior");

  BC.FindBehaviorByIDAndDowncast<BehaviorFindFaceAndThen>(BEHAVIOR_ID(FindFacesFetchCube),
                                                          BEHAVIOR_CLASS(FindFaceAndThen),
                                                          _iConfig.lookForUserBehavior);
  DEV_ASSERT(nullptr != _iConfig.lookForUserBehavior,
             "BehaviorFetchCube.InitBehavior.NullLookForUserBehavior");

  _iConfig.lookBackAtUserBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(TurnToLastFace));
  DEV_ASSERT(nullptr != _iConfig.lookBackAtUserBehavior,
             "BehaviorFetchCube.InitBehavior.NullLookBackAtUserBehavior");

  _iConfig.extensiveSearchForCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ShortLookAroundForFaceAndCube));
  DEV_ASSERT(nullptr != _iConfig.extensiveSearchForCubeBehavior,
             "BehaviorFetchCube.InitBehavior.NullExtensiveSearchForCubeBehavior");

  _iConfig.playWithCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(InteractWithStaticCube));
  DEV_ASSERT(nullptr != _iConfig.playWithCubeBehavior,
             "BehaviorFetchCube.InitBehavior.NullExtensiveSearchForCubeBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  const bool currOnCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
  const bool currOnChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if(currOnCharger || currOnChargerPlatform) {
    TransitionToDriveOffCharger();
    _dVars.startedOnCharger = true;
  }
  else {
    _dVars.poseAtStartOfBehavior = GetBEI().GetRobotInfo().GetPose();
    TransitionToLookForUser();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }

  if(FetchState::QuickSearchForCube == _dVars.state){
    auto searchDuration = GetBEI().GetRobotInfo().GetLastMsgTimestamp() - _dVars.searchStartTimeStamp;
    if(CheckForCube(kUseTimeStampInTargetCheck, searchDuration)){
      CancelDelegates(false);
      TransitionToPickUpCube();
    }
  }

  if(FetchState::ExtensiveSearchForCube == _dVars.state){
    auto searchDuration = GetBEI().GetRobotInfo().GetLastMsgTimestamp() - _dVars.searchStartTimeStamp;
    if(CheckForCube(kUseTimeStampInTargetCheck, searchDuration)){
      CancelDelegates(false);
      TransitionToPickUpCube();
    }
    else if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _dVars.stopSearchTime_s){
      CancelDelegates(false);
      TransitionToLookToUserForHelp();
    }
  }

  if(FetchState::ExasperatedSearchForCube == _dVars.state){
    auto searchDuration = GetBEI().GetRobotInfo().GetLastMsgTimestamp() - _dVars.searchStartTimeStamp;
    if(CheckForCube(kUseTimeStampInTargetCheck, searchDuration)){
      CancelDelegates(false);
      TransitionToPickUpCube();
    }
    else if(BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() > _dVars.stopSearchTime_s){
      CancelDelegates(false);
      TransitionToGetOutFailure();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToDriveOffCharger()
{
  SET_STATE(DriveOffCharger);

  if(_iConfig.driveOffChargerBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.driveOffChargerBehavior.get(),
                        &BehaviorFetchCube::TransitionToLookForUser);
  }
  else{
    PRINT_NAMED_ERROR("BehaviorFetchCube.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.driveOffChargerBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToLookForUser()
{
  SET_STATE(LookForUser);

  if(_iConfig.lookForUserBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.lookForUserBehavior.get(),
      [this](){
        _dVars.targetFace = _iConfig.lookForUserBehavior->GetFoundFace();
        TransitionToAttemptConnection();
      });
  }
  else{
    PRINT_NAMED_ERROR("BehaviorFetchCube.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.lookForUserBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToAttemptConnection()
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

        TransitionToVisuallyCheckLastKnown();
      });
  }
  else{
    PRINT_NAMED_ERROR("BehaviorFetchCube.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.connectToCubeBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToVisuallyCheckLastKnown()
{
  SET_STATE(VisuallyCheckLastKnown);

  if(CheckForCube()){
    bool visuallyVerify = true;
    TurnTowardsObjectAction* visualCheckAction = new TurnTowardsObjectAction(_dVars.cubePtr->GetID(),
                                                                             Radians{M_PI_F},
                                                                             visuallyVerify);
    DelegateIfInControl(visualCheckAction, [this](ActionResult result){
      if(ActionResult::SUCCESS == result){
        TransitionToPickUpCube();
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
void BehaviorFetchCube::TransitionToQuickSearchForCube()
{
  SET_STATE(QuickSearchForCube);

  // TODO:(str) placeholder quick search action
  const bool isAbsolute = false;
  auto searchAction = new CompoundActionSequential({
    new MoveHeadToAngleAction(DEG_TO_RAD(0.0f)),
    new WaitAction(0.25f),
    new TurnInPlaceAction(DEG_TO_RAD(-30), isAbsolute), // -30 deg
    new WaitAction(0.25f),
    new TurnInPlaceAction(DEG_TO_RAD(60), isAbsolute), // 30 deg
    new WaitAction(0.25f),
    new TurnInPlaceAction(DEG_TO_RAD(50.0f), isAbsolute), // 80 deg
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(100.0f), isAbsolute), //180 deg
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(-20.0f), isAbsolute), //160 deg
    new WaitAction(0.25f),
    new TurnInPlaceAction(DEG_TO_RAD(120.0f), isAbsolute), // 260 deg
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(-30.0f), isAbsolute), // 230 deg
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(130.0f), isAbsolute), // 360 deg
  });

  _dVars.searchStartTimeStamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
  DelegateIfInControl(searchAction,
    [this](){
      auto searchDuration = GetBEI().GetRobotInfo().GetLastMsgTimestamp() - _dVars.searchStartTimeStamp;
      if(CheckForCube(kUseTimeStampInTargetCheck, searchDuration)){
        TransitionToPickUpCube();
      }
      else{
        TransitionToExtensiveSearchForCube();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToExtensiveSearchForCube()
{
  SET_STATE(ExtensiveSearchForCube);

  // TODO:(str) sort out a more defined behavior here
  if(_iConfig.extensiveSearchForCubeBehavior->WantsToBeActivated()){
    _dVars.stopSearchTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kMaxExtensiveSearchTime_s;
    _dVars.searchStartTimeStamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
    DelegateIfInControl(_iConfig.extensiveSearchForCubeBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToLookToUserForHelp()
{
  SET_STATE(LookToUserForHelp);

  if(_iConfig.lookBackAtUserBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.lookBackAtUserBehavior.get(),
      [this](){
        DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFrustrated),
                            &BehaviorFetchCube::TransitionToExasperatedSearchForCube);
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToExasperatedSearchForCube()
{
  SET_STATE(ExasperatedSearchForCube);

  // TODO:(str) sort out a more defined behavior here
  if(_iConfig.extensiveSearchForCubeBehavior->WantsToBeActivated()){
    _dVars.stopSearchTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kMaxExasperatedSearchTime_s;
    _dVars.searchStartTimeStamp = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
    DelegateIfInControl(_iConfig.extensiveSearchForCubeBehavior.get());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToPickUpCube()
{
  SET_STATE(PickUpCube);
  _dVars.attemptsAtCurrentAction = 1;
  AttemptToPickUpCube();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::AttemptToPickUpCube()
{
  // Attempt to pick up the cube
  ObjectID cubeID;
  if(CheckForCube()){
    cubeID = _dVars.cubePtr->GetID();
  }
  else{
    // CancelSelf
    PRINT_NAMED_ERROR("BehaviorFetchCube.AttemptToPickUpCube.NoCubeFound",
                      "No cube was available, should not have made it to this state of the behavior");
    return;
  }

  _iConfig.pickUpCubeBehavior->SetTargetID(cubeID);
  const bool pickupWantsToRun = _iConfig.pickUpCubeBehavior->WantsToBeActivated();
  if (pickupWantsToRun) {
    DelegateIfInControl(_iConfig.pickUpCubeBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            TransitionToTakeCubeSomewhere();
                          } 
                          else { 
                            if(++_dVars.attemptsAtCurrentAction > kMaxPickUpAttempts){
                              TransitionToGetOutFailure();
                            }
                            else{
                              AttemptToPickUpCube();
                              PRINT_NAMED_INFO("BehaviorFetchCube.RetryPickUpCube","");
                            }
                          }
                        });
  } else {
    PRINT_NAMED_ERROR("BehaviorFetchCube.TransitionError", "Behavior %s did not want to be activated",
                      _iConfig.pickUpCubeBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToTakeCubeSomewhere()
{
  SET_STATE(TakeCubeSomewhere);
  _dVars.attemptsAtCurrentAction = 1;

  if(ComputeFaceBasedTargetPose()){
    PRINT_NAMED_INFO("BehaviorFetchCube.TakeCubeToUser","");
  }
  else{
    auto& rng = GetBEI().GetRobotInfo().GetRNG();
    bool goToCharger = (rng.RandDbl(1.0f) > kProbTakeToStartIfChargerKnown) || _dVars.startedOnCharger;
    bool goToStart = !_dVars.startedOnCharger;

    const ObservableObject* charger = nullptr;
    if(goToCharger){
      // Verify that the charger is actually an option
      const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
      charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.chargerFilter);
      goToCharger = (nullptr != charger);
    }

    if(goToCharger){
      Pose3d cubePlacementPose;
      cubePlacementPose.SetTranslation(kCubeRelChargerTargetPos);
      cubePlacementPose.SetParent(charger->GetPose());
      _dVars.destination = cubePlacementPose;
      PRINT_NAMED_INFO("BehaviorFetchCube.TakeCubeToCharger","");
    }
    else if(goToStart){
      _dVars.destination = _dVars.poseAtStartOfBehavior;
      PRINT_NAMED_INFO("BehaviorFetchCube.TakeCubeToStartLocale","");
    }
    else{
      // Charger was unavailable... and we don't have a startPose
      // Drop it like its hot...
      DelegateIfInControl(new PlaceObjectOnGroundAction(),
                          &BehaviorFetchCube::TransitionToGetOutFailure);
      return;
    }
  }

  AttemptToTakeCubeSomewhere();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::AttemptToTakeCubeSomewhere()
{
  // Place the cube at the destination
  DelegateIfInControl(new PlaceObjectOnGroundAtPoseAction(_dVars.destination),
                      [this](ActionResult result) {
                        const auto& robotInfo = GetBEI().GetRobotInfo();
                        if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
                          if(++_dVars.attemptsAtCurrentAction > kMaxDeliveryAttempts){
                            auto putDownAction = new PlaceObjectOnGroundAction();
                            DelegateIfInControl(putDownAction,
                                                &BehaviorFetchCube::TransitionToPlayWithCube);
                          }
                          else{
                            AttemptToTakeCubeSomewhere();
                            PRINT_NAMED_INFO("BehaviorFetchCube.RetryTakeCubeSomewhere","");
                          }
                        }
                        else {
                          auto& rng = GetBEI().GetRobotInfo().GetRNG();
                          if(rng.RandDbl(1.0f) > kProbExitInsteadOfPlay){
                            TransitionToPlayWithCube();
                          }
                          else {
                            TransitionToGetOutSuccess();
                          }
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToPlayWithCube()
{
  SET_STATE(PlayWithCube);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeSuccess),
    [this](){
      if(_iConfig.playWithCubeBehavior->WantsToBeActivated()){
        // Let the behavior end after playing
        DelegateIfInControl(_iConfig.playWithCubeBehavior.get());
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToGetOutSuccess()
{
  SET_STATE(GetOutSuccess);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeSuccess));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToGetOutFailure()
{
  SET_STATE(GetOutFailure);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFailure));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFetchCube::CheckForCube(bool useTimeStamp, RobotTimeStamp_t maxTimeSinceSeen_ms)
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  _dVars.cubePtr = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.cubesFilter);
  if(nullptr == _dVars.cubePtr){
    return false;
  }

  if(!useTimeStamp){
    return true;
  }
  else{
    const auto lastSeen_ms = _dVars.cubePtr->GetLastObservedTime();
    const auto currentTime_ms = GetBEI().GetRobotInfo().GetLastMsgTimestamp();
    return (currentTime_ms - maxTimeSinceSeen_ms) < lastSeen_ms;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFetchCube::ComputeFaceBasedTargetPose()
{
  // Get Face from face world
  const Vision::TrackedFace* facePtr = GetBEI().GetFaceWorld().GetFace(_dVars.targetFace);
  if(facePtr == nullptr){
    return false;
  }

  Pose3d robotPose = GetBEI().GetRobotInfo().GetPose();
  Pose3d facePose = facePtr->GetHeadPose();

  Pose3d projectedFacePose;
  if(!facePose.GetWithRespectTo(robotPose, projectedFacePose)){
    return false;
  }

  // Create a target pose pointing at the face, on the ground between the robot and the face
  projectedFacePose.SetTranslation({ projectedFacePose.GetTranslation().x(),
                                     projectedFacePose.GetTranslation().y(),
                                     robotPose.GetTranslation().z() });

  float distanceToProjectedPose = ComputeDistanceBetween(projectedFacePose, robotPose);
  float fwdTranslation = distanceToProjectedPose - kTargetDistFromFace_mm;
  Radians angleRobotFwdToFace(atan2f(projectedFacePose.GetTranslation().y(), projectedFacePose.GetTranslation().x()));

  Pose3d destinationPose;
  destinationPose.SetParent(robotPose);
  destinationPose.SetRotation(angleRobotFwdToFace, Z_AXIS_3D());
  destinationPose.TranslateForward(fwdTranslation);
  _dVars.destination = destinationPose.GetWithRespectToRoot();

  return true;
}

} // namespace Vector
} // namespace Anki
