/**
 * File: BehaviorPlaceCubeByCharger.cpp
 *
 * Author: Sam Russell
 * Created: 2018-08-08
 *
 * Description: Pick up a cube and place it nearby the charger, "put it away". If the charger can't be found,
 *              just put the cube somewhere else nearby.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPlaceCubeByCharger.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicCubeInteractions/behaviorPickUpCube.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/cubes/cubeInteractionTracker.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "engine/utils/robotPointSamplerHelper.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/math/polygon_impl.h"

#include "util/random/randomGenerator.h"
#include "util/random/randomIndexSampler.h"
#include "util/random/rejectionSamplerHelper.h"

#define SET_STATE(s) do{ \
                          _dVars.state = PlaceCubeState::s; \
                          SetDebugStateName(#s); \
                        } while(0);

namespace Anki {
namespace Vector {

namespace {
static const float kMinSearchAngle = 300.0f;
static const int   kMaxNumSearchTurns = 10;
static const int   kNumImagesToWaitDuringSearch = 2;

static const int kMaxPickUpAttempts = 2;

static const Vec3f kCubeRelChargerTargetPos = {-20.f, 100.f, 0.f};

static const float kFinalHeadAngle_deg = -20.0f;
static const float kVerifyBackupDist_mm = -30.0f;

const float kMinDrivingDist_mm = 150.0f;
const float kMaxDrivingDist_mm = 400.0f;
const float kMinCliffPenaltyDist_mm = 100.0f;
const float kMaxCliffPenaltyDist_mm = 600.0f;

constexpr MemoryMapTypes::FullContentArray kTypesToBlockSampling =
{
  {MemoryMapTypes::EContentType::Unknown               , false},
  {MemoryMapTypes::EContentType::ClearOfObstacle       , false},
  {MemoryMapTypes::EContentType::ClearOfCliff          , false},
  {MemoryMapTypes::EContentType::ObstacleObservable    , true },
  {MemoryMapTypes::EContentType::ObstacleProx          , true },
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , true },
  {MemoryMapTypes::EContentType::Cliff                 , true },
  {MemoryMapTypes::EContentType::InterestingEdge       , true },
  {MemoryMapTypes::EContentType::NotInterestingEdge    , true }
};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaceCubeByCharger::InstanceConfig::InstanceConfig()
: chargerFilter(std::make_unique<BlockWorldFilter>())
, cubesFilter(std::make_unique<BlockWorldFilter>())
, turnToUserBeforeSuccessReaction(false)
, turnToUserAfterSuccessReaction(false)
{ 
  chargerFilter->AddAllowedFamily(ObjectFamily::Charger);
  chargerFilter->AddAllowedType(ObjectType::Charger_Basic);

  cubesFilter->AddAllowedFamily(ObjectFamily::LightCube);

  searchSpacePointEvaluator = std::make_unique<Util::RejectionSamplerHelper<Point2f>>();
  searchSpacePolyEvaluator  = std::make_unique<Util::RejectionSamplerHelper<Poly2f>>();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorPlaceCubeByCharger::DynamicVariables::DynamicVariables()
: state(PlaceCubeState::PickUpCube)
, angleSwept_deg(0)
, numTurnsCompleted(0)
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
BehaviorPlaceCubeByCharger::~BehaviorPlaceCubeByCharger()
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
             "BehaviorPlaceCubeByCharger.InitBehavior.NullTurnToLastFaceBehavior");

  {
    using namespace RobotPointSamplerHelper;
    // Set up "would cross cliff" condition
    _iConfig.condHandleCliffs = _iConfig.searchSpacePointEvaluator->AddCondition(
      std::make_shared<RejectIfWouldCrossCliff>(kMinCliffPenaltyDist_mm)
    );
    _iConfig.condHandleCliffs->SetAcceptanceInterpolant(kMaxCliffPenaltyDist_mm, GetRNG());

    // Set up nav map condition
    _iConfig.condHandleCollisions = _iConfig.searchSpacePolyEvaluator->AddCondition(
      std::make_shared<RejectIfCollidesWithMemoryMap>(kTypesToBlockSampling)
    );
  }
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
      TransitionToReactToCharger();
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
                              PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.RobotDoesntKnowWhereChargerIs",
                                              "No charger found yet, searching for charger");
                              TransitionToSearchForCharger();
                            }
                            else {
                              TransitionToConfirmCharger();
                            }
                          } 
                          else { 
                            if(++_dVars.attemptsAtCurrentAction > kMaxPickUpAttempts){
                              TransitionToGetOut();
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
void BehaviorPlaceCubeByCharger::TransitionToConfirmCharger()
{
  SET_STATE(ConfirmCharger);
  bool visuallyVerify = true;
  TurnTowardsObjectAction* visualCheckAction = new TurnTowardsObjectAction(_dVars.chargerID,
                                                                            Radians{M_PI_F},
                                                                            visuallyVerify);
  DelegateIfInControl(visualCheckAction,
    [this](ActionResult result){
      if(ActionResult::SUCCESS == result){
        TransitionToReactToCharger();
      }
      else{
        TransitionToSearchForCharger();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToSearchForCharger()
{
  SET_STATE(SearchForCharger);
  StartNextSearchForChargerTurn();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToReactToCharger()
{
  SET_STATE(ReactToCharger)
  if(RobotKnowsWhereChargerIs()){
    DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::PlaceCubeByChargerReactToCharger),
                        &BehaviorPlaceCubeByCharger::TransitionToDriveToChargerPlacement);
  }
  else{
    TransitionToDriveToRandomPlacement();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToDriveToChargerPlacement()
{
  SET_STATE(DriveToChargerPlacement);

  const auto* charger = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.chargerID, ObjectFamily::Charger);
  if (charger == nullptr) {
    PRINT_NAMED_ERROR("BehaviorPlaceCubeByCharger.TransitionToPlacingCubeOnGround.NullCharger", "Null charger!");
    return;
  }
  
  Pose3d cubePlacementPose;
  cubePlacementPose.SetTranslation(kCubeRelChargerTargetPos);
  cubePlacementPose.SetParent(charger->GetPose());
  DriveToPoseAction* driveToChargerAction = new DriveToPoseAction(cubePlacementPose, true);
  DelegateIfInControl(driveToChargerAction, &BehaviorPlaceCubeByCharger::TransitionToPutCubeDown);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToDriveToRandomPlacement()
{
  SET_STATE(DriveToRandomPlacement);

  const auto& robotInfo = GetBEI().GetRobotInfo();
  const auto& robotPose = robotInfo.GetPose();
  const Point2f robotPosition{ robotPose.GetTranslation() };

  const auto memoryMap = GetBEI().GetMapComponent().GetCurrentMemoryMap();
  DEV_ASSERT( nullptr != memoryMap, "BehaviorFindHome.GenerateSearchPoses.NeedMemoryMap" );

  // Update cliff positions
  _iConfig.condHandleCliffs->SetRobotPosition(robotPosition);
  _iConfig.condHandleCliffs->UpdateCliffs(memoryMap);
  
  // Update memory map
  _iConfig.condHandleCollisions->SetMemoryMap(memoryMap);

  bool targetSelected = false;
  Pose3d randomTargetPoint;
  const unsigned int kMaxNumSampleSteps = 50;
  for( unsigned int cnt=0; cnt<kMaxNumSampleSteps; ++cnt ) {
    // Sample points centered around current robot position
    const float r1 = kMinDrivingDist_mm;
    const float r2 = kMaxDrivingDist_mm;
    Point2f sampledPos = robotPosition;
    {
      const Point2f pt = RobotPointSamplerHelper::SamplePointInAnnulus(GetRNG(), r1, r2 );
      sampledPos.x() += pt.x();
      sampledPos.y() += pt.y();
    }

    const bool acceptedPoint = _iConfig.searchSpacePointEvaluator->Evaluate(GetRNG(), sampledPos );
    if( !acceptedPoint ) {
      continue;
    }

    // choose a random angle about the Z axis to create a full test pose
    const float angle = M_TWO_PI_F * static_cast<float>( GetRNG().RandDbl() );
    Pose2d testPose( Radians{angle}, sampledPos );

    // Get the robot's would-be bounding quad at the test pose
    const auto& quad = robotInfo.GetBoundingQuadXY( Pose3d{testPose} );
    Poly2f footprint;
    footprint.ImportQuad2d(quad);

    const bool acceptedFootprint = _iConfig.searchSpacePolyEvaluator->Evaluate( GetRNG(), footprint );
    if( !acceptedFootprint ) {
      continue;
    }

    // if we get here, accept!!
    // Convert to full 3D pose in robot world origin:
    randomTargetPoint = Pose3d(testPose);
    randomTargetPoint.SetParent(robotInfo.GetWorldOrigin());
    targetSelected = true;
    break;
  }

  if (!targetSelected) {
    PRINT_NAMED_WARNING("BehaviorPlaceCubeByCharger.GenerateSearchPoses.NoPosesFound",
                        "No poses that satisfy the sampling requirements were found. Drop cube and exit");
    TransitionToPutCubeDown();
  }
  else{
    DelegateIfInControl(new DriveToPoseAction(randomTargetPoint, false),
                        &BehaviorPlaceCubeByCharger::TransitionToPutCubeDown);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToPutCubeDown()
{
  SET_STATE(PutCubeDown);

  AnimationTrigger putDownAnimTrigger;
  if(RobotKnowsWhereChargerIs()){
    putDownAnimTrigger = AnimationTrigger::PlaceCubeByChargerSuccess;
  }
  else{
    putDownAnimTrigger = AnimationTrigger::PlaceCubeByChargerFail;
  }

  // Place the cube using the Animation 
  DelegateIfInControl(new TriggerAnimationAction(putDownAnimTrigger),
    [this](ActionResult result) {
      const auto& robotInfo = GetBEI().GetRobotInfo();
      if (robotInfo.GetCarryingComponent().IsCarryingObject()) {
        PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.ObjectStillHeld",
                         "Robot still thinks its carrying an object after the anim finished");
        TransitionToLookDownAtBlock();
      }
      else{
        PRINT_NAMED_INFO("BehaviorPlaceCubeByCharger.PutDownSuccessfull",
                         "Robot put down cube successfully, transition to get out");
        TransitionToGetOut();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToLookDownAtBlock()
{
  SET_STATE(LookDownAtBlock)
  DelegateIfInControl(CreateLookAfterPlaceAction(GetBEI().GetRobotInfo().GetCarryingComponent(), true),
    [this]() {
      auto& robotInfo = GetBEI().GetRobotInfo();
      if(robotInfo.GetCarryingComponent().IsCarryingObject()) {
        // No matter what, even if we didn't see the object we were
        // putting down for some reason, mark the robot as not carrying
        // anything so we don't get stuck in a loop of trying to put
        // something down (i.e. assume the object is no longer on our lift)
        // TODO Carried over from BehaviorPutDownBlock: We should really be using 
        // some kind of PlaceOnGroundAction instead of raw animation 
        PRINT_NAMED_WARNING("BehaviorPutDownBlock.LookDownAtBlock.DidNotSeeBlock",
                            "Forcibly setting carried objects as unattached");
        robotInfo.GetCarryingComponent().SetCarriedObjectAsUnattached();
      }
      TransitionToGetOut();
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActionRunner* BehaviorPlaceCubeByCharger::CreateLookAfterPlaceAction(CarryingComponent& carryingComponent, bool doLookAtFaceAfter)
{
  CompoundActionSequential* action = new CompoundActionSequential();
  if( carryingComponent.IsCarryingObject() ) {
    // glance down to see if we see the cube if we still think we are carrying
    static const int kNumFrames = 2;
    
    CompoundActionParallel* parallel = new CompoundActionParallel({new MoveHeadToAngleAction(DEG_TO_RAD(kFinalHeadAngle_deg)),
                                                                   new DriveStraightAction(kVerifyBackupDist_mm)});
    action->AddAction(parallel);
    action->AddAction(new WaitForImagesAction(kNumFrames, VisionMode::DetectingMarkers));
  }

  return action;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::TransitionToGetOut()
{
  SET_STATE(GetOut);

  if(_iConfig.turnToLastFaceBehavior->WantsToBeActivated()){
    DelegateIfInControl(_iConfig.turnToLastFaceBehavior.get());
  }
  // Cancel self by default
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorPlaceCubeByCharger::RobotKnowsWhereChargerIs()
{
  const auto& robotPose = GetBEI().GetRobotInfo().GetPose();
  const auto* charger = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(robotPose, *_iConfig.chargerFilter);

  if (charger == nullptr) {
    return false;
  }

  _dVars.chargerID = charger->GetID();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorPlaceCubeByCharger::StartNextSearchForChargerTurn()
{
  // Have we turned enough to sweep the required angle?
  // Or have we exceeded the max number of turns?
  const bool sweptEnough = (_dVars.angleSwept_deg > kMinSearchAngle);
  const bool exceededMaxTurns = (_dVars.numTurnsCompleted >= kMaxNumSearchTurns);

  if (sweptEnough || exceededMaxTurns) {
    // If we made it all the way through the search without finding anything, stop the search and move on
    TransitionToReactToCharger();
    PRINT_CH_INFO("Behaviors", "BehaviorPlaceCubeByCharger.TransitionToSearchTurn.CompletedQuickSearch",
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
        StartNextSearchForChargerTurn();
      });
  }
}

} // namespace Victor
} // namespace Anki
