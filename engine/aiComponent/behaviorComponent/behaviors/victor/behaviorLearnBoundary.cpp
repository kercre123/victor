/**
 * File: BehaviorLearnBoundary.cpp
 *
 * Author: Matt Michini
 * Created: 2019-01-11
 *
 * Description: Use the cube to learn boundaries of the "play space"
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorLearnBoundary.h"

#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/actions/compoundActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/actions/visuallyVerifyActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindCube.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/components/cubes/cubeLights/cubeLightComponent.h"
#include "engine/navMap/mapComponent.h"
#include "engine/navMap/memoryMap/data/memoryMapData_Boundary.h"
#include "engine/viz/vizManager.h"

#include "coretech/common/engine/math/polygon_impl.h"

#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Vector {
  
namespace {
  const float kMinDistFromCube_mm = 200.f;
  const float kMaxDistFromCube_mm = 350.f;
  
  const float kMinDistToNextBoundaryPoint_mm = 30.f;
  
  const AnimationTrigger kGotItAnim = AnimationTrigger::ExploringHuhFar;

  // Thickness of the boundary inserted into the nav map
  const float kBoundaryThickness_mm = 40.f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLearnBoundary::BehaviorLearnBoundary(const Json::Value& config)
 : ICozmoBehavior(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorLearnBoundary::~BehaviorLearnBoundary()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLearnBoundary::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // Do not need a cube connection, but we'll take one if we can get it.
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  
  // The engine will take control of the cube lights to communicate internal state to the user, unless we indicate that
  // we want ownership
  modifiers.connectToCubeInBackground = true;

  // Turn on some vision modes for better marker detection
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingMarkers,         EVisionUpdateFrequency::High });
  modifiers.visionModesForActiveScope->insert({ VisionMode::FullWidthMarkerDetection, EVisionUpdateFrequency::High });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.findCubeBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
//  const char* list[] = {
//  };
//  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::OnBehaviorActivated() 
{
  _dVars = DynamicVariables();
  
  TransitionToFindCube();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::OnBehaviorDeactivated()
{
//  if (_dVars.cubePoses.size() > 1) {
//    LOG_WARNING("BehaviorLearnBoundary.OnBehaviorDeactivated.NoBoundaryCreated",
//                "At the end of the LearnBoundary behavior, we had enough poses to create a boundary but for some "
//                "reason did not");
//  }
}

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  _iConfig.findCubeBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(FindCube));
  DEV_ASSERT(nullptr != _iConfig.findCubeBehavior,
             "BehaviorLearnBoundary.InitBehavior.NullFindCubeBehavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObservableObject* BehaviorLearnBoundary::GetLocatedCube()
{
  // Try to get the existing cube if we have it
  auto* object = GetBEI().GetBlockWorld().GetLocatedObjectByID(_dVars.cubeID);
  if (object != nullptr) {
    return object;
  }
  
  BlockWorldFilter filt;
  filt.AddAllowedType(ObjectType::Block_LIGHTCUBE1);
  object = GetBEI().GetBlockWorld().FindLocatedObjectClosestTo(GetBEI().GetRobotInfo().GetPose(), filt);
  
  if (object != nullptr) {
    // If this is the first time grabbing the cube, then update its initial upAxis as well
    if (!_dVars.cubeID.IsSet()) {
      _dVars.firstSeenUpAxis = object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>();
    }
    _dVars.cubeID = object->GetID();
  }
  
  return object;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::PlayCubeLightAnim(const CubeAnimationTrigger& cubeAnimTrigger)
{
  if (!GetBEI().GetCubeCommsComponent().IsConnectedToCube()) {
    return;
  }
  
  const bool allowInBackground = true;
  GetBEI().GetCubeLightComponent().PlayLightAnimByTrigger(_dVars.cubeID,
                                                          cubeAnimTrigger,
                                                          [](){},
                                                          allowInBackground);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorLearnBoundary::HasCubeBeenFlipped()
{
  auto* object = GetLocatedCube();
  if ((object != nullptr) &&
      (object->GetPose().GetRotationMatrix().GetRotatedParentAxis<'Z'>() != _dVars.firstSeenUpAxis)) {
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToFindCube()
{
  const bool haveCube = (GetLocatedCube() != nullptr);
  
  if (haveCube) {
    TransitionToFaceAndVerifyCube();
  } else {
    const bool wantsToRun = _iConfig.findCubeBehavior->WantsToBeActivated();
    DEV_ASSERT(wantsToRun, "BehaviorLearnBoundary.TransitionToFindCube.FindCubeDoesntWantToRun");
    DelegateIfInControl(_iConfig.findCubeBehavior.get(),
                        [this](){
                          // Do we have a cube yet?
                          const bool haveCube = (GetLocatedCube() != nullptr);
                          if (haveCube) {
                            TransitionToFaceAndVerifyCube();
                          } else {
                            LOG_WARNING("BehaviorLearnBoundary.TransitionToFindCube.StillNoCube", "");
                            CancelSelf();
                          }
                        });
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToFaceAndVerifyCube()
{
  auto* action = new CompoundActionSequential();
  action->AddAction(new TurnTowardsObjectAction(_dVars.cubeID));
  action->AddAction(new VisuallyVerifyObjectAction(_dVars.cubeID));
  
  DelegateIfInControl(action,
                      [this]() {
                        TransitionToAdjustPositionWrtCube();
                      });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToAdjustPositionWrtCube()
{
  auto* object = GetLocatedCube();
  DEV_ASSERT(object != nullptr, "BehaviorLearnBoundary.TransitionToAdjustPositionWrtCube.NullObject");
  
  float distToCube_mm = 0.f;
  const bool result = ComputeDistanceBetween(GetBEI().GetRobotInfo().GetPose(),
                                             object->GetPose(),
                                             distToCube_mm);
  DEV_ASSERT(result, "BehaviorLearnBoundary.TransitionToAdjustPositionWrtCube.FailedComputingDistToCube");
  if (!Util::InRange(distToCube_mm, kMinDistFromCube_mm, kMaxDistFromCube_mm)) {
    // Adjust our position to be within the correct range from the cube
    float distToDrive = 0.f;
    if (distToCube_mm < kMinDistFromCube_mm) {
      distToDrive = distToCube_mm - kMinDistFromCube_mm;
    } else if (distToCube_mm > kMaxDistFromCube_mm) {
      distToDrive = distToCube_mm - kMaxDistFromCube_mm;
    }
    const float speed_mmps = 125.f;
    DelegateIfInControl(new DriveStraightAction(distToDrive, speed_mmps),
                        &BehaviorLearnBoundary::TransitionToRecordCubePose);
  } else {
    // distance is OK, go directly into recording the cube pose
    TransitionToRecordCubePose();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToRecordCubePose()
{
  auto* object = GetLocatedCube();
  DEV_ASSERT(object != nullptr, "BehaviorLearnBoundary.TransitionToRecordCubePose.NullObject");
  
  _dVars.cubePoses.push_back(object->GetPose());
  
  PlayCubeLightAnim(CubeAnimationTrigger::LearnBoundarySuccess);
  
  // Play a sound and an animation indicating "got it!"
  
  using GE = AudioMetaData::GameEvent::GenericEvent;
  using GO = AudioMetaData::GameObjectType;
  GetBEI().GetRobotAudioClient().PostEvent(GE::Play__Robot_Vic_Sfx__Camera_Flash,
                                           GO::Behavior);
  
  DelegateIfInControl(new TriggerAnimationAction(kGotItAnim),
                      &BehaviorLearnBoundary::TransitionToWaitForCubeToMove);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToWaitForCubeToMove()
{
  PlayCubeLightAnim(CubeAnimationTrigger::LearnBoundaryReady);
  
  // Has the cube moved far enough away from the previous location?
  auto waitLambda = [this](Robot& robot){
    auto* object = GetLocatedCube();
    if (object == nullptr) {
      return false;
    }

    const auto& latestBoundaryPose = _dVars.cubePoses.back();
    float distFromCubeToLast_mm = 0.f;
    const bool result = ComputeDistanceBetween(object->GetPose(),
                                               latestBoundaryPose,
                                               distFromCubeToLast_mm);
    DEV_ASSERT(result, "BehaviorLearnBoundary.TransitionToWaitForCubeToMove.FailedComputingDistanceBetween");
    
    // Has the cube moved far enough or been flipped to a new side?
    
    const bool movedFarEnough = (distFromCubeToLast_mm > kMinDistToNextBoundaryPoint_mm);
    const bool flippedToNewSide = HasCubeBeenFlipped();
    return (movedFarEnough || flippedToNewSide);
  };
  
  const float kWaitForMovementTimeout_sec = 6.f;
  
  DelegateIfInControl(new WaitForLambdaAction(waitLambda, kWaitForMovementTimeout_sec),
                      [this](const ActionResult& result) {
                        if (result == ActionResult::TIMEOUT) {
                          LOG_WARNING("BehaviorLearnBoundary.TransitionToWaitForCubeToMove.Lambda.Timeout", "");
                          TransitionToCreateBoundary();
                        } else if (HasCubeBeenFlipped()) {
                          LOG_WARNING("BehaviorLearnBoundary.TransitionToAdjustPositionWrtCube.CubeRotated",
                                      "User flipped the cube onto another side - ending the behavior");
                          TransitionToCreateBoundary();
                        } else {
                          TransitionToFindCube();
                        }
                      });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToCreateBoundary()
{
  // Record the boundary if there are eligible cube poses
  const auto& poses = _dVars.cubePoses;
  if (poses.size() >= 2) {
    // Simply go pairwise through each of the poses
    for (int i=0 ; i < poses.size() - 1 ; i++) {
      const auto& fromPose = poses[i];
      const auto& toPose = poses[i+1];
      
      // TODO: Maybe check that the parent is WorldOrigin here

      const Point2f boundaryFrom{fromPose.GetTranslation().x(), fromPose.GetTranslation().y()};
      const Point2f boundaryTo{toPose.GetTranslation().x(), toPose.GetTranslation().y()};
      
      MemoryMapData_Boundary boundary(boundaryFrom,
                                      boundaryTo,
                                      GetBEI().GetRobotInfo().GetWorldOrigin(),
                                      kBoundaryThickness_mm,
                                      GetBEI().GetRobotInfo().GetLastMsgTimestamp());
      GetBEI().GetMapComponent().InsertData(Poly2f{boundary.GetQuad()}, boundary);
    }
    
    // survey the boundary after creating it
    TransitionToSurveyBoundary();
  } else {
    LOG_WARNING("BehaviorLearnBoundary.TransitionToCreateBoundary.NotEnoughPoses",
                "Not enough cube poses to create a boundary");
    CancelSelf();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorLearnBoundary::TransitionToSurveyBoundary()
{
  auto* action = new CompoundActionSequential();
  for (const auto& pose : _dVars.cubePoses) {
    action->AddAction(new TurnTowardsPoseAction(pose));
  }
  DelegateIfInControl(action);
}

}
}
