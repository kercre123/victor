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
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPutDownBlockAtPose.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPlaceCubeByCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/behaviorFindFaceAndThen.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/faceWorld.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#define LOG_CHANNEL "Behaviors"

#define SET_STATE(s) do{ \
                          _dVars.state = FetchState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace{
// static const float kProbExitInsteadOfPlay = 0.5f;
static const float kTargetDistFromFace_mm = 150.0f;
static const char* kSkipConnectToCubeBehaviorKey = "skipConnectToCubeBehavior";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::InstanceConfig::InstanceConfig()
: cubesFilter(std::make_unique<BlockWorldFilter>())
, chargerFilter(std::make_unique<BlockWorldFilter>())
, skipConnectToCubeBehavior(true) //skip by default so we run it only when user driven
{
  cubesFilter->AddFilterFcn(&BlockWorldFilter::IsLightCubeFilter);

  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::DynamicVariables::DynamicVariables()
: state(FetchState::GetIn)
, cubeID()
, attemptsAtCurrentAction(0)
, startedOnCharger(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFetchCube::BehaviorFetchCube(const Json::Value& config)
 : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kSkipConnectToCubeBehaviorKey, _iConfig.skipConnectToCubeBehavior);
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
  modifiers.visionModesForActiveScope->insert({ VisionMode::Markers, EVisionUpdateFrequency::High });
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.wantsToBeActivatedWhenCarryingObject = false; 
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.reactToCliffBehavior.get());
  delegates.insert(_iConfig.driveOffChargerBehavior.get());
  delegates.insert(_iConfig.lookForUserBehavior.get());
  delegates.insert(_iConfig.findCubeBehavior.get());
  delegates.insert(_iConfig.putCubeByChargerBehavior.get());
  delegates.insert(_iConfig.putCubeSomewhereBehavior.get());
  delegates.insert(_iConfig.connectToCubeBehavior.get());
  delegates.insert(_iConfig.pickUpCubeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kSkipConnectToCubeBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorFindFaceAndThen>(BEHAVIOR_ID(FindFacesFetchCube),
                                                          BEHAVIOR_CLASS(FindFaceAndThen),
                                                          _iConfig.lookForUserBehavior);
  DEV_ASSERT(nullptr != _iConfig.lookForUserBehavior,
             "BehaviorFetchCube.InitBehavior.NullLookForUserBehavior");

  BC.FindBehaviorByIDAndDowncast<BehaviorFindCube>(BEHAVIOR_ID(FindCube),
                                                   BEHAVIOR_CLASS(FindCube),
                                                   _iConfig.findCubeBehavior);
  DEV_ASSERT(nullptr != _iConfig.findCubeBehavior,
             "BehaviorFetchCube.InitBehavior.NullFindCubeBehavior");

  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(BEHAVIOR_ID(PickupCubeNoInitialReaction),
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickUpCubeBehavior);
  DEV_ASSERT(_iConfig.pickUpCubeBehavior != nullptr,
             "BehaviorFetchCube.InitBehavior.NullPickupBehavior");

  BC.FindBehaviorByIDAndDowncast<BehaviorPutDownBlockAtPose>(BEHAVIOR_ID(PutDownBlockAtPose),
                                                            BEHAVIOR_CLASS(PutDownBlockAtPose),
                                                            _iConfig.putCubeSomewhereBehavior);
  DEV_ASSERT(_iConfig.putCubeSomewhereBehavior != nullptr,
             "BehaviorFetchCube.InitBehavior.NullPutCubeSomewhereBehavior");
        
  _iConfig.reactToCliffBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ReactToCliffDuringFetch));
  DEV_ASSERT(nullptr != _iConfig.reactToCliffBehavior,
             "BehaviorFetchCube.InitBehavior.NullReactToCliffBehavior");

  _iConfig.driveOffChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(DriveOffChargerFace));
  DEV_ASSERT(nullptr != _iConfig.driveOffChargerBehavior,
             "BehaviorFetchCube.InitBehavior.NullDriveOffChargerBehavior");

  _iConfig.connectToCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(ConnectToCube));
  DEV_ASSERT(nullptr != _iConfig.connectToCubeBehavior,
             "BehaviorFetchCube.InitBehavior.NullConnectToCubeBehavior");


  _iConfig.putCubeByChargerBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(PlaceCubeByCharger));
  DEV_ASSERT(nullptr != _iConfig.putCubeByChargerBehavior,
             "BehaviorFetchCube.InitBehavior.NullPutCubeByChargerBehavior");


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

  if(!_iConfig.reactToCliffBehavior->IsActivated() &&
      _iConfig.reactToCliffBehavior->WantsToBeActivated()){
    LOG_INFO("BehaviorFetchCube.CliffDetected", "Delegating to reactToCliffBehavior");
    CancelDelegates(false);
    DelegateIfInControl(_iConfig.reactToCliffBehavior.get(),
      [this](){
        // Handle resume after cliff encounter
        switch(_dVars.state){
          case FetchState::GetIn:
          case FetchState::DriveOffCharger:
          case FetchState::LookForUser:
          case FetchState::FindCube:
          case FetchState::PutCubeDownHere:
          case FetchState::GetOutSuccess:
          case FetchState::GetOutFailure:
          {
            // exit the behavior
            break;
          }
          case FetchState::AttemptConnection:
          case FetchState::PickUpCube:
          {
            TransitionToPickUpCube();
            break;
          }
          case FetchState::TakeCubeSomewhere:
          {
            TransitionToPutCubeDownHere();
            break;
          }
          default:
          {
            LOG_ERROR("BehaviorFetchCube.ResumeAfterCliff.UnhandledState",
                      "Attempted to handle invalid FetchState");
          }
        }

      });
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
    LOG_ERROR("BehaviorFetchCube.TransitionError",
              "Behavior %s did not want to be activated",
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
        TransitionToFindCube();
      });
  }
  else{
    LOG_ERROR("BehaviorFetchCube.TransitionError",
              "Behavior %s did not want to be activated",
              _iConfig.lookForUserBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToFindCube()
{
  SET_STATE(FindCube);

  if(_iConfig.findCubeBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.findCubeBehavior.get(),
      [this](){
        _dVars.cubeID = _iConfig.findCubeBehavior->GetFoundCubeID();
        auto* targetCube = GetTargetCube();
        if(nullptr != targetCube){
          TransitionToAttemptConnection();
        } //else just exit, the get-out will have been handled by the findCubeBehavior since it didn't find anything
      });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToAttemptConnection()
{
  SET_STATE(AttemptConnection);

  if(_iConfig.skipConnectToCubeBehavior){
    auto& ccc = GetBEI().GetCubeConnectionCoordinator();
    ccc.SubscribeToCubeConnection(this);
    TransitionToPickUpCube();
  }
  else{
    if(_iConfig.connectToCubeBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.connectToCubeBehavior.get(),
        [this](){
          auto& ccc = GetBEI().GetCubeConnectionCoordinator();
          if(ccc.IsConnectedToCube()){
            // Convert the background subscription to foreground
            ccc.SubscribeToCubeConnection(this);
          }

          TransitionToPickUpCube();
        });
    }
    else{
      LOG_ERROR("BehaviorFetchCube.TransitionError",
                "Behavior %s did not want to be activated",
                _iConfig.connectToCubeBehavior->GetDebugLabel().c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToPickUpCube()
{
  SET_STATE(PickUpCube);

  // Attempt to pick up the cube
  if(nullptr == GetTargetCube()){
    // CancelSelf
    LOG_ERROR("BehaviorFetchCube.PickUpCube.NoCubeFound",
              "No cube was available, should not have made it to this state of the behavior");
    return;
  }

  _iConfig.pickUpCubeBehavior->SetTargetID(_dVars.cubeID);
  if (_iConfig.pickUpCubeBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_iConfig.pickUpCubeBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            TransitionToTakeCubeSomewhere();
                          } 
                          else { 
                            TransitionToGetOutFailure();
                          }
                        });
  } else {
    LOG_ERROR("BehaviorFetchCube.TransitionError",
              "Behavior %s did not want to be activated",
              _iConfig.pickUpCubeBehavior->GetDebugLabel().c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToTakeCubeSomewhere()
{
  SET_STATE(TakeCubeSomewhere);
  _dVars.attemptsAtCurrentAction = 1;

  if(ComputeFaceBasedTargetPose()){
    LOG_DEBUG("BehaviorFetchCube.TakeCubeToUser","");
    _iConfig.putCubeSomewhereBehavior->SetDestinationPose(_dVars.destination);
    _iConfig.putCubeSomewhereBehavior->SetPutDownAnimTrigger(AnimationTrigger::FetchCubeSetDown);
    if(_iConfig.putCubeSomewhereBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.putCubeSomewhereBehavior.get(), &BehaviorFetchCube::TransitionToGetOutSuccess);
      return;
    }
  }
  else if(_dVars.startedOnCharger){
    LOG_DEBUG("BehaviorFetchCube.TakeCubeToCharger","");
    if(_iConfig.putCubeByChargerBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.putCubeByChargerBehavior.get());
      return;
    }
    LOG_ERROR("BehaviorFetchCube.TakeCubeToCharger.DidNotActivate",
              "BehaviorPlaceCubeByCharger did not want to be activated");
  }
  else{
    _dVars.destination = _dVars.poseAtStartOfBehavior;
    _iConfig.putCubeSomewhereBehavior->SetDestinationPose(_dVars.destination);
    _iConfig.putCubeSomewhereBehavior->SetPutDownAnimTrigger(AnimationTrigger::FetchCubeSetDown);
    if(_iConfig.putCubeSomewhereBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.putCubeSomewhereBehavior.get());
      return;
    }
  }

  // Nothing worked, drop it like its hot...
  DelegateIfInControl(new PlaceObjectOnGroundAction(), &BehaviorFetchCube::TransitionToGetOutFailure);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToPutCubeDownHere()
{
  SET_STATE(PutCubeDownHere);
  _iConfig.putCubeSomewhereBehavior->SetPutDownAnimTrigger(AnimationTrigger::FetchCubeSetDown);
  DelegateIfInControl(_iConfig.putCubeSomewhereBehavior.get(), &BehaviorFetchCube::TransitionToGetOutSuccess);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToGetOutSuccess()
{
  SET_STATE(GetOutSuccess);

  // Look at the user, celebrate lightly, exit
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeSuccess));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFetchCube::TransitionToGetOutFailure()
{
  SET_STATE(GetOutFailure);
  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::FetchCubeFailure));
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

  float distanceToProjectedPose = 0.f;
  if (!ComputeDistanceBetween(projectedFacePose, robotPose, distanceToProjectedPose)) {
    return false;
  }
  float fwdTranslation = distanceToProjectedPose - kTargetDistFromFace_mm;
  Radians angleRobotFwdToFace(atan2f(projectedFacePose.GetTranslation().y(), projectedFacePose.GetTranslation().x()));

  Pose3d destinationPose;
  destinationPose.SetParent(robotPose);
  destinationPose.SetRotation(angleRobotFwdToFace, Z_AXIS_3D());
  destinationPose.TranslateForward(fwdTranslation);
  _dVars.destination = destinationPose.GetWithRespectToRoot();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObservableObject* BehaviorFetchCube::GetTargetCube()
{
  return GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.cubeID);
}

} // namespace Vector
} // namespace Anki
