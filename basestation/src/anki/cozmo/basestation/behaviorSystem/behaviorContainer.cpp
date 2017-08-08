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


#include "anki/cozmo/basestation/behaviorSystem/behaviorContainer.h"

// Behaviors:
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"
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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorSearchForFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorStackBlocks.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/basicWorldInteractions/behaviorTurnToFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/behaviorWait.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorDevTurnInPlaceTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorFactoryTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/behaviorLiftLoadTest.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDriftCheck.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenDriveForwards.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/behaviorPlaypenMotorCalibration.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorCheckForStackAtInterval.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorCubeLiftWorkout.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorDriveInDesperation.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorDriveToFace.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/freeplay/behaviorEarnedSparks.h"
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
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer::BehaviorContainer(Robot& robot)
: _robot(robot)
{
  if(robot.HasExternalInterface()) {
    auto helper = MakeAnkiEventUtil(*robot.GetExternalInterface(), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAllBehaviorsList>();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer::~BehaviorContainer()
{
  // Delete all behaviors owned by the factory
  _idToBehaviorMap.clear();
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
IBehaviorPtr BehaviorContainer::CreateBehavior(const Json::Value& behaviorJson, Robot& robot)
{
  const BehaviorClass behaviorClass = IBehavior::ExtractBehaviorClassFromConfig(behaviorJson);
  IBehaviorPtr newBehavior = CreateBehavior(behaviorClass, robot, behaviorJson);
  return newBehavior;  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorContainer::CreateBehavior(BehaviorClass behaviorType, Robot& robot, const Json::Value& config)
{
  IBehaviorPtr newBehavior;
  
  switch (behaviorType)
  {
    case BehaviorClass::Wait:
    {
      newBehavior = IBehaviorPtr(new BehaviorWait(robot, config));
      break;
    }
    case BehaviorClass::Bouncer:
    {
      newBehavior = IBehaviorPtr(new BehaviorBouncer(robot, config));
      break;
    }
    case BehaviorClass::LookAround:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookAround(robot, config));
      break;
    }
    case BehaviorClass::InteractWithFaces:
    {
      newBehavior = IBehaviorPtr(new BehaviorInteractWithFaces(robot, config));
      break;
    }
    case BehaviorClass::PlayAnim:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimSequence(robot, config));
      break;
    }
    case BehaviorClass::PlayAnimWithFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimSequenceWithFace(robot, config));
      break;
    }
    case BehaviorClass::PlayArbitraryAnim:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayArbitraryAnim(robot, config));
      break;
    }
    case BehaviorClass::PounceOnMotion:
    {
      newBehavior = IBehaviorPtr(new BehaviorPounceOnMotion(robot, config));
      break;
    }
    case BehaviorClass::FindFaces:
    {
      newBehavior = IBehaviorPtr(new BehaviorFindFaces(robot, config));
      break;
    }
    case BehaviorClass::FistBump:
    {
      newBehavior = IBehaviorPtr(new BehaviorFistBump(robot, config));
      break;
    }
    case BehaviorClass::GuardDog:
    {
      newBehavior = IBehaviorPtr(new BehaviorGuardDog(robot, config));
      break;
    }
    case BehaviorClass::RequestGameSimple:
    {
      newBehavior = IBehaviorPtr(new BehaviorRequestGameSimple(robot, config));
      break;
    }
    case BehaviorClass::ExploreLookAroundInPlace:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreLookAroundInPlace(robot, config));
      break;
    }
    case BehaviorClass::ExploreVisitPossibleMarker:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreVisitPossibleMarker(robot, config));
      break;
    }
    case BehaviorClass::BringCubeToBeacon:
    {
      newBehavior = IBehaviorPtr(new BehaviorExploreBringCubeToBeacon(robot, config));
      break;
    }
    case BehaviorClass::LookInPlaceMemoryMap:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookInPlaceMemoryMap(robot, config));
      break;
    }
    case BehaviorClass::ThinkAboutBeacons:
    {
      newBehavior = IBehaviorPtr(new BehaviorThinkAboutBeacons(robot, config));
      break;
    }
    case BehaviorClass::VisitInterestingEdge:
    {
      newBehavior = IBehaviorPtr(new BehaviorVisitInterestingEdge(robot, config));
      break;
    }
    case BehaviorClass::RollBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorRollBlock(robot, config));
      break;
    }
    case BehaviorClass::FactoryTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorFactoryTest(robot, config));
      break;
    }
    case BehaviorClass::FactoryCentroidExtractor:
    {
      newBehavior = IBehaviorPtr(new BehaviorFactoryCentroidExtractor(robot, config));
      break;
    }
    case BehaviorClass::DockingTestSimple:
    {
      newBehavior = IBehaviorPtr(new BehaviorDockingTestSimple(robot, config));
      break;
    }
    case BehaviorClass::SearchForFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorSearchForFace(robot, config));
      break;
    }
    case BehaviorClass::StackBlocks:
    {
      newBehavior = IBehaviorPtr(new BehaviorStackBlocks(robot, config));
      break;
    }
    case BehaviorClass::PutDownBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorPutDownBlock(robot, config));
      break;
    }
    case BehaviorClass::DriveOffCharger:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveOffCharger(robot, config));
      break;
    }
    case BehaviorClass::PopAWheelie:
    {
      newBehavior = IBehaviorPtr(new BehaviorPopAWheelie(robot, config));
      break;
    }
    case BehaviorClass::PeekABoo:
    {
      newBehavior = IBehaviorPtr(new BehaviorPeekABoo(robot, config));
      break;
    }
    case BehaviorClass::DrivePath:
    {
      newBehavior = IBehaviorPtr(new BehaviorDrivePath(robot, config));
      break;
    }
    case BehaviorClass::PickUpCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorPickUpCube(robot, config));
      break;
    }
    case BehaviorClass::PickUpAndPutDownCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorPickUpAndPutDownCube(robot, config));
      break;
    }
    case BehaviorClass::KnockOverCubes:
    {
      newBehavior = IBehaviorPtr(new BehaviorKnockOverCubes(robot, config));
      break;
    }
    case BehaviorClass::BuildPyramid:
    {
      newBehavior = IBehaviorPtr(new BehaviorBuildPyramid(robot, config));
      break;
    }
    case BehaviorClass::BuildPyramidBase:
    {
      newBehavior = IBehaviorPtr(new BehaviorBuildPyramidBase(robot, config));
      break;
    }
    case BehaviorClass::CantHandleTallStack:
    {
      newBehavior = IBehaviorPtr(new BehaviorCantHandleTallStack(robot, config));
      break;
    }
    case BehaviorClass::OnboardingShowCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorOnboardingShowCube(robot, config));
      break;
    }
    case BehaviorClass::OnConfigSeen:
    {
      newBehavior = IBehaviorPtr(new BehaviorOnConfigSeen(robot, config));
      break;
    }
    case BehaviorClass::LookForFaceAndCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorLookForFaceAndCube(robot, config));
      break;
    }
    case BehaviorClass::CubeLiftWorkout:
    {
      newBehavior = IBehaviorPtr(new BehaviorCubeLiftWorkout(robot, config));
      break;
    }
    case BehaviorClass::CheckForStackAtInterval:
    {
      newBehavior = IBehaviorPtr(new BehaviorCheckForStackAtInterval(robot, config));
      break;
    }
    case BehaviorClass::EnrollFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorEnrollFace(robot, config));
      break;
    }
    case BehaviorClass::LiftLoadTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorLiftLoadTest(robot, config));
      break;
    }
    case BehaviorClass::RamIntoBlock:
    {
      newBehavior = IBehaviorPtr(new BehaviorRamIntoBlock(robot, config));
      break;
    }
    case BehaviorClass::RespondPossiblyRoll:
    {
      newBehavior = IBehaviorPtr(new BehaviorRespondPossiblyRoll(robot, config));
      break;
    }
    case BehaviorClass::RespondToRenameFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorRespondToRenameFace(robot, config));
      break;
    }
    case BehaviorClass::PyramidThankYou:
    {
      newBehavior = IBehaviorPtr(new BehaviorPyramidThankYou(robot, config));
      break;
    }
      
    case BehaviorClass::FeedingEat:
    {
      newBehavior = IBehaviorPtr(new BehaviorFeedingEat(robot, config));
      break;
    }
    case BehaviorClass::FeedingSearchForCube:
    {
      newBehavior = IBehaviorPtr(new BehaviorFeedingSearchForCube(robot, config));
      break;
    }
    case BehaviorClass::Singing:
    {
      newBehavior = IBehaviorPtr(new BehaviorSinging(robot, config));
      break;
    }
    case BehaviorClass::TrackLaser:
    {
      newBehavior = IBehaviorPtr(new BehaviorTrackLaser(robot, config));
      break;
    }
    case BehaviorClass::Dance:
    {
      newBehavior = IBehaviorPtr(new BehaviorDance(robot, config));
      break;
    }
    case BehaviorClass::DevTurnInPlaceTest:
    {
      newBehavior = IBehaviorPtr(new BehaviorDevTurnInPlaceTest(robot, config));
      break;
    }
    case BehaviorClass::DriveToFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveToFace(robot, config));
      break;
    }
    case BehaviorClass::EarnedSparks:
    {
      newBehavior = IBehaviorPtr(new BehaviorEarnedSparks(robot, config));
      break;
    }
    case BehaviorClass::TurnToFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorTurnToFace(robot, config));
      break;
    }
    case BehaviorClass::ExpressNeeds:
    {
      newBehavior = IBehaviorPtr(new BehaviorExpressNeeds(robot, config));
      break;
    }
    case BehaviorClass::PlayAnimOnNeedsChange:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlayAnimOnNeedsChange(robot, config));
      break;
    }
    case BehaviorClass::PlaypenCameraCalibration:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlaypenCameraCalibration(robot, config));
      break;
    }
    case BehaviorClass::PlaypenMotorCalibration:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlaypenMotorCalibration(robot, config));
      break;
    }
    case BehaviorClass::PlaypenDriftCheck:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlaypenDriftCheck(robot, config));
      break;
    }
    case BehaviorClass::PlaypenDriveForwards:
    {
      newBehavior = IBehaviorPtr(new BehaviorPlaypenDriveForwards(robot, config));
      break;
    }
    
    ////////////
    // Behaviors that are used by reaction triggers
    ////////////

    case BehaviorClass::ReactToPickup:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPickup(robot, config));
      break;
    }
    case BehaviorClass::ReactToPlacedOnSlope:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPlacedOnSlope(robot, config));
      break;
    }
    case BehaviorClass::ReactToCliff:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToCliff(robot, config));
      break;
    }
    case BehaviorClass::ReactToReturnedToTreads:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToReturnedToTreads(robot, config));
      break;
    }
    case BehaviorClass::ReactToRobotOnBack:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnBack(robot, config));
      break;
    }
    case BehaviorClass::ReactToRobotOnFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnFace(robot, config));
      break;
    }
    case BehaviorClass::ReactToRobotOnSide:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotOnSide(robot, config));
      break;
    }
    case BehaviorClass::ReactToRobotShaken:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToRobotShaken(robot, config));
      break;
    }
    case BehaviorClass::ReactToOnCharger:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToOnCharger(robot, config));
      break;
    }
    case BehaviorClass::AcknowledgeObject:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeObject(robot, config));
      break;
    }
    case BehaviorClass::AcknowledgeFace:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeFace(robot, config));
      break;
    }
    case BehaviorClass::ReactToUnexpectedMovement:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToUnexpectedMovement(robot, config));
      break;
    }
    case BehaviorClass::ReactToMotorCalibration:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToMotorCalibration(robot, config));
      break;
    }
    case BehaviorClass::ReactToCubeMoved:
    {
      newBehavior = IBehaviorPtr(new BehaviorAcknowledgeCubeMoved(robot, config));
      break;
    }
    case BehaviorClass::ReactToFrustration:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToFrustration(robot, config));
      break;
    }
    case BehaviorClass::ReactToSparked:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToSparked(robot, config));
      break;
    }
    case BehaviorClass::ReactToStackOfCubes:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToStackOfCubes(robot, config));
      break;
    }
    case BehaviorClass::ReactToPyramid:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPyramid(robot, config));
      break;
    }
    case BehaviorClass::ReactToPet:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToPet(robot, config));
      break;
    }
    case BehaviorClass::ReactToVoiceCommand:
    {
      newBehavior = IBehaviorPtr(new BehaviorReactToVoiceCommand(robot, config));
      break;
    }
    case BehaviorClass::DriveInDesperation:
    {
      newBehavior = IBehaviorPtr(new BehaviorDriveInDesperation(robot, config));
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
  std::vector<BehaviorID> behaviorList;
  for(const auto& entry : _idToBehaviorMap){
    behaviorList.push_back(entry.first);
  }
  
  ExternalInterface::RespondAllBehaviorsList message(std::move(behaviorList));
  _robot.Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
}

  
} // namespace Cozmo
} // namespace Anki

