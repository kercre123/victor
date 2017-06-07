/**
 * File: activityFeeding.cpp
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for building a pyramid
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/activityFeeding.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/activeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/drivingAnimationHandler.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace{
#define UPDATE_STAGE(s) UpdateActivityStage(s, #s)

CONSOLE_VAR(float, kTimeSearchForFace, "Behavior.Feeding", 5.0f);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct BackpackLightMapping{
  BackpackLightMapping(){};
  BackpackLightMapping(const BackpackLights& waitingForFoodBackpackLights,
                       const BackpackLights& eatingBackpackLights,
                       const BackpackLights& solidBackpackLights)
  : _waitingForFoodBackpackLights(waitingForFoodBackpackLights)
  , _eatingBackpackLights(eatingBackpackLights)
  , _solidBackpackLights(solidBackpackLights){}
  BackpackLights       _waitingForFoodBackpackLights;
  BackpackLights       _eatingBackpackLights;
  BackpackLights       _solidBackpackLights;
};
  
BackpackLightMapping kFeedingBackpackLights;
  
} // end namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFeeding::ActivityFeeding(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _timeFaceSearchShouldEnd_s(0.0f)
, _eatingComplete(false)
, _idleAndDrivingSet(false)
{
  ////////
  /// Grab behaviors
  ////////
  
  // Get the hunger loop behavior
  _searchForCubeBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FeedingSearchForCube);
  DEV_ASSERT(_searchForCubeBehavior != nullptr &&
             _searchForCubeBehavior->GetClass() == BehaviorClass::FeedingSearchForCube,
             "ActivityFeeding.FeedingSearchForCube.IncorrectBehaviorReceivedFromFactory");
  
  // Get the feeding get in behavior
  IBehavior* eatFoodBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FeedingEat);
  DEV_ASSERT(eatFoodBehavior != nullptr &&
             eatFoodBehavior->GetClass() == BehaviorClass::FeedingEat,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _eatFoodBehavior = dynamic_cast<BehaviorFeedingEat*>(eatFoodBehavior);
  DEV_ASSERT(_eatFoodBehavior != nullptr,
             "ActivityFeeding.FeedingFoodReady.DynamicCastFailed");

  // Get the searching for face behavior
  _searchingForFaceBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FindFaces_socialize);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _turnToFaceBehavior = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FeedingTurnToFace);
  DEV_ASSERT(_turnToFaceBehavior != nullptr &&
             _turnToFaceBehavior->GetClass() == BehaviorClass::TurnToFace,
             "ActivityFeeding.TurnToFaceBheavior.IncorrectBehaviorReceievedFromFactory");
  
  //Create an arbitrary animation behavior
  Json::Value basicConfig = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::PlayArbitraryAnim);
  _behaviorPlayAnimation = dynamic_cast<BehaviorPlayArbitraryAnim*>(
                                                                    robot.GetBehaviorFactory().CreateBehavior(
                                                                      BehaviorClass::PlayArbitraryAnim, robot, basicConfig));
  DEV_ASSERT(_behaviorPlayAnimation, "SparksBehaviorChooser.BehaviorPlayAnimPointerNotSet");
  
  ////////
  /// Setup Lights
  ////////

  
  // These have to be defined within the constructor to allow NamedColors to be used
  static const BackpackLights kPulsingCyanBackpack = {
    .onColors               = {{NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{1,1,1,1,1}},
    .offPeriod_ms           = {{1,1,1,1,1}},
    .transitionOnPeriod_ms  = {{2000,2000,2000,2000,2000}},
    .transitionOffPeriod_ms = {{2000,2000,2000,2000,2000}},
    .offset                 = {{0,0,0,0,0}}
  };
  
  static const BackpackLights kLoopingCyanBackpack = {
    .onColors               = {{NamedColors::BLACK, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::BLACK}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{0,360,360,360,0}},
    .offPeriod_ms           = {{0,1110,1110,1110,0}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,240,120,0,0}}
  };
  
  static const BackpackLights kSolidCyanBackpack = {
    .onColors               = {{NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN, NamedColors::CYAN}},
    .offColors              = {{NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK, NamedColors::BLACK}},
    .onPeriod_ms            = {{1000,1000,1000,1000,1000}},
    .offPeriod_ms           = {{0,0,0,0,0}},
    .transitionOnPeriod_ms  = {{0,0,0,0,0}},
    .transitionOffPeriod_ms = {{0,0,0,0,0}},
    .offset                 = {{0,0,0,0,0}}
  };
  
  kFeedingBackpackLights = BackpackLightMapping(kPulsingCyanBackpack,
                                               kLoopingCyanBackpack,
                                               kSolidCyanBackpack);
  
  // Make the activity a listener for the eating behavior so that it's notified
  // when the eating process starts
  _eatFoodBehavior->AddListener(static_cast<IFeedingListener*>(this));
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFeeding::~ActivityFeeding()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnSelectedInternal(Robot& robot)
{
  _eatingComplete = false;
  _chooserStage = FeedingActivityStage::None;
  SmartDisableReactionsWithLock(robot, GetIDStr(), ReactionTriggerHelpers::kAffectAllArray);
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  if(currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical)){
    robot.GetDrivingAnimationHandler().PushDrivingAnimations({AnimationTrigger::FeedingDrivingGetIn_Severe,
      AnimationTrigger::FeedingDrivingLoop_Severe,
      AnimationTrigger::FeedingDrivingGetOut_Severe});
    robot.GetAnimationStreamer().PushIdleAnimation(AnimationTrigger::FeedingIdleToFullCube_Severe);
    _idleAndDrivingSet = true;
  }else{
    _idleAndDrivingSet = false;
  }
  
  // Set up _cubeControllerMap with all connected cubes
  {
    for(auto& entry: _cubeControllerMap){
      using CS = FeedingCubeController::ControllerState;
      entry.second->SetControllerState(robot, CS::Deactivated);
    }
    _cubeControllerMap.clear();
    
    std::vector<const ActiveObject*> connectedObjs;
    BlockWorldFilter filter;
    filter.SetAllowedFamilies(
                              {{ObjectFamily::LightCube, ObjectFamily::Block}});
    robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjs);
    
    for(auto& entry: connectedObjs){
      _cubeControllerMap.emplace(std::make_pair(
                            entry->GetID(),
                            std::make_unique<FeedingCubeController>(robot, entry->GetID())));
    }
  }
  
  _eventHandlers.push_back(robot.GetExternalInterface()->Subscribe(
      ExternalInterface::MessageEngineToGameTag::RobotObservedObject,
      [this] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
      {
        RobotObservedObject(event.GetData().Get_RobotObservedObject().objectID);
      })
  );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnDeselectedInternal(Robot& robot)
{
  if(_idleAndDrivingSet){
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    robot.GetAnimationStreamer().PopIdleAnimation();
  }
  
  for(auto& entry: _cubeControllerMap){
    using CS = FeedingCubeController::ControllerState;
    entry.second->SetControllerState(robot, CS::Deactivated);
  }
  _cubeControllerMap.clear();
  _eventHandlers.clear();
  
  robot.GetBodyLightComponent().StopLoopingBackpackLights(_bodyLightDataLocator);
  _bodyLightDataLocator = {};
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* ActivityFeeding::ChooseNextBehaviorInternal(Robot& robot, const IBehavior* currentRunningBehavior)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  IBehavior* bestBehavior = nullptr;
  switch(_chooserStage){
    case FeedingActivityStage::None:
    {
      bestBehavior = _searchingForFaceBehavior;
      UPDATE_STAGE(FeedingActivityStage::SearchForFace);
      
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _timeFaceSearchShouldEnd_s = currentTime_s + kTimeSearchForFace;
      break;
    }
    case FeedingActivityStage::SearchForFace:
    {
      BehaviorPreReqRobot preReqData(robot);
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_turnToFaceBehavior->IsRunnable(preReqData)){
        UPDATE_STAGE(FeedingActivityStage::TurnToFace);
        bestBehavior = _turnToFaceBehavior;
      }else if(currentTime_s > _timeFaceSearchShouldEnd_s){
        TransitionToBestActivityStage(robot);
        bestBehavior = _behaviorPlayAnimation;
        
        using CS = FeedingCubeController::ControllerState;
        for(auto& entry: _cubeControllerMap){
          entry.second->SetControllerState(robot, CS::Activated);
        }
      }else{
        bestBehavior = _searchingForFaceBehavior;
      }
      break;
    }
    case FeedingActivityStage::TurnToFace:
    {
      if(_turnToFaceBehavior->IsRunning()){
        bestBehavior = _turnToFaceBehavior;
      }else{
        TransitionToBestActivityStage(robot);
        using CS = FeedingCubeController::ControllerState;
        for(auto& entry: _cubeControllerMap){
          entry.second->SetControllerState(robot, CS::Activated);
        }
        bestBehavior = _behaviorPlayAnimation;
      }
      break;
    }
    case FeedingActivityStage::WaitingForShake:
    {
      // Transition out of this state happens in the update function
      bestBehavior = _behaviorPlayAnimation;
      break;
    }
    case FeedingActivityStage::ReactingToShake:
    {
      if(!_behaviorPlayAnimation->IsRunning()){
        UPDATE_STAGE(FeedingActivityStage::WaitingForFullyCharged);
        AnimationTrigger bestAnimation = isNeedSevere ?
                            AnimationTrigger::FeedingIdleToFullCube_Severe :
                            AnimationTrigger::FeedingIdleToFullCube_Normal;
        UpdateAnimationToPlay(bestAnimation, 0);
      }
      bestBehavior = _behaviorPlayAnimation;
      break;
    }
    case FeedingActivityStage::WaitingForFullyCharged:
    {
      // Transition out of this state happens in the update function
      bestBehavior = _behaviorPlayAnimation;
      break;
    }
    case FeedingActivityStage::ReactingToFullyCharged:
    {
      if(!_behaviorPlayAnimation->IsRunning()){
        UPDATE_STAGE(FeedingActivityStage::SearchingForCube);
        AnimationTrigger bestAnimation = isNeedSevere ?
                            AnimationTrigger::FeedingReactToFullCube_Severe :
                            AnimationTrigger::FeedingIdleToFullCube_Normal;
        UpdateAnimationToPlay(bestAnimation, 0);
      }
      bestBehavior = _behaviorPlayAnimation;
      break;
    }
    case FeedingActivityStage::SearchingForCube:
    {
      // Transition out of hunger loop happens in the Update() function
      // when a cube is seen
      bestBehavior = _searchForCubeBehavior;
      break;
    }
    case FeedingActivityStage::ReactingToCube:
    {
      if(!_behaviorPlayAnimation->IsRunning()){
        // Notify the eat food behavior the ID it should interact with
        UPDATE_STAGE(FeedingActivityStage::EatFood);
        BehaviorPreReqAcknowledgeObject preReqData(_interactID);
        ANKI_VERIFY(_eatFoodBehavior->IsRunnable(preReqData),
                    "FeedingActivity.EatFood.NotRunnable",
                    "Eating behavior isn't runnable - feeding will get stuck in infinet loop");
        bestBehavior = _eatFoodBehavior;
      }else{
        bestBehavior = _behaviorPlayAnimation;
      }
      break;
    }
    case FeedingActivityStage::EatFood:
    {
      if(_eatFoodBehavior->IsRunning()){
        bestBehavior = _eatFoodBehavior;
      }else{
        TransitionToBestActivityStage(robot);
        bestBehavior = _behaviorPlayAnimation;
      }

      break;
    }
  }
  
  return bestBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityFeeding::Update(Robot& robot)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  // Update the cube controllers if the activity is past the intro/search for faces
  if(_chooserStage >= FeedingActivityStage::WaitingForShake){
    for(auto& entry: _cubeControllerMap){
      entry.second->Update(robot);
    }
  }
  
  if(_chooserStage == FeedingActivityStage::WaitingForShake){
    bool anyCubesCharging = false;
    for(const auto& entry: _cubeControllerMap){
      anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
    }
    if(anyCubesCharging){
      UPDATE_STAGE(FeedingActivityStage::ReactingToShake);
      AnimationTrigger bestAnimation = isNeedSevere ?
                          AnimationTrigger::FeedingReactToShake_Severe :
                          AnimationTrigger::FeedingReactToShake_Normal;
      UpdateAnimationToPlay(bestAnimation, 1);
    }
  }
  
  if(_chooserStage == FeedingActivityStage::WaitingForFullyCharged){
    bool anyCubesCharged = false;
    for(const auto& entry: _cubeControllerMap){
      anyCubesCharged = anyCubesCharged || entry.second->IsCubeCharged();
    }
    if(anyCubesCharged){
      UPDATE_STAGE(FeedingActivityStage::ReactingToFullyCharged);
      AnimationTrigger bestAnimation = isNeedSevere ?
                          AnimationTrigger::FeedingReactToFullCube_Severe :
                          AnimationTrigger::FeedingReactToFullCube_Normal;
      UpdateAnimationToPlay(bestAnimation, 1);
    }
  }
  
  // Transition out of hunger loop if an interactable object has been located
  if(_chooserStage == FeedingActivityStage::SearchingForCube){
    if(_interactID.IsSet()){
      UPDATE_STAGE(FeedingActivityStage::ReactingToCube);
      AnimationTrigger bestTrigger = isNeedSevere ?
                                       AnimationTrigger::FeedingReactToSeeCube_Severe :
                                       AnimationTrigger::FeedingReactToSeeCube_Normal;
      UpdateAnimationToPlay(bestTrigger, 1);
    }
  }
  
  return Result::RESULT_OK;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateActivityStage(FeedingActivityStage newStage,
                                  const std::string& stageName)
{
  PRINT_CH_INFO("Feeding",
                "ActivityFeeding.UpdateActivityStage",
                "Entering feeding stage %s",
                stageName.c_str());
  _chooserStage = newStage;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::TransitionToBestActivityStage(Robot& robot)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  if(_interactID.IsSet()){
    // Clear all data about the cube that we just interacted with
    using CS = FeedingCubeController::ControllerState;
    _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Deactivated);
    _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Activated);
    _interactID.UnSet();
  }
  
  // check for cubes that are fully charged or partially charged
  bool anyCubesCharging = false;
  bool anyCubesFullyCharged = false;
  for(const auto& entry: _cubeControllerMap){
    anyCubesFullyCharged = anyCubesFullyCharged || entry.second->IsCubeCharged();
    anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
  }
  
  AnimationTrigger bestAnimation = isNeedSevere ?
                      AnimationTrigger::FeedingIdleToFullCube_Severe :
                      AnimationTrigger::FeedingIdleToFullCube_Normal;
  UpdateAnimationToPlay(bestAnimation, 0);
  
  if(anyCubesFullyCharged){
    UPDATE_STAGE(FeedingActivityStage::SearchingForCube);
  }else if(anyCubesCharging){
    UPDATE_STAGE(FeedingActivityStage::WaitingForFullyCharged);
  }else{
    UPDATE_STAGE(FeedingActivityStage::WaitingForShake);
  }

  
  auto& bodyLightComp = robot.GetBodyLightComponent();
  bodyLightComp.StartLoopingBackpackLights(kFeedingBackpackLights._waitingForFoodBackpackLights,
                                           BackpackLightSource::Behavior,
                                           _bodyLightDataLocator);
  
  if(_idleAndDrivingSet && !isNeedSevere){
    robot.GetDrivingAnimationHandler().PopDrivingAnimations();
    robot.GetAnimationStreamer().PopIdleAnimation();
    _idleAndDrivingSet = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::RobotObservedObject(const ObjectID& objID)
{
  if((_chooserStage == FeedingActivityStage::SearchingForCube) &&
     !_interactID.IsSet()){
    auto entry = _cubeControllerMap.find(objID);
    if(entry != _cubeControllerMap.end()){
      if(entry->second->IsCubeCharged()){
        _interactID = objID;
      }
    }
  }
}


// Implementation of IFeedingListener
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::StartedEating(Robot& robot, const int duration_s)
{
  using CS = FeedingCubeController::ControllerState;
  _cubeControllerMap[_interactID]->SetControllerState(robot, CS::DrainCube, duration_s);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateAnimationToPlay(AnimationTrigger animTrigger, int repetitions)
{
  _behaviorPlayAnimation->Stop();
  _behaviorPlayAnimation->SetAnimationTrigger(animTrigger, repetitions);
  _behaviorPlayAnimation->Init();
}

  
  
} // namespace Cozmo
} // namespace Anki
