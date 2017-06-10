/**
 * File: behaviorFactory
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Factory for creating behaviors from data / messages
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"

// Behaviors:
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequence.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorDrivePath.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorFindFaces.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorKnockOverCubes.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorPickupCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorPopAWheelie.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorRollBlock.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorStackBlocks.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorDevTurnInPlaceTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorLiftLoadTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorCheckForStackAtInterval.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorCubeLiftWorkout.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorDriveToFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorOnConfigSeen.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorPickUpAndPutDownCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorBuildPyramid.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorBuildPyramidBase.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorRespondPossiblyRoll.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreBringCubeToBeacon.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreLookAroundInPlace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreVisitPossibleMarker.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorLookInPlaceMemoryMap.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorThinkAboutBeacons.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/exploration/behaviorVisitInterestingEdge.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/gameRequest/behaviorRequestGameSimple.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/oneShots/behaviorCantHandleTallStack.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/oneShots/behaviorDance.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/oneShots/behaviorSinging.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/putDownDispatch/behaviorLookForFaceAndCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorBouncer.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorFistBump.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorGuardDog.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorPeekABoo.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorPounceOnMotion.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/userInteractive/behaviorTrackLaser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/meetCozmo/behaviorEnrollFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/meetCozmo/behaviorRespondToRenameFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/onboarding/behaviorOnboardingShowCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorRamIntoBlock.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToDoubleTap.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToFrustration.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToOnCharger.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToPet.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToPickup.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToPyramid.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToReturnedToTreads.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnBack.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnSide.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToRobotShaken.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToSparked.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToStackOfCubes.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToUnexpectedMovement.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToVoiceCommand.h"

#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kBehaviorClassKey = "behaviorClass";
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFactory::BehaviorFactory(Robot& robot)
: _robot(robot)
{
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAllBehaviorsList>();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFactory::~BehaviorFactory()
{
  // Delete all behaviors owned by the factory
  
  for (auto& it : _idToBehaviorMap)
  {
    IBehavior* behavior = it.second;
    // we delete rather than destroy to avoid invalidating the map - it's emptied at the end anyway
    assert(behavior->IsOwnedByFactory());
    DeleteBehaviorInternal(behavior);
  }
    
  _idToBehaviorMap.clear();
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorFactory::CreateBehavior(const Json::Value& behaviorJson, Robot& robot, NameCollisionRule nameCollisionRule)
{
  
  const Json::Value& behaviorTypeJson = behaviorJson[kBehaviorClassKey];
  const char* behaviorTypeString = behaviorTypeJson.isString() ? behaviorTypeJson.asCString() : "";
  const BehaviorClass behaviorClass = BehaviorClassFromString(behaviorTypeString);

  IBehavior* newBehavior = CreateBehavior(behaviorClass, robot, behaviorJson, nameCollisionRule);
  return newBehavior;
  
  return nullptr;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorFactory::CreateBehavior(BehaviorClass behaviorType, Robot& robot, const Json::Value& config, NameCollisionRule nameCollisionRule)
{
  IBehavior* newBehavior = nullptr;
  
  switch (behaviorType)
  {
    case BehaviorClass::NoneBehavior:
    {
      newBehavior = new BehaviorNone(robot, config);
      break;
    }
    case BehaviorClass::Bouncer:
    {
      newBehavior = new BehaviorBouncer(robot, config);
      break;
    }
    case BehaviorClass::LookAround:
    {
      newBehavior = new BehaviorLookAround(robot, config);
      break;
    }
    case BehaviorClass::InteractWithFaces:
    {
      newBehavior = new BehaviorInteractWithFaces(robot, config);
      break;
    }
    case BehaviorClass::PlayAnim:
    {
      newBehavior = new BehaviorPlayAnimSequence(robot, config);
      break;
    }
    case BehaviorClass::PlanAnimWithFace:
    {
      newBehavior = new BehaviorPlayAnimSequenceWithFace(robot, config);
      break;
    }
    case BehaviorClass::PlayArbitraryAnim:
    {
      newBehavior = new BehaviorPlayArbitraryAnim(robot, config);
      break;
    }
    case BehaviorClass::PounceOnMotion:
    {
      newBehavior = new BehaviorPounceOnMotion(robot, config);
      break;
    }
    case BehaviorClass::FindFaces:
    {
      newBehavior = new BehaviorFindFaces(robot, config);
      break;
    }
    case BehaviorClass::FistBump:
    {
      newBehavior = new BehaviorFistBump(robot, config);
      break;
    }
    case BehaviorClass::GuardDog:
    {
      newBehavior = new BehaviorGuardDog(robot, config);
      break;
    }
    case BehaviorClass::RequestGameSimple:
    {
      newBehavior = new BehaviorRequestGameSimple(robot, config);
      break;
    }
    case BehaviorClass::ExploreLookAroundInPlace:
    {
      newBehavior = new BehaviorExploreLookAroundInPlace(robot, config);
      break;
    }
    case BehaviorClass::ExploreVisitPossibleMarker:
    {
      newBehavior = new BehaviorExploreVisitPossibleMarker(robot, config);
      break;
    }
    case BehaviorClass::BringCubeToBeacon:
    {
      newBehavior = new BehaviorExploreBringCubeToBeacon(robot, config);
      break;
    }
    case BehaviorClass::LookInPlaceMemoryMap:
    {
      newBehavior = new BehaviorLookInPlaceMemoryMap(robot, config);
      break;
    }
    case BehaviorClass::ThinkAboutBeacons:
    {
      newBehavior = new BehaviorThinkAboutBeacons(robot, config);
      break;
    }
    case BehaviorClass::VisitInterestingEdge:
    {
      newBehavior = new BehaviorVisitInterestingEdge(robot, config);
      break;
    }
    case BehaviorClass::RollBlock:
    {
      newBehavior = new BehaviorRollBlock(robot, config);
      break;
    }
    case BehaviorClass::FactoryTest:
    {
      newBehavior = new BehaviorFactoryTest(robot, config);
      break;
    }
    case BehaviorClass::FactoryCentroidExtractor:
    {
      newBehavior = new BehaviorFactoryCentroidExtractor(robot, config);
      break;
    }
    case BehaviorClass::DockingTestSimple:
    {
      newBehavior = new BehaviorDockingTestSimple(robot, config);
      break;
    }
    case BehaviorClass::StackBlocks:
    {
      newBehavior = new BehaviorStackBlocks(robot, config);
      break;
    }
    case BehaviorClass::PutDownBlock:
    {
      newBehavior = new BehaviorPutDownBlock(robot, config);
      break;
    }
    case BehaviorClass::DriveOffCharger:
    {
      newBehavior = new BehaviorDriveOffCharger(robot, config);
      break;
    }
    case BehaviorClass::PopAWheelie:
    {
      newBehavior = new BehaviorPopAWheelie(robot, config);
      break;
    }
    case BehaviorClass::PeekABoo:
    {
      newBehavior = new BehaviorPeekABoo(robot, config);
      break;
    }
    case BehaviorClass::DrivePath:
    {
      newBehavior = new BehaviorDrivePath(robot, config);
      break;
    }
    case BehaviorClass::PickUpCube:
    {
      newBehavior = new BehaviorPickUpCube(robot, config);
      break;
    }
    case BehaviorClass::PickUpAndPutDownCube:
    {
      newBehavior = new BehaviorPickUpAndPutDownCube(robot, config);
      break;
    }
    case BehaviorClass::KnockOverCubes:
    {
      newBehavior = new BehaviorKnockOverCubes(robot, config);
      break;
    }
    case BehaviorClass::BuildPyramid:
    {
      newBehavior = new BehaviorBuildPyramid(robot, config);
      break;
    }
    case BehaviorClass::BuildPyramidBase:
    {
      newBehavior = new BehaviorBuildPyramidBase(robot, config);
      break;
    }
    case BehaviorClass::CantHandleTallStack:
    {
      newBehavior = new BehaviorCantHandleTallStack(robot, config);
      break;
    }
    case BehaviorClass::OnboardingShowCube:
    {
      newBehavior = new BehaviorOnboardingShowCube(robot, config);
      break;
    }
    case BehaviorClass::OnConfigSeen:
    {
      newBehavior = new BehaviorOnConfigSeen(robot, config);
      break;
    }
    case BehaviorClass::LookForFaceAndCube:
    {
      newBehavior = new BehaviorLookForFaceAndCube(robot, config);
      break;
    }
    case BehaviorClass::CubeLiftWorkout:
    {
      newBehavior = new BehaviorCubeLiftWorkout(robot, config);
      break;
    }
    case BehaviorClass::CheckForStackAtInterval:
    {
      newBehavior = new BehaviorCheckForStackAtInterval(robot, config);
      break;
    }
    case BehaviorClass::EnrollFace:
    {
      newBehavior = new BehaviorEnrollFace(robot, config);
      break;
    }
    case BehaviorClass::LiftLoadTest:
    {
      newBehavior = new BehaviorLiftLoadTest(robot, config);
      break;
    }
    case BehaviorClass::RamIntoBlock:
    {
      newBehavior = new BehaviorRamIntoBlock(robot, config);
      break;
    }
    case BehaviorClass::RespondPossiblyRoll:
    {
      newBehavior = new BehaviorRespondPossiblyRoll(robot, config);
      break;
    }
    case BehaviorClass::RespondToRenameFace:
    {
      newBehavior = new BehaviorRespondToRenameFace(robot, config);
      break;
    }
    case BehaviorClass::PyramidThankYou:
    {
      newBehavior = new BehaviorPyramidThankYou(robot, config);
      break;
    }
      
    case BehaviorClass::FeedingEat:
    {
      newBehavior = new BehaviorFeedingEat(robot, config);
      break;
    }
    case BehaviorClass::FeedingSearchForCube:
    {
      newBehavior = new BehaviorFeedingSearchForCube(robot, config);
      break;
    }
    case BehaviorClass::Singing:
    {
      newBehavior = new BehaviorSinging(robot, config);
      break;
    }
    case BehaviorClass::TrackLaser:
    {
      newBehavior = new BehaviorTrackLaser(robot, config);
      break;
    }
    case BehaviorClass::Dance:
    {
      newBehavior = new BehaviorDance(robot, config);
      break;
    }
    case BehaviorClass::DevTurnInPlaceTest:
    {
      newBehavior = new BehaviorDevTurnInPlaceTest(robot, config);
      break;
    }
    case BehaviorClass::DriveToFace:
    {
      newBehavior = new BehaviorDriveToFace(robot, config);
      break;
    }
    case BehaviorClass::TurnToFace:
    {
      newBehavior = new BehaviorTurnToFace(robot, config);
      break;
    }
    case BehaviorClass::ExpressNeeds:
    {
      newBehavior = new BehaviorExpressNeeds(robot, config);
      break;
    }
    
    ////////////
    // Behaviors that are used by reaction triggers
    ////////////

    case BehaviorClass::ReactToPickup:
    {
      newBehavior = new BehaviorReactToPickup(robot, config);
      break;
    }
    case BehaviorClass::ReactToPlacedOnSlope:
    {
      newBehavior = new BehaviorReactToPlacedOnSlope(robot, config);
      break;
    }
    case BehaviorClass::ReactToCliff:
    {
      newBehavior = new BehaviorReactToCliff(robot, config);
      break;
    }
    case BehaviorClass::ReactToReturnedToTreads:
    {
      newBehavior = new BehaviorReactToReturnedToTreads(robot, config);
      break;
    }
    case BehaviorClass::ReactToRobotOnBack:
    {
      newBehavior = new BehaviorReactToRobotOnBack(robot, config);
      break;
    }
    case BehaviorClass::ReactToRobotOnFace:
    {
      newBehavior = new BehaviorReactToRobotOnFace(robot, config);
      break;
    }
    case BehaviorClass::ReactToRobotOnSide:
    {
      newBehavior = new BehaviorReactToRobotOnSide(robot, config);
      break;
    }
    case BehaviorClass::ReactToRobotShaken:
    {
      newBehavior = new BehaviorReactToRobotShaken(robot, config);
      break;
    }
    case BehaviorClass::ReactToOnCharger:
    {
      newBehavior = new BehaviorReactToOnCharger(robot, config);
      break;
    }
    case BehaviorClass::AcknowledgeObject:
    {
      newBehavior = new BehaviorAcknowledgeObject(robot, config);
      break;
    }
    case BehaviorClass::AcknowledgeFace:
    {
      newBehavior = new BehaviorAcknowledgeFace(robot, config);
      break;
    }
    case BehaviorClass::ReactToUnexpectedMovement:
    {
      newBehavior = new BehaviorReactToUnexpectedMovement(robot, config);
      break;
    }
    case BehaviorClass::ReactToMotorCalibration:
    {
      newBehavior = new BehaviorReactToMotorCalibration(robot, config);
      break;
    }
    case BehaviorClass::ReactToCubeMoved:
    {
      newBehavior = new BehaviorAcknowledgeCubeMoved(robot, config);
      break;
    }
    case BehaviorClass::ReactToFrustration:
    {
      newBehavior = new BehaviorReactToFrustration(robot, config);
      break;
    }
    case BehaviorClass::ReactToSparked:
    {
      newBehavior = new BehaviorReactToSparked(robot, config);
      break;
    }
    case BehaviorClass::ReactToStackOfCubes:
    {
      newBehavior = new BehaviorReactToStackOfCubes(robot, config);
      break;
    }
    case BehaviorClass::ReactToPyramid:
    {
      newBehavior = new BehaviorReactToPyramid(robot, config);
      break;
    }
    case BehaviorClass::ReactToPet:
    {
      newBehavior = new BehaviorReactToPet(robot, config);
      break;
    }
    case BehaviorClass::ReactToDoubleTap:
    {
      newBehavior = new BehaviorReactToDoubleTap(robot, config);
      break;
    }
    case BehaviorClass::ReactToVoiceCommand:
    {
      newBehavior = new BehaviorReactToVoiceCommand(robot, config);
      break;
    }
  }
  
  
  if(newBehavior != nullptr){
    newBehavior->SetBehaviorClass(behaviorType);
    newBehavior = AddToFactory(newBehavior, nameCollisionRule);
  }
  
  if (newBehavior == nullptr){
    PRINT_NAMED_ERROR("BehaviorFactory.CreateBehavior.Failed",
                      "Failed to create Behavior of type '%s'", BehaviorClassToString(behaviorType));
    return nullptr;
  }
  
  
  return newBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorFactory::AddToFactory(IBehavior* newBehavior, NameCollisionRule nameCollisionRule)
{
  assert(newBehavior);
  assert(!newBehavior->_isOwnedByFactory);
  newBehavior->_isOwnedByFactory = true;
  
  BehaviorID behaviorID = newBehavior->GetID();
  auto newEntry = _idToBehaviorMap.insert( BehaviorIDToBehaviorMap::value_type(behaviorID, newBehavior) );
  
  const bool addedNewEntry = newEntry.second;

  if (addedNewEntry)
  {
    PRINT_NAMED_INFO("BehaviorFactory::AddToFactory", "Added new behavior '%s' %p",
                     BehaviorIDToString(behaviorID), newBehavior);
  }
  else
  {
    // map insert failed (found an existing entry), handle this name collision
    IBehavior* oldBehavior = newEntry.first->second;
    
    switch(nameCollisionRule)
    {
      case NameCollisionRule::ReuseOld:
      {
        PRINT_NAMED_INFO("BehaviorFactory.AddToFactory.ReuseOld",
                         "Behavior '%s' already exists (%p) - reusing!",
                         BehaviorIDToString(behaviorID), oldBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we never added newBehavior to the map
        DeleteBehaviorInternal(newBehavior);
        newBehavior = oldBehavior;
        break;
      }
      case NameCollisionRule::OverwriteWithNew:
      {
        PRINT_NAMED_INFO("BehaviorFactory.AddToFactory.Overwrite",
                         "Behavior '%s' already exists (%p) - overwriting with %p",
                         BehaviorIDToString(behaviorID), oldBehavior, newBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we are replacing oldBehavior's map entry
        DeleteBehaviorInternal(oldBehavior);
        newEntry.first->second = newBehavior;
        break;
      }
      case NameCollisionRule::Fail:
      {
        PRINT_NAMED_ERROR("BehaviorFactory.AddToFactory.NameClashFail",
                          "Behavior '%s' already exists (%p) - fail!",
                          BehaviorIDToString(behaviorID), oldBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we never added newBehavior to the map
        DeleteBehaviorInternal(newBehavior);
        newBehavior = nullptr;
        break;
      }
    }
  }
  
  return newBehavior;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactory::DestroyBehavior(IBehavior* behavior)
{
  // we assume all behaviors are created and owned by factory so this should always be true
  DEV_ASSERT_MSG(behavior->IsOwnedByFactory(), "BehaviorFactory.DestroyBehavior",
                 "Attempted to destroy behavior not owned by factory");
  RemoveBehaviorFromMap(behavior);
  DeleteBehaviorInternal(behavior);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactory::SafeDestroyBehavior(IBehavior*& behaviorPtrRef)
{
  DestroyBehavior(behaviorPtrRef);
  behaviorPtrRef = nullptr;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFactory::DeleteBehaviorInternal(IBehavior* behavior)
{
  behavior->_isOwnedByFactory = false;
  delete behavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFactory::RemoveBehaviorFromMap(IBehavior* behavior)
{
  // check the scored behavior map
  const auto& scoredIt = _idToBehaviorMap.find(behavior->GetID());
  if (scoredIt != _idToBehaviorMap.end())
  {
    // check it's the same pointer
    IBehavior* existingBehavior = scoredIt->second;
    if (existingBehavior == behavior)
    {
      _idToBehaviorMap.erase(scoredIt);
      return true;
    }
  }
  
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorFactory::FindBehaviorByID(BehaviorID behaviorID) const
{
  IBehavior* foundBehavior = nullptr;

  auto scoredIt = _idToBehaviorMap.find(behaviorID);
  if (scoredIt != _idToBehaviorMap.end())
  {
    foundBehavior = scoredIt->second;
    return foundBehavior;
  }
  
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* BehaviorFactory::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  for(const auto behavior : _idToBehaviorMap)
  {
    if(behavior.second->GetExecutableType() == type)
    {
      return behavior.second;
    }
  }
  
  return nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BehaviorFactory::HandleMessage(const ExternalInterface::RequestAllBehaviorsList& msg)
{
  std::vector<BehaviorID> behaviorList;
  for(const auto& entry : GetBehaviorMap()){
    behaviorList.push_back(entry.first);
  }
  
  ExternalInterface::RespondAllBehaviorsList message(std::move(behaviorList));
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

  
} // namespace Cozmo
} // namespace Anki

