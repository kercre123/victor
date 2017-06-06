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
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingHungerLoop.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace{
#define UPDATE_STAGE(s) UpdateActivityStage(s, #s)

CONSOLE_VAR(float, kTimeSearchForFace, "Behavior.Feeding", 5.0f);
CONSOLE_VAR(int, kSuccessfullFeedingsToEnd, "Behavior.Feeding", 3);

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
, _currentSuccessfullFoodCount(0)
, _eatingComplete(false)
{
  ////////
  /// Grab behaviors
  ////////
  
  // Get the hunger loop behavior
  IBehavior* hungerLoop = robot.GetBehaviorFactory().FindBehaviorByID(BehaviorID::FeedingHungerLoop);
  DEV_ASSERT(hungerLoop != nullptr &&
             hungerLoop->GetClass() == BehaviorClass::FeedingHungerLoop,
             "ActivityFeeding.FeedingHungerLoop.IncorrectBehaviorReceivedFromFactory");
  
  _hungerLoopBehavior = dynamic_cast<BehaviorFeedingHungerLoop*>(hungerLoop);
  DEV_ASSERT(_hungerLoopBehavior != nullptr,
             "ActivityFeeding.FeedingHungerLoop.DynamicCastFailed");
  
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
  _currentSuccessfullFoodCount = 0;
  _eatingComplete = false;
  _chooserStage = FeedingActivityStage::None;
  SmartDisableReactionsWithLock(robot, GetIDStr(), ReactionTriggerHelpers::kAffectAllArray);
  
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
IBehavior* ActivityFeeding::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  IBehavior* bestBehavior = nullptr;
  switch(_chooserStage){
    case FeedingActivityStage::None:
    {
      _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::FeedingGetIn, 1);
      bestBehavior = _behaviorPlayAnimation;
      UPDATE_STAGE(FeedingActivityStage::FeedingGetIn);
      break;
    }
    case FeedingActivityStage::FeedingGetIn:
    {
      if(_behaviorPlayAnimation->IsRunning()){
        bestBehavior = _behaviorPlayAnimation;
      }else{
        bestBehavior = _searchingForFaceBehavior;
        UPDATE_STAGE(FeedingActivityStage::SearchForFace);
        
        const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
        _timeFaceSearchShouldEnd_s = currentTime_s + kTimeSearchForFace;
      }
      
      break;
    }
    case FeedingActivityStage::SearchForFace:
    {
      const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      if(_hungerLoopBehavior->HasUsableFace(robot) ||
         (currentTime_s > _timeFaceSearchShouldEnd_s)){
        TransitionIntoHungerLoop(robot);
        bestBehavior = _hungerLoopBehavior;
        using CS = FeedingCubeController::ControllerState;
        for(auto& entry: _cubeControllerMap){
          entry.second->SetControllerState(robot, CS::Activated);
        }
      }else{
        bestBehavior = _searchingForFaceBehavior;
      }
      break;
    }
    case FeedingActivityStage::HungerLoop:
    {
      // Transition out of hunger loop happens in the Update() function
      // when a cube is seen
      bestBehavior = _hungerLoopBehavior;
      break;
    }
    case FeedingActivityStage::EatFood:
    {
      // Notify the eat food behavior the ID it should interact with if the behavior
      // hasn't started yet
      if(!_eatFoodBehavior->IsRunning()){
        BehaviorPreReqAcknowledgeObject preReqData(_interactID);
        ANKI_VERIFY(_eatFoodBehavior->IsRunnable(preReqData),
                    "FeedingActivity.EatFood.NotRunnable",
                    "Eating behavior isn't runnable - feeding will get stuck in infinet loop");
      }
      bestBehavior = _eatFoodBehavior;
      break;
    }
    case FeedingActivityStage::TransitionCelebrateFood:
    {
      // Transition into this state happens in the Update() function
      // when the cube is fully drained
      auto& bodyLightComp = robot.GetBodyLightComponent();
      bodyLightComp.StartLoopingBackpackLights(kFeedingBackpackLights._solidBackpackLights,
                                               BackpackLightSource::Behavior,
                                               _bodyLightDataLocator);
      robot.SetCarriedObjectAsUnattached();
      _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::WorkoutPutDown_highEnergy, 1);
      bestBehavior = _behaviorPlayAnimation;
      UPDATE_STAGE(FeedingActivityStage::CelebrateFood);
      break;
    }
    case FeedingActivityStage::CelebrateFood:
    {
      if(_behaviorPlayAnimation->IsRunning()){
        bestBehavior = _behaviorPlayAnimation;
      }else{
        // do another feeding
        TransitionIntoHungerLoop(robot);
        bestBehavior = _hungerLoopBehavior;
      }
      break;
    }
    case FeedingActivityStage::FeedingGetOut:
    {
      // Transition into this state happens in the Update() function
      // when it's determined that Cozmo has eaten all the cubes he needs
      auto& bodyLightComp = robot.GetBodyLightComponent();
      bodyLightComp.StartLoopingBackpackLights(kFeedingBackpackLights._solidBackpackLights,
                                               BackpackLightSource::Behavior,
                                               _bodyLightDataLocator);
      _behaviorPlayAnimation->SetAnimationTrigger(AnimationTrigger::BuildPyramidSuccess, 1);
      bestBehavior = _behaviorPlayAnimation;
      UPDATE_STAGE(FeedingActivityStage::Done);
      break;
    }
    case FeedingActivityStage::Done:
    {
      if(!_behaviorPlayAnimation->IsRunning()){
        bestBehavior = nullptr;
      }else{
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
  // Update the cube controllers if the activity is past the intro/search for faces
  if(_chooserStage >= FeedingActivityStage::HungerLoop){
    for(auto& entry: _cubeControllerMap){
      entry.second->Update(robot);
    }
  }
  
  // Transition out of hunger loop if an interactable object has been located
  if(_chooserStage == FeedingActivityStage::HungerLoop){
    if(_interactID.IsSet()){
      UPDATE_STAGE(FeedingActivityStage::EatFood);
    }
  }
  
  // Transition out of eating food if the cube has been drained while eating food
  if((_chooserStage == FeedingActivityStage::EatFood) &&
     _cubeControllerMap[_interactID]->IsCubeDrained()){
    _currentSuccessfullFoodCount++;
    
    if(_currentSuccessfullFoodCount >= kSuccessfullFeedingsToEnd){
      UPDATE_STAGE(FeedingActivityStage::FeedingGetOut);
    }else{
      UPDATE_STAGE(FeedingActivityStage::TransitionCelebrateFood);
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
void ActivityFeeding::TransitionIntoHungerLoop(Robot& robot)
{
  if(_interactID.IsSet()){
    // Clear all data about the cube that we just interacted with
    using CS = FeedingCubeController::ControllerState;
    _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Deactivated);
    _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Activated);
    _interactID.UnSet();
  }
  
  UPDATE_STAGE(FeedingActivityStage::HungerLoop);
  auto& bodyLightComp = robot.GetBodyLightComponent();
  bodyLightComp.StartLoopingBackpackLights(kFeedingBackpackLights._waitingForFoodBackpackLights,
                                           BackpackLightSource::Behavior,
                                           _bodyLightDataLocator);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::RobotObservedObject(const ObjectID& objID)
{
  if((_chooserStage == FeedingActivityStage::HungerLoop) &&
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
void ActivityFeeding::StartedEating(Robot& robot)
{
  auto& bodyLightComp = robot.GetBodyLightComponent();
  bodyLightComp.StartLoopingBackpackLights(kFeedingBackpackLights._eatingBackpackLights,
                                           BackpackLightSource::Behavior,
                                           _bodyLightDataLocator);
  using CS = FeedingCubeController::ControllerState;
  _cubeControllerMap[_interactID]->SetControllerState(robot, CS::DrainCube);
}
  
  
} // namespace Cozmo
} // namespace Anki
