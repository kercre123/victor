/**
 * File: BehaviorPlaceCubeByCharger.cpp
 *
 * Author: Sam Russell
 * Created: 2018-08-08
 *
 * Description: Pick up a cube and place it nearby the charger, "put it away"
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPlaceCubeByCharger.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"

#include "coretech/common/engine/jsonTools.h"

#define SET_STATE(s) do{ \
                          _dVars.state = PlaceCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace {
static const int kMaxPickUpAttempts = 2;
static const int kMaxPutDownAttmepts = 2;

static const Vec3f kCubeRelChargerTargetPos = {20.f, 100.f, 0.f};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaceCubeByCharger::InstanceConfig::InstanceConfig()
: chargerFilter(std::make_unique<BlockWorldFilter>())
, cubesFilter(std::make_unique<BlockWorldFilter>())
{ 
  chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);

  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaceCubeByCharger::DynamicVariables::DynamicVariables()
: state(PlaceCubeState::PickUpCube)
, attemptsAtCurrentAction(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaceCubeByCharger::BehaviorPlaceCubeByCharger(const Json::Value& config)
: ICozmoBehavior(config)
, _iConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  // const char* list[] = {
  // };
  // expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast<BehaviorPickUpCube>(BEHAVIOR_ID(PickupCube),
                                                     BEHAVIOR_CLASS(PickUpCube),
                                                     _iConfig.pickupBehavior);
  DEV_ASSERT(_iConfig.pickupBehavior != nullptr,
             "BehaviorPlaceCubeByCharger.InitBehavior.NullPickupBehavior");

  _iConfig.turnToLastFaceBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(TurnToLastFace));
  DEV_ASSERT(nullptr != _iConfig.turnToLastFaceBehavior,
             "BehaviorPlaceCubeByCharger.InitBehavior.NullFindHomeBehavior");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.pickupBehavior.get());
  delegates.insert(_iConfig.turnToLastFaceBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlaceCubeByCharger::WantsToBeActivatedBehavior() const
{
  // Make sure we're tracking a usable cube target
  return (nullptr != GetBEI().GetCubeInteractionTracker().GetTargetStatus().observableObject);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  TransitionToPickUpCube();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::BehaviorUpdate() 
{
  if(!IsActivated()){
    return;
  }

  if(PlaceCubeState::SearchForCharger == _dVars.state){
    if(RobotKnowsWhereChargerIs()){
      CancelDelegates(false);
      TransitionToPlacingCubeByCharger();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToPickUpCube()
{
  SET_STATE(PickUpCube);
  _dVars.attemptsAtCurrentAction = 1;
  AttemptToPickUpCube();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::AttemptToPickUpCube()
{
  // Attempt to pick up the cube
  const auto cube = GetBEI().GetCubeInteractionTracker().GetTargetStatus().observableObject;
  ObjectID cubeID;
  if(nullptr != cube){
    _dVars.cubeStartPosition = cube->GetPose();
    cubeID = cube->GetID();
  }
  else{
    // CancelSelf
    return;
  }

  _iConfig.pickupBehavior->SetTargetID(cubeID);
  const bool pickupWantsToRun = _iConfig.pickupBehavior->WantsToBeActivated();
  if (pickupWantsToRun) {
    DelegateIfInControl(_iConfig.pickupBehavior.get(),
                        [this]() {
                          if (GetBEI().GetRobotInfo().GetCarryingComponent().IsCarryingObject()) {
                            // Succeeded in picking up cube
                            if(!RobotKnowsWhereChargerIs()){
                              TransitionToSearchForCharger();
                            }
                            else {
                              TransitionToPlacingCubeByCharger();
                            }
                          } 
                          else { 
                            if(++_dVars.attemptsAtCurrentAction > kMaxPickUpAttempts){
                              TransitionToGetOutFailed();
                            }
                            else{
                              AttemptToPickUpCube();
                              PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.RetryPickUpCube","");
                            }
                          }
                        });
  } else {
    // Cancel Self
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToSearchForCharger()
{
  SET_STATE(SearchForCharger);

  const bool isAbsolute = false;
  auto searchAction = new CompoundActionSequential({
    new TurnInPlaceAction(DEG_TO_RAD(120.0f), isAbsolute),
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(60.0f), isAbsolute),
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(-20.0f), isAbsolute),
    new WaitAction(0.25f),
    new TurnInPlaceAction(DEG_TO_RAD(100.0f), isAbsolute),
    new WaitAction(1.0f),
    new TurnInPlaceAction(DEG_TO_RAD(30.0f), isAbsolute)
  });

  DelegateIfInControl(searchAction,
                      [this](){
                        if(RobotKnowsWhereChargerIs()){
                          TransitionToPlacingCubeByCharger();
                        }
                        else{
                          TransitionToPuttingCubeBack();
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToPlacingCubeByCharger()
{
  SET_STATE(PlaceCubeByCharger);
  _dVars.attemptsAtCurrentAction = 1;
  AttemptToPlaceCubeByCharger();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::AttemptToPlaceCubeByCharger()
{
  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorPlaceCubeByCharger.TransitionToPlacingCubeOnGround.NullCharger", "Null charger!");
    return;
  }

  // Place the cube on the ground next to the charger
  Pose3d cubePlacementPose;
  cubePlacementPose.SetTranslation(kCubeRelChargerTargetPos);
  cubePlacementPose.SetParent(charger->GetPose());
  DelegateIfInControl(new PlaceObjectOnGroundAtPoseAction(cubePlacementPose),
                      [this](ActionResult result) {
                        const auto& robotInfo = GetBEI().GetRobotInfo();
                        if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
                          if(++_dVars.attemptsAtCurrentAction > kMaxPutDownAttmepts){
                            auto putDownAction = new PlaceObjectOnGroundAction();
                            DelegateIfInControl(putDownAction,
                                                &BehaviorPlaceCubeByCharger::TransitionToGetOutFailed);
                          }
                          else{
                            AttemptToPlaceCubeByCharger();
                            PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.RetryPlaceCubeByCharger","");
                          }
                        }
                        else {
                          TransitionToGetOutSucceeded();
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToPuttingCubeBack()
{
  SET_STATE(PutCubeBack);
  _dVars.attemptsAtCurrentAction = 1;
  AttemptToPutCubeBack();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::AttemptToPutCubeBack()
{
  // Place the cube on the ground where we picked it up from
  DelegateIfInControl(new PlaceObjectOnGroundAtPoseAction(_dVars.cubeStartPosition),
                      [this](ActionResult result) {
                        const auto& robotInfo = GetBEI().GetRobotInfo();
                        if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
                          if(++_dVars.attemptsAtCurrentAction > kMaxPutDownAttmepts){
                            auto putDownAction = new PlaceObjectOnGroundAction();
                            DelegateIfInControl(putDownAction,
                                                &BehaviorPlaceCubeByCharger::TransitionToGetOutFailed);
                          }
                          else{
                            AttemptToPutCubeBack();
                            PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.RetryPutCubeBack","");
                          }
                        }
                        else{
                          TransitionToGetOutSucceeded();
                        }
                      });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToGetOutFailed()
{
  SET_STATE(GetOutFailed);
  // Turn to face
  if(_iConfig.turnToLastFaceBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.turnToLastFaceBehavior.get(),
      [this](){
        DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PlaceCubeByChargerFail));
      });
  }
  else{
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PlaceCubeByChargerFail));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToGetOutSucceeded()
{
  SET_STATE(GetOutSuccess);

  // Turn to original cube pose
  auto turnToOldCubePoseCallback = [this]()
  {
    if(_iConfig.turnToLastFaceBehavior->WantsToBeActivated()){
      DelegateIfInControl(_iConfig.turnToLastFaceBehavior.get(),
        [this](){
          DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PlaceCubeByChargerSuccess));
        });
    }
    else{
      DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PlaceCubeByChargerSuccess));
    }
  };
  auto turnToOldCubePoseAction = new TurnTowardsPoseAction(_dVars.cubeStartPosition);
  DelegateIfInControl(turnToOldCubePoseAction, turnToOldCubePoseCallback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlaceCubeByCharger::RobotKnowsWhereChargerIs()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.chargerFilter);

  if (charger == nullptr) {
    PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.RobotDoesntKnowWhereChargerIs",
                     "No charger found yet, searching or exiting");
    return false;
  }

  _dVars.chargerID = charger->GetID();
  return true;
}

} // namespace Victor
} // namespace Anki
