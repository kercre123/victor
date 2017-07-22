/**
 * File: activityFeeding.cpp
 *
 * Author: Kevin M. Karol
 * Created: 04/27/17
 *
 * Description: Activity for feeding
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
#define UPDATE_STAGE(s) UpdateActivityStage(FeedingActivityStage::s, #s)

CONSOLE_VAR(float, kTimeSearchForFace, "Activity.Feeding", 5.0f);
static const char* kUniversalChooser = "universalChooser";

  // Special version of CYAN to match feeding animations
static const ColorRGBA FEEDING_CYAN      (0.f, 1.f, 0.7f);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kFeedingActivityAffectedArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
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
, _chooserStage(FeedingActivityStage::SearchForFace)
, _lastStageChangeTime_s(0)
, _timeFaceSearchShouldEnd_s(FLT_MAX)
, _eatingComplete(false)
, _severeAnimsSet(false)
, _usingLookingUpIdle(false)
, _universalResponseChooser(nullptr)
{
  ////////
  /// Grab behaviors
  ////////
  
  const auto& BM = robot.GetBehaviorManager();
  // Get the hunger loop behavior
  {
    IBehaviorPtr searchBehavior = BM.FindBehaviorByID(BehaviorID::FeedingSearchForCube);
    _searchForCubeBehavior = std::static_pointer_cast<BehaviorFeedingSearchForCube>(searchBehavior);
    DEV_ASSERT(_searchForCubeBehavior != nullptr &&
               _searchForCubeBehavior->GetClass() == BehaviorClass::FeedingSearchForCube,
               "ActivityFeeding.FeedingSearchForCube.IncorrectBehaviorReceivedFromFactory");
  }

  // Get the feeding get in behavior
  IBehaviorPtr eatFoodBehavior = BM.FindBehaviorByID(BehaviorID::FeedingEat);
  DEV_ASSERT(eatFoodBehavior != nullptr &&
             eatFoodBehavior->GetClass() == BehaviorClass::FeedingEat,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _eatFoodBehavior = std::static_pointer_cast<BehaviorFeedingEat>(eatFoodBehavior);
  DEV_ASSERT(_eatFoodBehavior != nullptr,
             "ActivityFeeding.FeedingFoodReady.DynamicCastFailed");

  // Get the searching for face behavior
  _searchingForFaceBehavior = BM.FindBehaviorByID(BehaviorID::FindFaces_socialize);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _searchingForFaceBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingFindFacesSevere);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FeedingFoodReady.IncorrectBehaviorReceivedFromFactory");
  
  _turnToFaceBehavior = BM.FindBehaviorByID(BehaviorID::FeedingTurnToFace);
  DEV_ASSERT(_turnToFaceBehavior != nullptr &&
             _turnToFaceBehavior->GetClass() == BehaviorClass::TurnToFace,
             "ActivityFeeding.TurnToFaceBehavior.IncorrectBehaviorReceivedFromFactory");
  
  // TODO: Add DEV_ASSERTS to all these behaviors
  
  _waitBehavior = BM.FindBehaviorByID(BehaviorID::Wait);
  
  _reactCubeShakeBehavior = BM.FindBehaviorByID(BehaviorID::FeedingReactCubeShake);
  _reactCubeShakeBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactCubeShake_Severe);
  
  _reactFullCubeBehavior = BM.FindBehaviorByID(BehaviorID::FeedingReactFullCube);
  _reactFullCubeBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactFullCube_Severe);
  
  _reactSeeCharged = BM.FindBehaviorByID(BehaviorID::FeedingReactSeeCharged);
  _reactSeeCharged_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactSeeCharged_Severe);
  
  ////////
  /// Setup UniversalChooser
  ////////
  const Json::Value& universalChooserJSON = config[kUniversalChooser];
  _universalResponseChooser.reset(BehaviorChooserFactory::CreateBehaviorChooser(robot, universalChooserJSON));
  DEV_ASSERT(_universalResponseChooser != nullptr, "ActivityFeeding.UniversalChooserNotSpecified");
  
  ////////
  /// Setup Lights
  ////////
  
  // Make the activity a listener for the eating behavior so that it's notified
  // when the eating process starts
  _eatFoodBehavior->AddListener(static_cast<IFeedingListener*>(this));
  
  /// Setup choose behavior map
  _stageToBehaviorMap = {
    {FeedingActivityStage::SearchForFace,                 _searchingForFaceBehavior},
    {FeedingActivityStage::SearchForFace_Severe,          _searchingForFaceBehavior_Severe},
    {FeedingActivityStage::TurnToFace,                    _turnToFaceBehavior},
    {FeedingActivityStage::WaitingForShake,               _waitBehavior},
    {FeedingActivityStage::ReactingToShake,               _reactCubeShakeBehavior},
    {FeedingActivityStage::ReactingToShake_Severe,        _reactCubeShakeBehavior_Severe},
    {FeedingActivityStage::WaitingForFullyCharged,        _waitBehavior},
    {FeedingActivityStage::ReactingToFullyCharged,        _reactFullCubeBehavior},
    {FeedingActivityStage::ReactingToFullyCharged_Severe, _reactFullCubeBehavior_Severe},
    {FeedingActivityStage::SearchingForCube,              std::static_pointer_cast<IBehavior>(_searchForCubeBehavior)},
    {FeedingActivityStage::ReactingToSeeCharged,         _reactSeeCharged},
    {FeedingActivityStage::ReactingToSeeCharged_Severe,  _reactSeeCharged_Severe},
    {FeedingActivityStage::EatFood,                       std::static_pointer_cast<IBehavior>(_eatFoodBehavior)}
  };
  
  DEV_ASSERT(_stageToBehaviorMap.size() == (static_cast<int>(FeedingActivityStage::EatFood) + 1),
             "AcitivityFeeding.Constructor.StageBehaviorSizeMismatch");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFeeding::~ActivityFeeding()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnSelectedInternal(Robot& robot)
{
  _eatingComplete = false;
  _timeFaceSearchShouldEnd_s = FLT_MAX;
  _usingLookingUpIdle = false;
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
  
  // Setup messages to listen for
  _eventHandlers.push_back(robot.GetExternalInterface()->Subscribe(
      ExternalInterface::MessageEngineToGameTag::RobotObservedObject,
      [this, &robot] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
      {
        RobotObservedObject(event.GetData().Get_RobotObservedObject().objectID, robot);
      })
  );

  _eventHandlers.push_back(robot.GetExternalInterface()->Subscribe(
     ExternalInterface::MessageEngineToGameTag::ObjectConnectionState,
     [this, &robot] (const AnkiEvent<ExternalInterface::MessageEngineToGame>& event)
     {
       HandleObjectConnectionStateChange(robot, event.GetData().Get_ObjectConnectionState());
     })
  );
  
  // Start the feeding cube controllers
  using CS = FeedingCubeController::ControllerState;
  for(auto& entry: _cubeControllerMap){
    entry.second->SetControllerState(robot, CS::Activated);
  }
  
  
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
  
  // Select the best activity stage
  _chooserStage = FeedingActivityStage::SearchForFace;
  TransitionToBestActivityStage(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnDeselectedInternal(Robot& robot)
{
  if(_severeAnimsSet){
    ClearSevereAnims(robot);
  }
  
  robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
  
  for(auto& entry: _cubeControllerMap){
    using CS = FeedingCubeController::ControllerState;
    entry.second->SetControllerState(robot, CS::Deactivated);
  }
  _cubeControllerMap.clear();
  _eventHandlers.clear();
  
  // Clear the public state broadcaster
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  
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

  // First check for universal responses - eg drive off charger
  if(_universalResponseChooser){
    bestBehavior = _universalResponseChooser->ChooseNextBehavior(robot, currentRunningBehavior);
  }
  
  if(bestBehavior != nullptr){
    return bestBehavior;
  }

  const auto& iter = _stageToBehaviorMap.find(_chooserStage);
  if(iter == _stageToBehaviorMap.end()){
    return bestBehavior;
  }
  IBehaviorPtr desiredBehavior = iter->second;
  
  // Since transitioning to new stages generally occurs in the Update function
  // check whether the desired behavior has started running - when it's run
  // at least one tick and stops running re-evaluate the best stage to be at
  if(HasSingleBehaviorStageStarted(desiredBehavior) &&
     !desiredBehavior->IsRunning()){
    return TransitionToBestActivityStage(robot);
  }else{
    return desiredBehavior;
  }
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityFeeding::Update(Robot& robot)
{
  // Maintain appropriate music state and disables
  NeedId currentSevereExpression = robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression();
  if(!_severeAnimsSet &&
     (currentSevereExpression == NeedId::Energy)){
    SetupSevereAnims(robot);
  }else if(_severeAnimsSet &&
           (currentSevereExpression != NeedId::Energy)){
    ClearSevereAnims(robot);
  }
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  // Maintain the appropriate idle
  if((_chooserStage == FeedingActivityStage::WaitingForFullyCharged) &&
     !_usingLookingUpIdle){
    
    AnimationTrigger idleAnim = AnimationTrigger::FeedingIdleWaitForFullCube_Normal;
    if(isNeedSevere){
      idleAnim = AnimationTrigger::FeedingIdleWaitForFullCube_Severe;
    }
    
    robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
    robot.GetAnimationStreamer().PushIdleAnimation(idleAnim, GetIDStr());
    _usingLookingUpIdle = true;
  }else if((_chooserStage != FeedingActivityStage::WaitingForFullyCharged) &&
           _usingLookingUpIdle){
    AnimationTrigger idleAnim = AnimationTrigger::FeedingIdleWaitForShake_Normal;
    if(isNeedSevere){
      idleAnim = AnimationTrigger::FeedingIdleWaitForShake_Severe;
    }
    
    robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
    robot.GetAnimationStreamer().PushIdleAnimation(idleAnim, GetIDStr());
    _usingLookingUpIdle = false;
  }
  
  
  // Keep track of cubes charged in parallel for DAS tracking
  {
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

  
  ///////
  // Check for interrupt conditions
  ///////
  
  // If we've set the interact ID, make sure we're in feeding state
  if(_interactID.IsSet() &&
     _chooserStage < FeedingActivityStage::ReactingToSeeCharged){
    if(isNeedSevere){
      UPDATE_STAGE(ReactingToSeeCharged_Severe);
    }else{
      UPDATE_STAGE(ReactingToSeeCharged);
    }
  }
  
  {
    // Determine whether any cubes are charged or charging
    bool anyCubesToSearchFor = false;
    bool anyCubesCharged = false;
    bool anyCubesCharging = false;
    for(const auto& entry: _cubeControllerMap){
      const bool alreadySearchedForCube =
             (_cubesSearchCouldntFind.find(entry.first) != _cubesSearchCouldntFind.end());
      anyCubesToSearchFor |= (entry.second->IsCubeCharged() && !alreadySearchedForCube);
      anyCubesCharged = anyCubesCharged || entry.second->IsCubeCharged();
      anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
    }
    
    // When to react to shaking at any point before we start searching
    if(_chooserStage <= FeedingActivityStage::WaitingForShake){
      if(anyCubesCharging){
        if(isNeedSevere){
          UPDATE_STAGE(ReactingToShake_Severe);
        }else{
          UPDATE_STAGE(ReactingToShake);
        }
      }
    }
    
    
    if(_chooserStage == FeedingActivityStage::WaitingForFullyCharged){
      if(anyCubesToSearchFor){
        if(isNeedSevere){
          UPDATE_STAGE(ReactingToFullyCharged_Severe);
        }else{
          UPDATE_STAGE(ReactingToFullyCharged);
        }
      }else if(!anyCubesCharging){
        UPDATE_STAGE(WaitingForShake);
      }
    }
    
    // Conditions that can break searching for food
    if(_chooserStage == FeedingActivityStage::SearchingForCube){
      // if charged cube disconnected, transition out of search
      if(!anyCubesCharged){
        TransitionToBestActivityStage(robot);
      }
      
      if(_interactID.IsSet()){
        if(isNeedSevere){
          UPDATE_STAGE(ReactingToSeeCharged_Severe);
        }else{
          UPDATE_STAGE(ReactingToSeeCharged);
        }
      }else if(!_searchForCubeBehavior->IsRunning()){
        // if search wants to end, mark as cube search failed and transition out
        _cubesSearchCouldntFind.insert(_searchingForID);
        _searchingForID.UnSet();
      }
    }
    
  }
  
  // Reset eat the interacting cube if eating was interrupted
  if((_chooserStage == FeedingActivityStage::EatFood) &&
      HasSingleBehaviorStageStarted(_eatFoodBehavior) &&
      !_eatFoodBehavior->IsRunning()){
    _cubesSearchCouldntFind.erase(_interactID);
    // If an attempt was made to eat the cube and it's no longer fully charged
    // clear it out for interaction
    if(!_cubeControllerMap[_interactID]->IsCubeCharged()){
      using CS = FeedingCubeController::ControllerState;
      _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Deactivated);
      _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Activated);
      _interactID.UnSet();
    }
  }
  
  return Result::RESULT_OK;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityFeeding::TransitionToBestActivityStage(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
  // Transition into turn to face if one is available
  if(_chooserStage < FeedingActivityStage::TurnToFace){
    BehaviorPreReqRobot preReqData(robot);
    if(_turnToFaceBehavior->IsRunnable(preReqData)){
      UPDATE_STAGE(TurnToFace);
      return _turnToFaceBehavior;
    }
  }
  
  // Transition to search for face if we've just been selected and don't
  // want to jump to later in the process
  if((_timeFaceSearchShouldEnd_s == FLT_MAX) &&
     (_chooserStage < FeedingActivityStage::SearchForFace_Severe)){
    // Setup inital search for face
    if(isNeedSevere){
      UPDATE_STAGE(SearchForFace_Severe);
      return _searchingForFaceBehavior_Severe;
    }else{
      UPDATE_STAGE(SearchForFace);
      return _searchForCubeBehavior;
    }
  }
  // Ensure we only search for face OnSelected
  _timeFaceSearchShouldEnd_s = currentTime_s + kTimeSearchForFace;
  
  
  //Jump to appropriate waiting/animation stage based on cube charge state
  // check for cubes that are fully charged or partially charged
  bool anyCubesCharging = false;
  for(const auto& entry: _cubeControllerMap){
    const bool alreadySearchedForCube =
           (_cubesSearchCouldntFind.find(entry.first) != _cubesSearchCouldntFind.end());
    if(entry.second->IsCubeCharged() && !alreadySearchedForCube){
      _searchingForID = entry.first;
    }
    anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
  }
  
  if(_searchingForID.IsSet()){
    UPDATE_STAGE(SearchingForCube);
    if(_interactID.IsSet() &&
       (nullptr == robot.GetBlockWorld().GetLocatedObjectByID(_interactID))){
      _interactID.UnSet();
    }
    return _searchForCubeBehavior;
  }else if(anyCubesCharging){
    UPDATE_STAGE(WaitingForFullyCharged);
    return _waitBehavior;
  }else{
    UPDATE_STAGE(WaitingForShake);
    return _waitBehavior;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::RobotObservedObject(const ObjectID& objID, Robot& robot)
{
  if(!_interactID.IsSet()){
    auto entry = _cubeControllerMap.find(objID);
    if(entry != _cubeControllerMap.end()){
      if(entry->second->IsCubeCharged()){
        _interactID = objID;
        // Pass the interactID into the eat food behavior so that it knows the
        // cube to eat
        BehaviorPreReqAcknowledgeObject preReqData(_interactID, robot);
        _eatFoodBehavior->IsRunnable(preReqData);
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
void ActivityFeeding::EatingInterrupted(Robot& robot)
{
  using CS = FeedingCubeController::ControllerState;
  DEV_ASSERT(_cubeControllerMap[_interactID]->GetControllerState() == CS::DrainCube,
             "ActivityFeeding.EatingInterrupted.CubeNotDraining");
  _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Deactivated);
  _cubeControllerMap[_interactID]->SetControllerState(robot, CS::Activated);
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
  (newStage == FeedingActivityStage::ReactingToSeeCharged);
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
