/**
 * File: behaviorContainer
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Container which creates and stores behaviors by ID
 * which were generated from data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "engine/aiComponent/behaviorSystem/behaviorContainer.h"

#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
// Behaviors:
#include "engine/aiComponent/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"
#include "engine/aiComponent/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequence.h"
#include "engine/aiComponent/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorDrivePath.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorFindFaces.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorInteractWithFaces.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorKnockOverCubes.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorLookAround.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorPickupCube.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorPopAWheelie.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorPutDownBlock.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorRollBlock.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorSearchForFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorStackBlocks.h"
#include "engine/aiComponent/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/behaviorWait.h"
#include "engine/aiComponent/behaviorSystem/behaviors/devBehaviors/behaviorDevTurnInPlaceTest.h"
#include "engine/aiComponent/behaviorSystem/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "engine/aiComponent/behaviorSystem/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "engine/aiComponent/behaviorSystem/behaviors/devBehaviors/behaviorFactoryTest.h"
#include "engine/aiComponent/behaviorSystem/behaviors/devBehaviors/behaviorLiftLoadTest.h"
#include "engine/aiComponent/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/aiComponent/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorCheckForStackAtInterval.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorCubeLiftWorkout.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorDriveInDesperation.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorDriveToFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorEarnedSparks.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorOnConfigSeen.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/behaviorPickUpAndPutDownCube.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorBuildPyramid.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorBuildPyramidBase.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/buildPyramid/behaviorRespondPossiblyRoll.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreBringCubeToBeacon.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreLookAroundInPlace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorExploreVisitPossibleMarker.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorLookInPlaceMemoryMap.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorThinkAboutBeacons.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/exploration/behaviorVisitInterestingEdge.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/gameRequest/behaviorRequestGameSimple.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorCantHandleTallStack.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorDance.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/oneShots/behaviorSinging.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/putDownDispatch/behaviorLookForFaceAndCube.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorBouncer.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorFistBump.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorGuardDog.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorPeekABoo.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorPounceOnMotion.h"
#include "engine/aiComponent/behaviorSystem/behaviors/freeplay/userInteractive/behaviorTrackLaser.h"
#include "engine/aiComponent/behaviorSystem/behaviors/meetCozmo/behaviorEnrollFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/meetCozmo/behaviorRespondToRenameFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/onboarding/behaviorOnboardingShowCube.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorAcknowledgeFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorAcknowledgeObject.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorRamIntoBlock.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToFrustration.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToOnCharger.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToPet.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToPickup.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToPyramid.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToReturnedToTreads.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnBack.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnFace.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToRobotOnSide.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToRobotShaken.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToSparked.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToStackOfCubes.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToUnexpectedMovement.h"
#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToVoiceCommand.h"

#include "engine/robot.h"

namespace Anki {
namespace Cozmo {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer::BehaviorContainer(const BehaviorIDJsonMap& behaviorData)
{
  for( const auto& behaviorIDJsonPair : behaviorData )
  {
    const auto& behaviorID = behaviorIDJsonPair.first;
    const auto& behaviorJson = behaviorIDJsonPair.second;
    if (!behaviorJson.empty())
    {
      // PRINT_NAMED_DEBUG("BehaviorContainer.Constructor", "Loading '%s'", fullFileName.c_str());
      IBehaviorPtr newBehaviorPtr = CreateBehavior(behaviorJson);
      if ( newBehaviorPtr == nullptr ) {
        PRINT_NAMED_ERROR("Robot.LoadBehavior.CreateFailed",
                          "Failed to create a behavior for behavior id '%s'",
                          BehaviorIDToString(behaviorID));
      }
    }
    else
    {
      PRINT_NAMED_WARNING("Robot.LoadBehavior",
                          "Failed to read behavior file for behavior id '%s'",
                          BehaviorIDToString(behaviorID));
    }
    // don't print anything if we read an empty json
  }
  
  // If we didn't load any behaviors from data, there's no reason to check to
  // see if all executable behaviors have a 1-to-1 matching
  if(behaviorData.size() > 0){
    VerifyExecutableBehaviors();
  }
}


  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer::~BehaviorContainer()
{
  // Delete all behaviors owned by the factory
  _idToBehaviorMap.clear();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorContainer::Init(BehaviorExternalInterface& behaviorExternalInterface,
                             const bool shouldAddToActivatableScope)
{
  auto externalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(externalInterface != nullptr) {
    _robotExternalInterface = externalInterface.get();
    auto helper = MakeAnkiEventUtil((*externalInterface.get()), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAllBehaviorsList>();
  }
  
  for(auto& behaviorMap: _idToBehaviorMap){
    behaviorMap.second->Init(behaviorExternalInterface);
    // To support old behavior manager functionality, have all behaviors be within
    // "activatable" scope since the old behavior manager isn't aware of this state
    if(shouldAddToActivatableScope){
      behaviorMap.second->EnteredActivatableScope();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::FindBehaviorByID(BehaviorID behaviorID) const
{
  IBehaviorPtr foundBehavior = nullptr;
  
  auto scoredIt = _idToBehaviorMap.find(behaviorID);
  if (scoredIt != _idToBehaviorMap.end())
  {
    foundBehavior = scoredIt->second;
    return foundBehavior;
  }
  
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
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
void BehaviorContainer::VerifyExecutableBehaviors() const
{

  std::map< ExecutableBehaviorType, BehaviorID > executableBehaviorMap;
    
  for( const auto& it : _idToBehaviorMap ) {
    IBehaviorPtr behaviorPtr = it.second;
    const ExecutableBehaviorType executableBehaviorType = behaviorPtr->GetExecutableType();
    if( executableBehaviorType != ExecutableBehaviorType::Count )
    {
#if (DEV_ASSERT_ENABLED)
      const auto mapIt = executableBehaviorMap.find(executableBehaviorType);      
      DEV_ASSERT_MSG((mapIt == executableBehaviorMap.end()), "ExecutableBehaviorType.NotUnique",
                     "Multiple behaviors marked as %s including '%s' and '%s'",
                     EnumToString(executableBehaviorType),
                     BehaviorIDToString(it.first),
                     BehaviorIDToString(mapIt->second));
#endif
      executableBehaviorMap[executableBehaviorType] = it.first;
    }
  }
  
  #if (DEV_ASSERT_ENABLED)
    for( size_t i = 0; i < (size_t)ExecutableBehaviorType::Count; ++i)
    {
      const ExecutableBehaviorType executableBehaviorType = (ExecutableBehaviorType)i;
      const auto mapIt = executableBehaviorMap.find(executableBehaviorType);
      DEV_ASSERT_MSG((mapIt != executableBehaviorMap.end()), "ExecutableBehaviorType.NoMapping",
                     "Should be one behavior marked as %s but found none",
                     EnumToString(executableBehaviorType));
    }
  #endif
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::CreateBehavior(const Json::Value& behaviorJson)
{
  const BehaviorClass behaviorClass = IBehavior::ExtractBehaviorClassFromConfig(behaviorJson);
  IBehaviorPtr newBehavior = CreateBehavior(behaviorClass, behaviorJson);
  return newBehavior;  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::CreateBehavior(BehaviorClass behaviorType, const Json::Value& config)
{
  IBehaviorPtr newBehavior;
  
  switch (behaviorType)
  {
    case BehaviorClass::Wait:
    {
      newBehavior = IBehaviorPtr(new BehaviorWait(config));
      break;
    }
    case BehaviorClass::Bouncer:
    {
      newBehavior = IBehaviorPtr(new BehaviorBouncer(config));
      break;
    }
    case BehaviorClass::LookAround:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookAround(config));
      break;
    }
    case BehaviorClass::InteractWithFaces:
    {
      newBehavior = IBehaviorPtr(new BehaviorInteractWithFaces(config));
      break;
    }
    case BehaviorClass::PlayAnim:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimSequence(config));
      break;
    }
    case BehaviorClass::PlayAnimWithFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimSequenceWithFace(config));
      break;
    }
    case BehaviorClass::PlayArbitraryAnim:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayArbitraryAnim(config));
      break;
    }
    case BehaviorClass::PounceOnMotion:
    {
      newBehavior = IBehaviorPtr(new BehaviorPounceOnMotion(config));
      break;
    }
    case BehaviorClass::FindFaces:
    {
      newBehavior = IBehaviorPtr(new BehaviorFindFaces(config));
      break;
    }
    case BehaviorClass::FistBump:
    {
      newBehavior = IBehaviorPtr(new BehaviorFistBump(config));
      break;
    }
    case BehaviorClass::GuardDog:
    {
      newBehavior = IBehaviorPtr(new BehaviorGuardDog(config));
      break;
    }
    case BehaviorClass::RequestGameSimple:
    {
      newBehavior = IBehaviorPtr(new BehaviorRequestGameSimple(config));
      break;
    }
    case BehaviorClass::ExploreLookAroundInPlace:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreLookAroundInPlace(config));
      break;
    }
    case BehaviorClass::ExploreVisitPossibleMarker:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreVisitPossibleMarker(config));
      break;
    }
    case BehaviorClass::BringCubeToBeacon:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreBringCubeToBeacon(config));
      break;
    }
    case BehaviorClass::LookInPlaceMemoryMap:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookInPlaceMemoryMap(config));
      break;
    }
    case BehaviorClass::ThinkAboutBeacons:
    {
      newBehavior = IBehaviorPtr(new BehaviorThinkAboutBeacons(config));
      break;
    }
    case BehaviorClass::VisitInterestingEdge:
    {
      newBehavior = IBehaviorPtr(new BehaviorVisitInterestingEdge(config));
      break;
    }
    case BehaviorClass::RollBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorRollBlock(config));
      break;
    }
    case BehaviorClass::FactoryTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorFactoryTest(config));
      break;
    }
    case BehaviorClass::FactoryCentroidExtractor:
    {
      newBehavior = IBehaviorPtr(new BehaviorFactoryCentroidExtractor(config));
      break;
    }
    case BehaviorClass::DockingTestSimple:
    {
      newBehavior = IBehaviorPtr(new BehaviorDockingTestSimple(config));
      break;
    }
    case BehaviorClass::SearchForFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorSearchForFace(config));
      break;
    }
    case BehaviorClass::StackBlocks:
    {
      newBehavior = IBehaviorPtr(new BehaviorStackBlocks(config));
      break;
    }
    case BehaviorClass::PutDownBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorPutDownBlock(config));
      break;
    }
    case BehaviorClass::DriveOffCharger:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveOffCharger(config));
      break;
    }
    case BehaviorClass::PopAWheelie:
    {
      newBehavior = IBehaviorPtr(new BehaviorPopAWheelie(config));
      break;
    }
    case BehaviorClass::PeekABoo:
    {
      newBehavior = IBehaviorPtr(new BehaviorPeekABoo(config));
      break;
    }
    case BehaviorClass::DrivePath:
    {
      newBehavior = IBehaviorPtr(new BehaviorDrivePath(config));
      break;
    }
    case BehaviorClass::PickUpCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorPickUpCube(config));
      break;
    }
    case BehaviorClass::PickUpAndPutDownCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorPickUpAndPutDownCube(config));
      break;
    }
    case BehaviorClass::KnockOverCubes:
    {
      newBehavior = IBehaviorPtr(new BehaviorKnockOverCubes(config));
      break;
    }
    case BehaviorClass::BuildPyramid:
    {
      newBehavior = IBehaviorPtr(new BehaviorBuildPyramid(config));
      break;
    }
    case BehaviorClass::BuildPyramidBase:
    {
      newBehavior = IBehaviorPtr(new BehaviorBuildPyramidBase(config));
      break;
    }
    case BehaviorClass::CantHandleTallStack:
    {
      newBehavior = IBehaviorPtr(new BehaviorCantHandleTallStack(config));
      break;
    }
    case BehaviorClass::OnboardingShowCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorOnboardingShowCube(config));
      break;
    }
    case BehaviorClass::OnConfigSeen:
    {
      newBehavior = IBehaviorPtr(new BehaviorOnConfigSeen(config));
      break;
    }
    case BehaviorClass::LookForFaceAndCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookForFaceAndCube(config));
      break;
    }
    case BehaviorClass::CubeLiftWorkout:
    {
      newBehavior = IBehaviorPtr(new BehaviorCubeLiftWorkout(config));
      break;
    }
    case BehaviorClass::CheckForStackAtInterval:
    {
      newBehavior = IBehaviorPtr(new BehaviorCheckForStackAtInterval(config));
      break;
    }
    case BehaviorClass::EnrollFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorEnrollFace(config));
      break;
    }
    case BehaviorClass::LiftLoadTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorLiftLoadTest(config));
      break;
    }
    case BehaviorClass::RamIntoBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorRamIntoBlock(config));
      break;
    }
    case BehaviorClass::RespondPossiblyRoll:
    {
      newBehavior = IBehaviorPtr(new BehaviorRespondPossiblyRoll(config));
      break;
    }
    case BehaviorClass::RespondToRenameFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorRespondToRenameFace(config));
      break;
    }
    case BehaviorClass::PyramidThankYou:
    {
      newBehavior = IBehaviorPtr(new BehaviorPyramidThankYou(config));
      break;
    }
      
    case BehaviorClass::FeedingEat:
    {
      newBehavior = IBehaviorPtr(new BehaviorFeedingEat(config));
      break;
    }
    case BehaviorClass::FeedingSearchForCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorFeedingSearchForCube(config));
      break;
    }
    case BehaviorClass::Singing:
    {
      newBehavior = IBehaviorPtr(new BehaviorSinging(config));
      break;
    }
    case BehaviorClass::TrackLaser:
    {
      newBehavior = IBehaviorPtr(new BehaviorTrackLaser(config));
      break;
    }
    case BehaviorClass::Dance:
    {
      newBehavior = IBehaviorPtr(new BehaviorDance(config));
      break;
    }
    case BehaviorClass::DevTurnInPlaceTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorDevTurnInPlaceTest(config));
      break;
    }
    case BehaviorClass::DriveToFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveToFace(config));
      break;
    }
    case BehaviorClass::EarnedSparks:
    {
      newBehavior = IBehaviorPtr(new BehaviorEarnedSparks(config));
      break;
    }
    case BehaviorClass::TurnToFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorTurnToFace(config));
      break;
    }
    case BehaviorClass::ExpressNeeds:
    {
      newBehavior = IBehaviorPtr(new BehaviorExpressNeeds(config));
      break;
    }
    case BehaviorClass::PlayAnimOnNeedsChange:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimOnNeedsChange(config));
      break;
    }
    
    ////////////
    // Behaviors that are used by reaction triggers
    ////////////

    case BehaviorClass::ReactToPickup:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPickup(config));
      break;
    }
    case BehaviorClass::ReactToPlacedOnSlope:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPlacedOnSlope(config));
      break;
    }
    case BehaviorClass::ReactToCliff:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToCliff(config));
      break;
    }
    case BehaviorClass::ReactToReturnedToTreads:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToReturnedToTreads(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnBack:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnBack(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnFace(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnSide:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnSide(config));
      break;
    }
    case BehaviorClass::ReactToRobotShaken:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotShaken(config));
      break;
    }
    case BehaviorClass::ReactToOnCharger:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToOnCharger(config));
      break;
    }
    case BehaviorClass::AcknowledgeObject:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeObject(config));
      break;
    }
    case BehaviorClass::AcknowledgeFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeFace(config));
      break;
    }
    case BehaviorClass::ReactToUnexpectedMovement:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToUnexpectedMovement(config));
      break;
    }
    case BehaviorClass::ReactToMotorCalibration:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToMotorCalibration(config));
      break;
    }
    case BehaviorClass::ReactToCubeMoved:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeCubeMoved(config));
      break;
    }
    case BehaviorClass::ReactToFrustration:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToFrustration(config));
      break;
    }
    case BehaviorClass::ReactToSparked:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToSparked(config));
      break;
    }
    case BehaviorClass::ReactToStackOfCubes:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToStackOfCubes(config));
      break;
    }
    case BehaviorClass::ReactToPyramid:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPyramid(config));
      break;
    }
    case BehaviorClass::ReactToPet:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPet(config));
      break;
    }
    case BehaviorClass::ReactToVoiceCommand:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToVoiceCommand(config));
      break;
    }
    case BehaviorClass::DriveInDesperation:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveInDesperation(config));
      break;
    }
       
  }
  
  if(newBehavior != nullptr){
    newBehavior = AddToFactory(newBehavior);
  }
  
  if (newBehavior == nullptr){
    PRINT_NAMED_ERROR("behaviorContainer.CreateBehavior.Failed",
                      "Failed to create Behavior of type '%s'", BehaviorClassToString(behaviorType));
    return nullptr;
  }
  
  
  return newBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::AddToFactory(IBehaviorPtr newBehavior)
{
  assert(newBehavior);
  
  BehaviorID behaviorID = newBehavior->GetID();
  auto newEntry = _idToBehaviorMap.insert( BehaviorIDToBehaviorMap::value_type(behaviorID, newBehavior) );
  
  const bool addedNewEntry = newEntry.second;

  if (addedNewEntry)
  {
    PRINT_NAMED_INFO("behaviorContainer::AddToFactory", "Added new behavior '%s' %p",
                     BehaviorIDToString(behaviorID), newBehavior.get());
  }
  else
  {
    DEV_ASSERT_MSG(false,
                   "behaviorContainer.AddToFactory.DuplicateID",
                   "Attempted to create a second behavior with id %s",
                   newBehavior->GetIDStr().c_str());
  }
  
  return newBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorContainer::RemoveBehaviorFromMap(IBehaviorPtr behavior)
{
  // check the scored behavior map
  const auto& scoredIt = _idToBehaviorMap.find(behavior->GetID());
  if (scoredIt != _idToBehaviorMap.end())
  {
    // check it's the same pointer
    IBehaviorPtr existingBehavior = scoredIt->second;
    if (existingBehavior == behavior)
    {
      _idToBehaviorMap.erase(scoredIt);
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void BehaviorContainer::HandleMessage(const ExternalInterface::RequestAllBehaviorsList& msg)
{
  if(_robotExternalInterface != nullptr){
    std::vector<BehaviorID> behaviorList;
    for(const auto& entry : _idToBehaviorMap){
      behaviorList.push_back(entry.first);
    }
    ExternalInterface::RespondAllBehaviorsList message(std::move(behaviorList));
    _robotExternalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass BehaviorContainer::GetBehaviorClass(IBehaviorPtr behavior) const
{
  return behavior->GetClass();
}

  
} // namespace Cozmo
} // namespace Anki

