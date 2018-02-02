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


#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"

#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"

#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimOnNeedsChange.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequence.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequenceWithFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorPlayAnimSequenceWithObject.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDriveOffCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorDrivePath.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindFaces.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindHome.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorGoHome.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorInteractWithFaces.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorKnockOverCubes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorLookAround.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPickupCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPopAWheelie.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorPutDownBlock.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRequestToGoHome.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorRollBlock.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorSearchForFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorStackBlocks.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorTurn.h"
#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorTurnToFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorHighLevelAI.h"
#include "engine/aiComponent/behaviorComponent/behaviors/behaviorWait.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevImageCapture.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevPettingTestSimple.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDevTurnInPlaceTest.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDispatchAfterShake.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorDockingTestSimple.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorFactoryCentroidExtractor.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorLiftLoadTest.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenCameraCalibration.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDistanceSensor.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDriftCheck.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenDriveForwards.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenEndChecks.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenInitChecks.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenMotorCalibration.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenPickupCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenReadToolCode.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenSoundCheck.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenTest.h"
#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/playpen/behaviorPlaypenWaitToStart.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherQueue.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRandom.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherRerun.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherScoring.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherStrictPriority.h"
#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherStrictPriorityWithCooldown.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/aiComponent/behaviorComponent/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorCheckForStackAtInterval.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorCubeLiftWorkout.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorDriveInDesperation.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorDriveToFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorEarnedSparks.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorOnConfigSeen.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/behaviorPickUpAndPutDownCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorBuildPyramid.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorBuildPyramidBase.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorPyramidThankYou.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/buildPyramid/behaviorRespondPossiblyRoll.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorExploreBringCubeToBeacon.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorExploreLookAroundInPlace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorExploreVisitPossibleMarker.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorLookInPlaceMemoryMap.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorThinkAboutBeacons.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/exploration/behaviorVisitInterestingEdge.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/gameRequest/behaviorRequestGameSimple.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorCantHandleTallStack.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorDance.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorExpressNeeds.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/oneShots/behaviorSinging.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/putDownDispatch/behaviorLookForFaceAndCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorBouncer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorFistBump.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorGuardDog.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPeekABoo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPounceOnMotion.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPounceWithProx.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorPuzzleMaze.h"
#include "engine/aiComponent/behaviorComponent/behaviors/freeplay/userInteractive/behaviorTrackLaser.h"
#include "engine/aiComponent/behaviorComponent/behaviors/meetCozmo/behaviorEnrollFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/meetCozmo/behaviorRespondToRenameFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/observing/behaviorObservingLookAtFaces.h"
#include "engine/aiComponent/behaviorComponent/behaviors/observing/behaviorObservingOnCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingShowCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/proxBehaviors/behaviorProxGetToDistance.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeCubeMoved.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorAcknowledgeObject.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorRamIntoBlock.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToCliff.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToFrustration.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToMotorCalibration.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToOnCharger.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPet.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPickup.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPlacedOnSlope.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPyramid.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToReturnedToTreads.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnBack.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnFace.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotOnSide.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToRobotShaken.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToSparked.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToStackOfCubes.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToUnexpectedMovement.h"
#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToVoiceCommand.h"
#include "engine/aiComponent/behaviorComponent/behaviors/sleeping/behaviorSleeping.h"
#include "engine/aiComponent/behaviorComponent/behaviors/victor/behaviorComeHere.h"

#include "engine/robot.h"

#include "clad/types/behaviorComponent/behaviorTypes.h"

#define LOG_CHANNEL    "Behaviors"

namespace Anki {
namespace Cozmo {
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorContainer::BehaviorContainer(const BehaviorIDJsonMap& behaviorData)
: IDependencyManagedComponent<BCComponentID>(BCComponentID::BehaviorContainer)
{
  for( const auto& behaviorIDJsonPair : behaviorData )
  {
    const auto& behaviorID = behaviorIDJsonPair.first;
    const auto& behaviorJson = behaviorIDJsonPair.second;
    if (!behaviorJson.empty())
    {
      // PRINT_NAMED_DEBUG("BehaviorContainer.Constructor", "Loading '%s'", fullFileName.c_str());
      ICozmoBehaviorPtr newBehaviorPtr = CreateBehaviorFromConfig(behaviorJson);
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
void BehaviorContainer::InitDependent(Robot* robot, const BCCompMap& dependentComponents)
{
  auto& bei = dependentComponents.find(BCComponentID::BehaviorExternalInterface)->second.GetValue<BehaviorExternalInterface>();
  Init(bei);
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorContainer::Init(BehaviorExternalInterface& behaviorExternalInterface)
{
  /**auto externalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(externalInterface != nullptr) {
    _robotExternalInterface = externalInterface.get();
    auto helper = MakeAnkiEventUtil((*externalInterface.get()), *this, _signalHandles);
    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAllBehaviorsList>();
  }**/
  
  for(auto& behaviorMap: _idToBehaviorMap){
    behaviorMap.second->Init(behaviorExternalInterface);
  }

  for(auto& behaviorMap: _idToBehaviorMap){
    behaviorMap.second->InitBehaviorOperationModifiers();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::FindBehaviorByID(BehaviorID behaviorID) const
{
  ICozmoBehaviorPtr foundBehavior = nullptr;
  
  auto scoredIt = _idToBehaviorMap.find(behaviorID);
  if (scoredIt != _idToBehaviorMap.end())
  {
    foundBehavior = scoredIt->second;
    return foundBehavior;
  }
  
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  for (const auto & behavior : _idToBehaviorMap)
  {
    if (behavior.second->GetExecutableType() == type)
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
    ICozmoBehaviorPtr behaviorPtr = it.second;
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
ICozmoBehaviorPtr BehaviorContainer::CreateBehaviorFromConfig(const Json::Value& behaviorJson)
{
  const BehaviorClass behaviorClass = ICozmoBehavior::ExtractBehaviorClassFromConfig(behaviorJson);
  ICozmoBehaviorPtr newBehavior = CreateBehaviorAndAddToContainer(behaviorClass, behaviorJson);
  return newBehavior;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::CreateBehaviorAndAddToContainer(BehaviorClass behaviorType, const Json::Value& config)
{
  ICozmoBehaviorPtr newBehavior = CreateBehaviorBase(behaviorType, config);  
  if(newBehavior != nullptr){
    newBehavior = AddToContainer(newBehavior);
  }
  return newBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::CreateAnonymousBehavior(BehaviorClass behaviorType, const Json::Value& config) const
{
  ICozmoBehaviorPtr newBehavior = CreateBehaviorBase(behaviorType, config);
  return newBehavior;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::CreateBehaviorBase(BehaviorClass behaviorType, const Json::Value& config) const
{
  ICozmoBehaviorPtr newBehavior;

  switch (behaviorType)
  {
    case BehaviorClass::Wait:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorWait(config));
      break;
    }
    case BehaviorClass::Bouncer:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorBouncer(config));
      break;
    }
    case BehaviorClass::LookAround:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorLookAround(config));
      break;
    }
    case BehaviorClass::InteractWithFaces:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorInteractWithFaces(config));
      break;
    }
    case BehaviorClass::PlayAnim:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlayAnimSequence(config));
      break;
    }
    case BehaviorClass::PlayAnimWithFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlayAnimSequenceWithFace(config));
      break;
    }
    case BehaviorClass::PlayAnimWithObject:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlayAnimSequenceWithObject(config));
      break;
    }
    case BehaviorClass::PounceOnMotion:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPounceOnMotion(config));
      break;
    }
    case BehaviorClass::FindFaces:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFindFaces(config));
      break;
    }
    case BehaviorClass::FistBump:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFistBump(config));
      break;
    }
    case BehaviorClass::GuardDog:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorGuardDog(config));
      break;
    }
    case BehaviorClass::RequestGameSimple:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRequestGameSimple(config));
      break;
    }
    case BehaviorClass::ExploreLookAroundInPlace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorExploreLookAroundInPlace(config));
      break;
    }
    case BehaviorClass::ExploreVisitPossibleMarker:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorExploreVisitPossibleMarker(config));
      break;
    }
    case BehaviorClass::BringCubeToBeacon:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorExploreBringCubeToBeacon(config));
      break;
    }
    case BehaviorClass::LookInPlaceMemoryMap:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorLookInPlaceMemoryMap(config));
      break;
    }
    case BehaviorClass::ThinkAboutBeacons:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorThinkAboutBeacons(config));
      break;
    }
    case BehaviorClass::VisitInterestingEdge:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorVisitInterestingEdge(config));
      break;
    }
    case BehaviorClass::RollBlock:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRollBlock(config));
      break;
    }
    case BehaviorClass::FactoryCentroidExtractor:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFactoryCentroidExtractor(config));
      break;
    }
    case BehaviorClass::DockingTestSimple:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDockingTestSimple(config));
      break;
    }
    case BehaviorClass::SearchForFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorSearchForFace(config));
      break;
    }
    case BehaviorClass::StackBlocks:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorStackBlocks(config));
      break;
    }
    case BehaviorClass::PutDownBlock:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPutDownBlock(config));
      break;
    }
    case BehaviorClass::DriveOffCharger:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDriveOffCharger(config));
      break;
    }
    case BehaviorClass::FindHome:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFindHome(config));
      break;
    }
    case BehaviorClass::GoHome:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorGoHome(config));
      break;
    }
    case BehaviorClass::RequestToGoHome:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRequestToGoHome(config));
      break;
    }
    case BehaviorClass::PopAWheelie:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPopAWheelie(config));
      break;
    }
    case BehaviorClass::PeekABoo:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPeekABoo(config));
      break;
    }
    case BehaviorClass::DrivePath:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDrivePath(config));
      break;
    }
    case BehaviorClass::PickUpCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPickUpCube(config));
      break;
    }
    case BehaviorClass::PickUpAndPutDownCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPickUpAndPutDownCube(config));
      break;
    }
    case BehaviorClass::KnockOverCubes:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorKnockOverCubes(config));
      break;
    }
    case BehaviorClass::BuildPyramid:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorBuildPyramid(config));
      break;
    }
    case BehaviorClass::BuildPyramidBase:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorBuildPyramidBase(config));
      break;
    }
    case BehaviorClass::CantHandleTallStack:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorCantHandleTallStack(config));
      break;
    }
    case BehaviorClass::OnboardingShowCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorOnboardingShowCube(config));
      break;
    }
    case BehaviorClass::OnConfigSeen:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorOnConfigSeen(config));
      break;
    }
    case BehaviorClass::LookForFaceAndCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorLookForFaceAndCube(config));
      break;
    }
    case BehaviorClass::CubeLiftWorkout:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorCubeLiftWorkout(config));
      break;
    }
    case BehaviorClass::CheckForStackAtInterval:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorCheckForStackAtInterval(config));
      break;
    }
    case BehaviorClass::EnrollFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorEnrollFace(config));
      break;
    }
    case BehaviorClass::LiftLoadTest:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorLiftLoadTest(config));
      break;
    }
    case BehaviorClass::RamIntoBlock:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRamIntoBlock(config));
      break;
    }
    case BehaviorClass::RespondPossiblyRoll:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRespondPossiblyRoll(config));
      break;
    }
    case BehaviorClass::RespondToRenameFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorRespondToRenameFace(config));
      break;
    }
    case BehaviorClass::PyramidThankYou:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPyramidThankYou(config));
      break;
    }
      
    case BehaviorClass::FeedingEat:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFeedingEat(config));
      break;
    }
    case BehaviorClass::FeedingSearchForCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorFeedingSearchForCube(config));
      break;
    }
    case BehaviorClass::Singing:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorSinging(config));
      break;
    }
    case BehaviorClass::TrackLaser:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorTrackLaser(config));
      break;
    }
    case BehaviorClass::Dance:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDance(config));
      break;
    }
    case BehaviorClass::DevTurnInPlaceTest:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDevTurnInPlaceTest(config));
      break;
    }
    case BehaviorClass::DriveToFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDriveToFace(config));
      break;
    }
    case BehaviorClass::EarnedSparks:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorEarnedSparks(config));
      break;
    }
    case BehaviorClass::Turn:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorTurn(config));
      break;
    }
    case BehaviorClass::TurnToFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorTurnToFace(config));
      break;
    }
    case BehaviorClass::ExpressNeeds:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorExpressNeeds(config));
      break;
    }
    case BehaviorClass::PlayAnimOnNeedsChange:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlayAnimOnNeedsChange(config));
      break;
    }
      
    case BehaviorClass::PuzzleMaze:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPuzzleMaze(config));
      break;
    }





    case BehaviorClass::PlaypenCameraCalibration:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenCameraCalibration(config));
      break;
    }
    case BehaviorClass::PlaypenDistanceSensor:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenDistanceSensor(config));
      break;
    }
    case BehaviorClass::PlaypenDriftCheck:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenDriftCheck(config));
      break;
    }
    case BehaviorClass::PlaypenDriveForwards:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenDriveForwards(config));
      break;
    }
    case BehaviorClass::PlaypenInitChecks:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenInitChecks(config));
      break;
    }
    case BehaviorClass::PlaypenEndChecks:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenEndChecks(config));
      break;
    }
    case BehaviorClass::PlaypenMotorCalibration:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenMotorCalibration(config));
      break;
    }
    case BehaviorClass::PlaypenPickupCube:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenPickupCube(config));
      break;
    }
    case BehaviorClass::PlaypenReadToolCode:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenReadToolCode(config));
      break;
    }
    case BehaviorClass::PlaypenSoundCheck:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenSoundCheck(config));
      break;
    }
    case BehaviorClass::PlaypenTest:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenTest(config));
      break;
    }
    case BehaviorClass::PlaypenWaitToStart:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPlaypenWaitToStart(config));
      break;
    }


    case BehaviorClass::DevPettingTestSimple:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDevPettingTestSimple(config));
      break;
    }

    case BehaviorClass::PounceWithProx:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorPounceWithProx(config));
      break;
    }
    case BehaviorClass::ProxGetToDistance:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorProxGetToDistance(config));
      break;
    }

    // Dispatch Behvaiors
    case BehaviorClass::BehaviorDispatcherRerun:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherRerun(config));
      break;
    }
    case BehaviorClass::DispatcherStrictPriority:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherStrictPriority(config));
      break;
    }
    case BehaviorClass::DispatcherStrictPriorityWithCooldown:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherStrictPriorityWithCooldown(config));
      break;
    }
    case BehaviorClass::DispatcherScoring:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherScoring(config));
      break;
    }
    case BehaviorClass::DispatcherQueue:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherQueue(config));
      break;
    }
    case BehaviorClass::DispatcherRandom:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatcherRandom(config));
      break;
    }
    case BehaviorClass::ComeHere:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorComeHere(config));
      break;
    }
    
    ////////////
    // Behaviors that are used by reaction triggers
    ////////////

    case BehaviorClass::ReactToPickup:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToPickup(config));
      break;
    }
    case BehaviorClass::ReactToPlacedOnSlope:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToPlacedOnSlope(config));
      break;
    }
    case BehaviorClass::ReactToCliff:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToCliff(config));
      break;
    }
    case BehaviorClass::ReactToReturnedToTreads:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToReturnedToTreads(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnBack:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToRobotOnBack(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToRobotOnFace(config));
      break;
    }
    case BehaviorClass::ReactToRobotOnSide:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToRobotOnSide(config));
      break;
    }
    case BehaviorClass::ReactToRobotShaken:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToRobotShaken(config));
      break;
    }
    case BehaviorClass::ReactToOnCharger:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToOnCharger(config));
      break;
    }
    case BehaviorClass::AcknowledgeObject:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorAcknowledgeObject(config));
      break;
    }
    case BehaviorClass::AcknowledgeFace:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorAcknowledgeFace(config));
      break;
    }
    case BehaviorClass::ReactToUnexpectedMovement:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToUnexpectedMovement(config));
      break;
    }
    case BehaviorClass::ReactToMotorCalibration:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToMotorCalibration(config));
      break;
    }
    case BehaviorClass::ReactToCubeMoved:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorAcknowledgeCubeMoved(config));
      break;
    }
    case BehaviorClass::ReactToFrustration:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToFrustration(config));
      break;
    }
    case BehaviorClass::ReactToSparked:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToSparked(config));
      break;
    }
    case BehaviorClass::ReactToStackOfCubes:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToStackOfCubes(config));
      break;
    }
    case BehaviorClass::ReactToPyramid:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToPyramid(config));
      break;
    }
    case BehaviorClass::ReactToPet:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToPet(config));
      break;
    }
    case BehaviorClass::ReactToVoiceCommand:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorReactToVoiceCommand(config));
      break;
    }
    case BehaviorClass::DriveInDesperation:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDriveInDesperation(config));
      break;
    }
    case BehaviorClass::DevImageCapture:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDevImageCapture(config));
      break;
    }

    case BehaviorClass::HighLevelAI:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorHighLevelAI(config));
      break;
    }
    case BehaviorClass::ObservingLookAtFaces:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorObservingLookAtFaces(config));
      break;
    }
    case BehaviorClass::Sleeping:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorSleeping(config));
      break;
    }
    case BehaviorClass::ObservingOnCharger:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorObservingOnCharger(config));
      break;
    }
    case BehaviorClass::DispatchAfterShake:
    {
      newBehavior = ICozmoBehaviorPtr(new BehaviorDispatchAfterShake(config));
      break;
    }
  }
  
  if (newBehavior == nullptr){
    PRINT_NAMED_ERROR("BehaviorContainer.CreateBehavior.Failed",
                      "Failed to create Behavior of type '%s'", BehaviorClassToString(behaviorType));
  }
  
  return newBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorContainer::AddToContainer(ICozmoBehaviorPtr newBehavior)
{
  assert(newBehavior);
  
  BehaviorID behaviorID = newBehavior->GetID();
  auto newEntry = _idToBehaviorMap.insert( BehaviorIDToBehaviorMap::value_type(behaviorID, newBehavior) );
  
  const bool addedNewEntry = newEntry.second;

  if (addedNewEntry)
  {
    PRINT_CH_DEBUG(LOG_CHANNEL, "BehaviorContainer::AddToContainer",
                   "Added new behavior '%s' %p",
                   BehaviorIDToString(behaviorID), newBehavior.get());
  }
  else
  {
    DEV_ASSERT_MSG(false,
                   "BehaviorContainer.AddToContainer.DuplicateID",
                   "Attempted to create a second behavior with id %s",
                   newBehavior->GetIDStr().c_str());
  }
  
  return newBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorContainer::RemoveBehaviorFromMap(ICozmoBehaviorPtr behavior)
{
  // check the scored behavior map
  const auto& scoredIt = _idToBehaviorMap.find(behavior->GetID());
  if (scoredIt != _idToBehaviorMap.end())
  {
    // check it's the same pointer
    ICozmoBehaviorPtr existingBehavior = scoredIt->second;
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
  /**if(_robotExternalInterface != nullptr){
    std::vector<BehaviorID> behaviorList;
    for(const auto& entry : _idToBehaviorMap){
      behaviorList.push_back(entry.first);
    }
    ExternalInterface::RespondAllBehaviorsList message(std::move(behaviorList));
    _robotExternalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }**/
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass BehaviorContainer::GetBehaviorClass(ICozmoBehaviorPtr behavior) const
{
  return behavior->GetClass();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorContainer::GetIDString(BehaviorID behaviorID) const
{
  return BehaviorIDToString(behaviorID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string BehaviorContainer::GetClassString(BehaviorClass behaviorClass) const
{
  return BehaviorClassToString(behaviorClass);
}

  
} // namespace Cozmo
} // namespace Anki

