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
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/blockWorld/blockWorldFilter.h"
#include "anki/cozmo/basestation/components/publicStateBroadcaster.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"


#include "clad/vizInterface/messageViz.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace{
#define UPDATE_STAGE(s) UpdateActivityStage(s, #s)

CONSOLE_VAR(float, kTimeSearchForFace, "Activity.Feeding", 5.0f);
static const char* kUniversalChooser = "universalChooser";

  // Special version of CYAN to match feeding animations
static const ColorRGBA FEEDING_CYAN      (0.f, 1.f, 0.7f);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kFeedingActivityAffectedArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kFeedingActivityAffectedArray),
              "Reaction triggers duplicate or non-sequential");
  

constexpr ReactionTriggerHelpers::FullReactionArray kSevereFeedingDisables = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::DoubleTapDetected,            true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           true},
  {ReactionTrigger::VC,                           true}
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kSevereFeedingDisables),
              "Reaction triggers duplicate or non-sequential");
  
  
const char* kSevereFeedingDisableLock = "feeding_severe_disables";
const char* kWaitShakeDASKey = "wait_shake";
const char* kShakeFullDASKey = "shake_full";
const char* kFindCubeDASKey =  "find_cube";
const char* kEatingDASKey =  "eating";
  
} // end namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFeeding::ActivityFeeding(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _chooserStage(FeedingActivityStage::None)
, _lastStageChangeTime_s(0)
, _timeFaceSearchShouldEnd_s(0.0f)
, _eatingComplete(false)
, _severeAnimsSet(false)
, _universalResponseChooser(nullptr)
{
  ////////
  /// Grab behaviors
  ////////
  
  // Get the hunger loop behavior
  _searchForCubeBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FeedingSearchForCube);
  DEV_ASSERT(_searchForCubeBehavior != nullptr &&
             _searchForCubeBehavior->GetClass() == BehaviorClass::FeedingSearchForCube,
             "ActivityFeeding.FeedingSearchForCube.IncorrectBehaviorReceivedFromFactory");
  
  // Get the feeding get in behavior
  IBehaviorPtr eatFoodBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FeedingEat);
  DEV_ASSERT(eatFoodBehavior != nullptr &&
             eatFoodBehavior->GetClass() == BehaviorClass::FeedingEat,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _eatFoodBehavior = std::static_pointer_cast<BehaviorFeedingEat>(eatFoodBehavior);
  DEV_ASSERT(_eatFoodBehavior != nullptr,
             "ActivityFeeding.FeedingFoodReady.DynamicCastFailed");

  // Get the searching for face behavior
  _searchingForFaceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FindFaces_socialize);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _searchingForFaceBehavior_Severe = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FeedingFindFacesSevere);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _turnToFaceBehavior = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::FeedingTurnToFace);
  DEV_ASSERT(_turnToFaceBehavior != nullptr &&
             _turnToFaceBehavior->GetClass() == BehaviorClass::TurnToFace,
             "ActivityFeeding.TurnToFaceBheavior.IncorrectBehaviorReceievedFromFactory");
  
  // Grab the arbitrary animation behavior
  IBehaviorPtr playAnimPtr = robot.GetBehaviorManager().FindBehaviorByID(BehaviorID::PlayArbitraryAnim);
  _behaviorPlayAnimation = std::static_pointer_cast<BehaviorPlayArbitraryAnim>(playAnimPtr);
  
  DEV_ASSERT(_behaviorPlayAnimation != nullptr &&
             _behaviorPlayAnimation->GetClass() == BehaviorClass::PlayArbitraryAnim,
             "SparksBehaviorChooser.BehaviorPlayAnimPointerNotSet");
  
  ////////
  /// Setup UniversalChooser
  ////////
  const Json::Value& universalChooserJSON = config[kUniversalChooser];
  _universalResponseChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, universalChooserJSON);
  DEV_ASSERT(_universalResponseChooser != nullptr, "ActivityFeeding.UniversalChooserNotSpecifed");
  
  ////////
  /// Setup Lights
  ////////
  
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
  SmartDisableReactionsWithLock(robot, GetIDStr(), kFeedingActivityAffectedArray);
  
  NeedId currentSevereExpression = robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression();
  if(currentSevereExpression == NeedId::Energy){
    SetupSevereAnims(robot);
  }else{
    robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(
            BehaviorStageTag::Feeding, static_cast<int>(FeedingStage::MildEnergy));
    _severeAnimsSet = false;
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

  _eventHandlers.push_back(robot.GetExternalInterface()->Subscribe(
     ExternalInterface::MessageEngineToGameTag::ObjectConnectionState,
     [this, &robot] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
     {
       HandleObjectConnectionStateChange(robot, event.GetData().Get_ObjectConnectionState());
     })
  );
  
  
  // DAS Events
  _DASCubesPerFeeding = 0;
  _DASFeedingSessionsPerAppRun++;
  Anki::Util::sEvent("meta.feeding_times_started",
                     {},
                     std::to_string(_DASFeedingSessionsPerAppRun).c_str());

  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const float needLevel = currNeedState.GetNeedLevel(NeedId::Energy);
  Anki::Util::sEvent("meta.feeding_energy_at_start",
                     {},
                     std::to_string(needLevel).c_str());

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnDeselectedInternal(Robot& robot)
{
  if(_severeAnimsSet){
    ClearSevereAnims(robot);
  }
  
  for(auto& entry: _cubeControllerMap){
    using CS = FeedingCubeController::ControllerState;
    entry.second->SetControllerState(robot, CS::Deactivated);
  }
  _cubeControllerMap.clear();
  _eventHandlers.clear();
  
  
  // DAS Events
  const NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsState();
  const float needLevel = currNeedState.GetNeedLevel(NeedId::Energy);
  Anki::Util::sEvent("meta.feeding_energy_at_end",
                     {{DDATA, "energyLevel"}},
                     std::to_string(needLevel).c_str());
  
  Anki::Util::sEvent("meta.feeding_cubes_per_feeding",
                     {},
                     std::to_string(_DASCubesPerFeeding).c_str());
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityFeeding::ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr bestBehavior;
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  // First check for universal responses - eg drive off charger
  if(_universalResponseChooser){
    bestBehavior = _universalResponseChooser->ChooseNextBehavior(robot, currentRunningBehavior);
  }
  
  if(bestBehavior != nullptr){
    return bestBehavior;
  }
  
  // If no universal response, choose the behavior most appropriate to current stage

  switch(_chooserStage){
    case FeedingActivityStage::None:
    {
      if(isNeedSevere){
        bestBehavior = _searchingForFaceBehavior_Severe;
      }else{
        bestBehavior = _searchingForFaceBehavior;
      }
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
        bestBehavior = TransitionToBestActivityStage(robot);
        
        using CS = FeedingCubeController::ControllerState;
        for(auto& entry: _cubeControllerMap){
          entry.second->SetControllerState(robot, CS::Activated);
        }
      }else{
        if(isNeedSevere){
          bestBehavior = _searchingForFaceBehavior_Severe;
        }else{
          bestBehavior = _searchingForFaceBehavior;

        }
      }
      break;
    }
    case FeedingActivityStage::TurnToFace:
    {
      if(_turnToFaceBehavior->IsRunning()){
        bestBehavior = _turnToFaceBehavior;
      }else{
        using CS = FeedingCubeController::ControllerState;
        for(auto& entry: _cubeControllerMap){
          entry.second->SetControllerState(robot, CS::Activated);
        }
        bestBehavior = TransitionToBestActivityStage(robot);
      }
      break;
    }
    case FeedingActivityStage::WaitingForShake:
    {
      // Transition out of this state happens in the update function
      // Ensure play anim is always initialized properly in case  of interrupts
      if(HasSingleBehaviorStageStarted(_behaviorPlayAnimation) &&
         !_behaviorPlayAnimation->IsRunning()){
        bestBehavior = TransitionToBestActivityStage(robot);
      }else{
        bestBehavior = _behaviorPlayAnimation;
      }
      break;
    }
    case FeedingActivityStage::ReactingToShake:
    {
      if(HasSingleBehaviorStageStarted(_behaviorPlayAnimation) &&
         !_behaviorPlayAnimation->IsRunning()){
        UPDATE_STAGE(FeedingActivityStage::WaitingForFullyCharged);
        AnimationTrigger bestAnimation = isNeedSevere ?
                            AnimationTrigger::FeedingIdleWaitForFullCube_Severe :
                            AnimationTrigger::FeedingIdleWaitForFullCube_Normal;
        UpdateAnimationToPlay(bestAnimation);
      }
      bestBehavior = _behaviorPlayAnimation;
      break;
    }
    case FeedingActivityStage::WaitingForFullyCharged:
    {
      // Transition out of this state happens in the update function
      if(HasSingleBehaviorStageStarted(_behaviorPlayAnimation) &&
         !_behaviorPlayAnimation->IsRunning()){
        bestBehavior = TransitionToBestActivityStage(robot);
      }else{
        bestBehavior = _behaviorPlayAnimation;
      }
      break;
    }
    case FeedingActivityStage::ReactingToFullyCharged:
    {
      if(HasSingleBehaviorStageStarted(_behaviorPlayAnimation) &&
         !_behaviorPlayAnimation->IsRunning()){
        UPDATE_STAGE(FeedingActivityStage::SearchingForCube);
        AnimationTrigger bestAnimation = isNeedSevere ?
                            AnimationTrigger::FeedingReactToFullCube_Severe :
                            AnimationTrigger::FeedingReactToFullCube_Normal;
        UpdateAnimationToPlay(bestAnimation);
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
      if(HasSingleBehaviorStageStarted(_behaviorPlayAnimation) &&
         !_behaviorPlayAnimation->IsRunning()){
        // Notify the eat food behavior the ID it should interact with
        BehaviorPreReqAcknowledgeObject preReqData(_interactID, robot);
        if(_eatFoodBehavior->IsRunnable(preReqData)){
          UPDATE_STAGE(FeedingActivityStage::EatFood);
          bestBehavior = _eatFoodBehavior;
        }else{
          bestBehavior = TransitionToBestActivityStage(robot);
        }
      }else{
        bestBehavior = _behaviorPlayAnimation;
      }
      break;
    }
    case FeedingActivityStage::EatFood:
    {
      if(!HasSingleBehaviorStageStarted(_eatFoodBehavior) ||
         _eatFoodBehavior->IsRunning()){
        bestBehavior = _eatFoodBehavior;
      }else{
        // If an attempt was made to eat the cube and it's no longer fully charged
        // clear it out for interaction
        if(_interactID.IsSet() && !_cubeControllerMap[_interactID]->IsCubeCharged()){
          using CS = FeedingCubeController::ControllerState;
          _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Deactivated);
          _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Activated);
          _interactID.UnSet();
        }
        
        bestBehavior = TransitionToBestActivityStage(robot);
      }

      break;
    }
  }
  
  return bestBehavior;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityFeeding::Update(Robot& robot)
{
  NeedId currentSevereExpression = robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression();
  if(!_severeAnimsSet &&
     (currentSevereExpression == NeedId::Energy)){
    SetupSevereAnims(robot);
  }
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  // Update the cube controllers if the activity is past the intro/search for faces
  if(_chooserStage >= FeedingActivityStage::WaitingForShake){
    int countCubesCharged = 0;
    for(auto& entry: _cubeControllerMap){
      entry.second->Update(robot);
      if(entry.second->IsCubeCharged()){
        countCubesCharged++;
      }
    }
    
    // DAS event - track the most cubes charged in parallel each session
    if(countCubesCharged > _DASMostCubesInParallel){
      Anki::Util::sEvent("meta.feeding_max_parallelCubes",
                         {},
                         std::to_string(countCubesCharged).c_str());
      _DASMostCubesInParallel = countCubesCharged;
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
      UpdateAnimationToPlay(bestAnimation);
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
      UpdateAnimationToPlay(bestAnimation);
    }
  }
  
  // Transition out of hunger loop if an interactable object has been located or
  // charged cube has disconnected
  if(_chooserStage == FeedingActivityStage::SearchingForCube){
    bool anyCubesCharged = false;
    for(const auto& entry: _cubeControllerMap){
      anyCubesCharged = anyCubesCharged || entry.second->IsCubeCharged();
    }
    if(!anyCubesCharged){
      TransitionToBestActivityStage(robot);
    }
    
    if(_interactID.IsSet()){
      UPDATE_STAGE(FeedingActivityStage::ReactingToCube);
      AnimationTrigger bestTrigger = isNeedSevere ?
                                       AnimationTrigger::FeedingReactToSeeCube_Severe :
                                       AnimationTrigger::FeedingReactToSeeCube_Normal;
      UpdateAnimationToPlay(bestTrigger);
    }
  }

  return Result::RESULT_OK;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateActivityStage(FeedingActivityStage newStage,
                                  const std::string& newStageName)
{
  
  PRINT_CH_INFO("Feeding",
                "ActivityFeeding.UpdateActivityStage",
                "Entering feeding stage %s",
                newStageName.c_str());
  
  // DAS EVENT
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const float timeInStage = currentTime - _DASTimeLastFeedingStageStarted;
  const bool  firstShakeDetected =
                 (_chooserStage == FeedingActivityStage::WaitingForShake) &&
                 (newStage == FeedingActivityStage::ReactingToShake);
  const bool  shakeFinished =
          (_chooserStage == FeedingActivityStage::WaitingForFullyCharged) &&
          (newStage == FeedingActivityStage::ReactingToFullyCharged);
  const bool  searchFinished =
          (_chooserStage == FeedingActivityStage::SearchingForCube) &&
          (newStage == FeedingActivityStage::ReactingToCube);
  const bool  eatingFinished = (_chooserStage == FeedingActivityStage::EatFood);
  
  const char* previousStageName = nullptr;
  
  if(firstShakeDetected){
    previousStageName = kWaitShakeDASKey;
  }else if(shakeFinished){
    previousStageName = kShakeFullDASKey;
  }else if(searchFinished){
    previousStageName = kFindCubeDASKey;
  }else if(eatingFinished){
    previousStageName = kEatingDASKey;
  }
  
  if(previousStageName != nullptr){
    Anki::Util::sEvent("meta.feeding_stage_time",
                       {{DDATA, previousStageName}},
                       std::to_string(timeInStage).c_str());
  }
  
  _DASTimeLastFeedingStageStarted = currentTime;
  _chooserStage = newStage;
  _lastStageChangeTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityFeeding::TransitionToBestActivityStage(Robot& robot)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);

  // check for cubes that are fully charged or partially charged
  bool anyCubesCharging = false;
  bool anyCubesFullyCharged = false;
  for(const auto& entry: _cubeControllerMap){
    anyCubesFullyCharged = anyCubesFullyCharged || entry.second->IsCubeCharged();
    anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
  }
  
  AnimationTrigger bestAnimation = AnimationTrigger::Count;
  
  if(_severeAnimsSet && !isNeedSevere){
    ClearSevereAnims(robot);
  }
  
  if(anyCubesFullyCharged){
    UPDATE_STAGE(FeedingActivityStage::SearchingForCube);
    if(_interactID.IsSet() &&
       (nullptr == robot.GetBlockWorld().GetLocatedObjectByID(_interactID))){
      _interactID.UnSet();
    }
    return _searchForCubeBehavior;
  }else if(anyCubesCharging){
    UPDATE_STAGE(FeedingActivityStage::WaitingForFullyCharged);
    bestAnimation = isNeedSevere ?
                        AnimationTrigger::FeedingIdleWaitForFullCube_Severe :
                        AnimationTrigger::FeedingIdleWaitForFullCube_Normal;
    
  }else{
    UPDATE_STAGE(FeedingActivityStage::WaitingForShake);
    bestAnimation = isNeedSevere ?
                        AnimationTrigger::FeedingIdleWaitForShake_Severe  :
                        AnimationTrigger::FeedingIdleWaitForShake_Normal;
  }
  
  UpdateAnimationToPlay(bestAnimation);
  return _behaviorPlayAnimation;
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
  _DASCubesPerFeeding++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateAnimationToPlay(AnimationTrigger animTrigger)
{
  // Animations should play once and then allow the activity to re-evaluate
  // what to do next
  const int repetitions = 1;
  const bool animWasPlaying = _behaviorPlayAnimation->IsRunning();
  
  _behaviorPlayAnimation->Stop();
  _behaviorPlayAnimation->SetAnimationTrigger(animTrigger, repetitions);
  
  // If the behavior stays the same the behavior manager has no way to know that
  // it needs to be re-initialized, so initialize here
  if(animWasPlaying){
    _behaviorPlayAnimation->Init();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::HandleObjectConnectionStateChange(Robot& robot, const ObjectConnectionState& connectionState)
{
  using CS = FeedingCubeController::ControllerState;
  const ObjectID& objID = connectionState.objectID;
  auto mapObjIter = _cubeControllerMap.find(objID);
  
  
  if(connectionState.connected){
    const auto& emplaceRes = _cubeControllerMap.emplace(
            std::make_pair(objID,
                            std::make_unique<FeedingCubeController>(robot,
                                                                    objID)));
    
    // Activate the cube if emplace succeeded and
    // past the appropriate activity stage
    if(emplaceRes.second &&
       (_chooserStage >= FeedingActivityStage::WaitingForShake)){
      mapObjIter = emplaceRes.first;
      mapObjIter->second->SetControllerState(robot, CS::Activated);
    }
  }else{
    if(mapObjIter != _cubeControllerMap.end()){
      mapObjIter->second->SetControllerState(robot,CS::Deactivated);
      _cubeControllerMap.erase(objID);
    }
    
    if(_interactID.GetValue() == connectionState.objectID){
      _interactID.UnSet();
    }
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityFeeding::HasSingleBehaviorStageStarted(IBehaviorPtr behavior)
{
  if(behavior != nullptr){
    return behavior->GetTimeStartedRunning_s() >= _lastStageChangeTime_s;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::SetupSevereAnims(Robot& robot)
{
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(
       BehaviorStageTag::Feeding, static_cast<int>(FeedingStage::SevereEnergy));
  SmartDisableReactionsWithLock(robot, kSevereFeedingDisableLock, kSevereFeedingDisables);
  _severeAnimsSet = true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::ClearSevereAnims(Robot& robot)
{
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(
       BehaviorStageTag::Feeding, static_cast<int>(FeedingStage::MildEnergy));
  SmartRemoveDisableReactionsLock(robot, kSevereFeedingDisableLock);
  _severeAnimsSet = false;
}


} // namespace Cozmo
} // namespace Anki
