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
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"

// Behaviors:
#include "../behaviors/dispatch/behaviorLookForFaceAndCube.h"
#include "../behaviors/exploration/behaviorExploreBringCubeToBeacon.h"
#include "../behaviors/exploration/behaviorExploreLookAroundInPlace.h"
#include "../behaviors/exploration/behaviorExploreVisitPossibleMarker.h"
#include "../behaviors/exploration/behaviorLookInPlaceMemoryMap.h"
#include "../behaviors/exploration/behaviorThinkAboutBeacons.h"
#include "../behaviors/exploration/behaviorVisitInterestingEdge.h"
#include "anki/cozmo/basestation/behaviors/behaviorDockingTestSimple.h"
#include "anki/cozmo/basestation/behaviors/behaviorDriveOffCharger.h"
#include "anki/cozmo/basestation/behaviors/behaviorDrivePath.h"
#include "anki/cozmo/basestation/behaviors/behaviorFactoryCentroidExtractor.h"
#include "anki/cozmo/basestation/behaviors/behaviorFactoryTest.h"
#include "anki/cozmo/basestation/behaviors/behaviorFindFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviors/behaviorOnboardingShowCube.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayAnimSequence.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviors/behaviorPutDownBlock.h"
#include "anki/cozmo/basestation/behaviors/gameRequest/behaviorRequestGameSimple.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorAcknowledgeFace.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorCantHandleTallStack.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactAcknowledgeCubeMoved.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/behaviors/reactionary/BehaviorReactToDoubleTap.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToFrustration.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToMotorCalibration.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToOnCharger.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPet.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPickup.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPoke.h"
#include "anki/cozmo/basestation/behaviors/reactionary/BehaviorReactToPyramid.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToReturnedToTreads.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnBack.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnFace.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToRobotOnSide.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToSparked.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToStackOfCubes.h"
#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToUnexpectedMovement.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramid.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorCubeLiftworkout.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorKnockOverCubes.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPickupCube.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPopAWheelie.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorPounceOnMotion.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorRollBlock.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorStackBlocks.h"

namespace Anki {
namespace Cozmo {


BehaviorFactory::BehaviorFactory()
{
}


BehaviorFactory::~BehaviorFactory()
{
  // Delete all behaviors owned by the factory
  
  for (auto& it : _nameToBehaviorMap)
  {
    IBehavior* behavior = it.second;
    // we delete rather than destroy to avoid invalidating the map - it's emptied at the end anyway
    assert(behavior->IsOwnedByFactory());
    DeleteBehaviorInternal(behavior);
  }
  
  _nameToBehaviorMap.clear();
}


IBehavior* BehaviorFactory::CreateBehavior(BehaviorType behaviorType, Robot& robot, const Json::Value& config, NameCollisionRule nameCollisionRule)
{
  IBehavior* newBehavior = nullptr;
  
  switch (behaviorType)
  {
    case BehaviorType::NoneBehavior:
    {
      newBehavior = new BehaviorNone(robot, config);
      break;
    }
    case BehaviorType::LookAround:
    {
      newBehavior = new BehaviorLookAround(robot, config);
      break;
    }
    case BehaviorType::InteractWithFaces:
    {
      newBehavior = new BehaviorInteractWithFaces(robot, config);
      break;
    }
    case BehaviorType::ReactToPickup:
    {
      newBehavior = new BehaviorReactToPickup(robot, config);
      break;
    }
    case BehaviorType::ReactToCliff:
    {
      newBehavior = new BehaviorReactToCliff(robot, config);
      break;
    }
    case BehaviorType::ReactToPoke:
    {
      newBehavior = new BehaviorReactToPoke(robot, config);
      break;
    }
    case BehaviorType::PlayAnim:
    {
      newBehavior = new BehaviorPlayAnimSequence(robot, config);
      break;
    }
    case BehaviorType::PlayArbitraryAnim:
    {
      newBehavior = new BehaviorPlayArbitraryAnim(robot, config);
      break;
    }
    case BehaviorType::PounceOnMotion:
    {
      newBehavior = new BehaviorPounceOnMotion(robot, config);
      break;
    }
    case BehaviorType::FindFaces:
    {
      newBehavior = new BehaviorFindFaces(robot, config);
      break;
    }
    case BehaviorType::RequestGameSimple:
    {
      newBehavior = new BehaviorRequestGameSimple(robot, config);
      break;
    }
    case BehaviorType::ExploreLookAroundInPlace:
    {
      newBehavior = new BehaviorExploreLookAroundInPlace(robot, config);
      break;
    }
    case BehaviorType::ExploreVisitPossibleMarker:
    {
      newBehavior = new BehaviorExploreVisitPossibleMarker(robot, config);
      break;
    }
    case BehaviorType::BringCubeToBeacon:
    {
      newBehavior = new BehaviorExploreBringCubeToBeacon(robot, config);
      break;
    }
    case BehaviorType::LookInPlaceMemoryMap:
    {
      newBehavior = new BehaviorLookInPlaceMemoryMap(robot, config);
      break;
    }
    case BehaviorType::ThinkAboutBeacons:
    {
      newBehavior = new BehaviorThinkAboutBeacons(robot, config);
      break;
    }
    case BehaviorType::VisitInterestingEdge:
    {
      newBehavior = new BehaviorVisitInterestingEdge(robot, config);
      break;
    }
    case BehaviorType::RollBlock:
    {
      newBehavior = new BehaviorRollBlock(robot, config);
      break;
    }
    case BehaviorType::FactoryTest:
    {
      newBehavior = new BehaviorFactoryTest(robot, config);
      break;
    }
    case BehaviorType::FactoryCentroidExtractor:
    {
      newBehavior = new BehaviorFactoryCentroidExtractor(robot, config);
      break;
    }
    case BehaviorType::ReactToReturnedToTreads:
    {
      newBehavior = new BehaviorReactToReturnedToTreads(robot, config);
      break;
    }
    case BehaviorType::ReactToRobotOnBack:
    {
      newBehavior = new BehaviorReactToRobotOnBack(robot, config);
      break;     
    }
    case BehaviorType::ReactToRobotOnFace:
    {
      newBehavior = new BehaviorReactToRobotOnFace(robot, config);
      break;
    }
    case BehaviorType::ReactToRobotOnSide:
    {
      newBehavior = new BehaviorReactToRobotOnSide(robot, config);
      break;
    }
    case BehaviorType::DockingTestSimple:
    {
      newBehavior = new BehaviorDockingTestSimple(robot, config);
      break;
    }
    case BehaviorType::StackBlocks:
    {
      newBehavior = new BehaviorStackBlocks(robot, config);
      break;
    }
    case BehaviorType::PutDownBlock:
    {
      newBehavior = new BehaviorPutDownBlock(robot, config);
      break;
    }
    case BehaviorType::ReactToOnCharger:
    {
      newBehavior = new BehaviorReactToOnCharger(robot, config);
      break;
    }
    case BehaviorType::DriveOffCharger:
    {
      newBehavior = new BehaviorDriveOffCharger(robot, config);
      break;
    }
    case BehaviorType::PopAWheelie:
    {
      newBehavior = new BehaviorPopAWheelie(robot, config);
      break;
    }
    case BehaviorType::AcknowledgeObject:
    {
      newBehavior = new BehaviorAcknowledgeObject(robot, config);
      break;
    }
    case BehaviorType::AcknowledgeFace:
    {
      newBehavior = new BehaviorAcknowledgeFace(robot, config);
      break;
    }
    case BehaviorType::DrivePath:
    {
      newBehavior = new BehaviorDrivePath(robot, config);
      break;
    }
    case BehaviorType::ReactToUnexpectedMovement:
    {
      newBehavior = new BehaviorReactToUnexpectedMovement(robot, config);
      break;
    }
    case BehaviorType::ReactToMotorCalibration:
    {
      newBehavior = new BehaviorReactToMotorCalibration(robot, config);
      break;
    }
    case BehaviorType::PickUpCube:
    {
      newBehavior = new BehaviorPickUpCube(robot, config);
      break;
    }
    case BehaviorType::ReactToCubeMoved:
    {
      newBehavior = new BehaviorReactAcknowledgeCubeMoved(robot, config);
      break;
    }
    case BehaviorType::KnockOverCubes:
    {
      newBehavior = new BehaviorKnockOverCubes(robot, config);
      break;
    }
    case BehaviorType::BuildPyramid:
    {
      newBehavior = new BehaviorBuildPyramid(robot, config);
      break;
    }
    case BehaviorType::BuildPyramidBase:
    {
      newBehavior = new BehaviorBuildPyramidBase(robot, config);
      break;
    }
    case BehaviorType::ReactToFrustration:
    {
      newBehavior = new BehaviorReactToFrustration(robot, config);
      break;
    }
    case BehaviorType::CantHandleTallStack:
    {
      newBehavior = new BehaviorCantHandleTallStack(robot, config);
      break;
    }
    case BehaviorType::OnboardingShowCube:
    {
      newBehavior = new BehaviorOnboardingShowCube(robot, config);
      break;
    }
    case BehaviorType::ReactToSparked:
    {
      newBehavior = new BehaviorReactToSparked(robot, config);
      break;
    }
    case BehaviorType::ReactToStackOfCubes:
    {
      newBehavior = new BehaviorReactToStackOfCubes(robot, config);
      break;
    }
    case BehaviorType::ReactToPyramid:
    {
      newBehavior = new BehaviorReactToPyramid(robot, config);
      break;
    }
    case BehaviorType::ReactToPet:
    {
      newBehavior = new BehaviorReactToPet(robot, config);
      break;
    }
    case BehaviorType::ReactToDoubleTap:
    {
      newBehavior = new BehaviorReactToDoubleTap(robot, config);
      break;
    }
    case BehaviorType::LookForFaceAndCube:
    {
      newBehavior = new BehaviorLookForFaceAndCube(robot, config);
      break;
    }
    case BehaviorType::CubeLiftWorkout:
    {
      newBehavior = new BehaviorCubeLiftWorkout(robot, config);
      break;
    }
    case BehaviorType::Count:
    {
      PRINT_NAMED_ERROR("BehaviorFactory.CreateBehavior.BadType", "Unexpected type '%s'", EnumToString(behaviorType));
      break;
    }
  }
  
  
  if (newBehavior)
  {
    newBehavior->SetBehaviorType(behaviorType);
    //Handle disable by default
    if(newBehavior->IsReactionary()){
      IReactionaryBehavior* reactionary = newBehavior->AsReactionaryBehavior();
      reactionary->HandleDisableByDefault(robot);
    }
    newBehavior = AddToFactory(newBehavior, nameCollisionRule);
  }
    
  if (newBehavior == nullptr)
  {
    PRINT_NAMED_ERROR("BehaviorFactory.CreateBehavior.Failed",
                      "Failed to create Behavior of type '%s'", BehaviorTypeToString(behaviorType));
  }
  
  return newBehavior;
}
  
  
IBehavior* BehaviorFactory::AddToFactory(IBehavior* newBehavior, NameCollisionRule nameCollisionRule)
{
  assert(newBehavior);
  assert(!newBehavior->_isOwnedByFactory);
  
  newBehavior->_isOwnedByFactory = true;
  const std::string& newBehaviorName = newBehavior->GetName();
  
  auto newEntry = _nameToBehaviorMap.insert( NameToBehaviorMap::value_type(newBehaviorName, newBehavior) );
  const bool addedNewEntry = newEntry.second;
  
  if (addedNewEntry)
  {
    PRINT_NAMED_INFO("BehaviorFactory::AddToFactory", "Added new behavior '%s' %p", newBehaviorName.c_str(), newBehavior);
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
                         "Behavior '%s' already exists (%p) - reusing!", newBehaviorName.c_str(), oldBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we never added newBehavior to the map
        DeleteBehaviorInternal(newBehavior);
        newBehavior = oldBehavior;
        break;
      }
      case NameCollisionRule::OverwriteWithNew:
      {
        PRINT_NAMED_INFO("BehaviorFactory.AddToFactory.Overwrite",
                         "Behavior '%s' already exists (%p) - overwriting with %p", newBehaviorName.c_str(), oldBehavior, newBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we are replacing oldBehavior's map entry
        DeleteBehaviorInternal(oldBehavior);
        newEntry.first->second = newBehavior;
        break;
      }
      case NameCollisionRule::Fail:
      {
        PRINT_NAMED_ERROR("BehaviorFactory.AddToFactory.NameClashFail",
                          "Behavior '%s' already exists (%p) - fail!", newBehaviorName.c_str(), oldBehavior);
        
        // use DeleteBehaviorInternal instead of Destroy - we never added newBehavior to the map
        DeleteBehaviorInternal(newBehavior);
        newBehavior = nullptr;
        break;
      }
    }
  }
  
  return newBehavior;
}


static const char* kBehaviorTypeKey = "behaviorType";
  
  
IBehavior* BehaviorFactory::CreateBehavior(const Json::Value& behaviorJson, Robot& robot, NameCollisionRule nameCollisionRule)
{
  // Find the behavior type
  
  const Json::Value& behaviorTypeJson = behaviorJson[kBehaviorTypeKey];
  const char* behaviorTypeString = behaviorTypeJson.isString() ? behaviorTypeJson.asCString() : "";
  
  const BehaviorType behaviorType = BehaviorTypeFromString(behaviorTypeString);
  
  if (behaviorType == BehaviorType::Count)
  {
    PRINT_NAMED_WARNING("BehaviorFactory.CreateBehavior.UnknownType", "Unknown type '%s'", behaviorTypeString);
    return nullptr;
  }
  
  IBehavior* newBehavior = CreateBehavior(behaviorType, robot, behaviorJson, nameCollisionRule);
  
  return newBehavior;
}
 
  
void BehaviorFactory::DestroyBehavior(IBehavior* behavior)
{
  ASSERT_NAMED_EVENT(behavior->IsOwnedByFactory(),
               "BehaviorFactory.DestroyBehavior", "Attempted to destroy behavior not owned by factory"); // we assume all behaviors are created and owned by factory so this should always be true

  RemoveBehaviorFromMap(behavior);
  DeleteBehaviorInternal(behavior);
}

  
void BehaviorFactory::SafeDestroyBehavior(IBehavior*& behaviorPtrRef)
{
  DestroyBehavior(behaviorPtrRef);
  behaviorPtrRef = nullptr;
}
  

void BehaviorFactory::DeleteBehaviorInternal(IBehavior* behavior)
{
  behavior->_isOwnedByFactory = false;
  delete behavior;
}

  
bool BehaviorFactory::RemoveBehaviorFromMap(IBehavior* behavior)
{
  const auto& it = _nameToBehaviorMap.find(behavior->GetName());
  if (it != _nameToBehaviorMap.end())
  {
    // check it's the same pointer
    IBehavior* existingBehavior = it->second;
    if (existingBehavior == behavior)
    {
      _nameToBehaviorMap.erase(it);
      return true;
    }
  }
  
  return false;
}


IBehavior* BehaviorFactory::FindBehaviorByName(const std::string& inName)
{
  IBehavior* foundBehavior = nullptr;
  
  auto it = _nameToBehaviorMap.find(inName);
  if (it != _nameToBehaviorMap.end())
  {
    foundBehavior = it->second;
  }
  
  return foundBehavior;
}

  
IBehavior* BehaviorFactory::FindBehaviorByType(const BehaviorType& type)
{
  for(const auto behavior : _nameToBehaviorMap)
  {
    if(behavior.second->GetType() == type)
    {
      return behavior.second;
    }
  }
  return nullptr;
}
  
  
IBehavior* BehaviorFactory::FindBehaviorByExecutableType(ExecutableBehaviorType type)
{
  for(const auto behavior : _nameToBehaviorMap)
  {
    if(behavior.second->GetExecutableType() == type)
    {
      return behavior.second;
    }
  }
  return nullptr;
}

  
} // namespace Cozmo
} // namespace Anki

