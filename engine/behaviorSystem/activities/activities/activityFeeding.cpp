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

#include "engine/behaviorSystem/activities/activities/activityFeeding.h"

#include "anki/common/basestation/utils/timer.h"

#include "engine/activeObject.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/behaviorSystem/bsRunnableChoosers/bsRunnableChooserFactory.h"
#include "engine/behaviorSystem/bsRunnableChoosers/iBSRunnableChooser.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/behaviors/animationWrappers/behaviorPlayArbitraryAnim.h"
#include "engine/behaviorSystem/behaviors/feeding/behaviorFeedingEat.h"
#include "engine/behaviorSystem/behaviors/feeding/behaviorFeedingSearchForCube.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/blockWorld/blockWorldFilter.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/needsSystem/needsState.h"
#include "engine/robot.h"


#include "clad/vizInterface/messageViz.h"
#include "util/console/consoleInterface.h"
#include "engine/faceWorld.h"

namespace Anki {
namespace Cozmo {

namespace{
#define SET_STAGE(r, s) SetActivityStage(r, FeedingActivityStage::s, #s)

CONSOLE_VAR(float, kTimeSearchForFace, "Activity.Feeding", 5.0f);
CONSOLE_VAR(uint16_t, kMaxFaceAgeToTurnTowards_ms, "Activity.Feeding", 2000);

static const char* kUniversalChooser = "universalChooser";
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kFeedingActivityAffectedArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       true},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          true},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           false},
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
  {ReactionTrigger::RobotFalling,                 true},
  {ReactionTrigger::RobotPickedUp,                true},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             true},
  {ReactionTrigger::RobotOnBack,                  true},
  {ReactionTrigger::RobotOnFace,                  true},
  {ReactionTrigger::RobotOnSide,                  true},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      true},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kSevereFeedingDisables),
              "Reaction triggers duplicate or non-sequential");
  
  
const char* kSevereFeedingDisableLock = "feeding_severe_disables";
const char* kFindFaceDASKey = "find_face";
const char* kWaitShakeDASKey = "wait_shake";
const char* kShakeFullDASKey = "shake_full";
const char* kFindCubeDASKey =  "find_cube";
const char* kEatingDASKey =  "eating";
  
} // end namespace


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFeeding::ActivityFeeding(Robot& robot, const Json::Value& config)
: IActivity(robot, config)
, _activityStage(FeedingActivityStage::SearchForFace)
, _lastStageChangeTime_s(0)
, _timeFaceSearchShouldEnd_s(FLT_MAX)
, _eatingComplete(false)
, _severeBehaviorLocksSet(false)
, _currIdle(AnimationTrigger::Count)
, _hasSetIdle(false)
, _universalResponseChooser(nullptr)
{
  ////////
  /// Grab behaviors
  ////////
  
  const auto& BM = robot.GetBehaviorManager();

  BM.FindBehaviorByIDAndDowncast(BehaviorID::FeedingSearchForCube,
                                 BehaviorClass::FeedingSearchForCube,
                                 _searchForCubeBehavior);

  BM.FindBehaviorByIDAndDowncast(BehaviorID::FeedingEat,
                                 BehaviorClass::FeedingEat,
                                 _eatFoodBehavior);
  
  // Get the searching for face behavior
  _searchingForFaceBehavior = BM.FindBehaviorByID(BehaviorID::FindFaces_socialize);
  DEV_ASSERT(_searchingForFaceBehavior != nullptr &&
             _searchingForFaceBehavior->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FindFaces.IncorrectBehaviorReceivedFromFactory");
  
  _searchingForFaceBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingFindFacesSevere);
  DEV_ASSERT(_searchingForFaceBehavior_Severe != nullptr &&
             _searchingForFaceBehavior_Severe->GetClass() == BehaviorClass::FindFaces,
             "ActivityFeeding.FindFaces_Severe.IncorrectBehaviorReceivedFromFactory");
  
  _turnToFaceBehavior = BM.FindBehaviorByID(BehaviorID::FeedingPlayRequestAtFace);
  DEV_ASSERT(_turnToFaceBehavior != nullptr,
             "ActivityFeeding.TurnToFaceBehavior.IncorrectBehaviorReceievedFromFactory");

  _turnToFaceBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingPlayRequestAtFace_Severe);
  DEV_ASSERT(_turnToFaceBehavior != nullptr,
             "ActivityFeeding.TurnToFace_SevereBehavior.IncorrectBehaviorReceivedFromFactory");

  _waitBehavior = BM.FindBehaviorByID(BehaviorID::Wait);
  DEV_ASSERT(_waitBehavior != nullptr, "ActivityFeeding.WaitBehavior.NotFound");
  
  _reactCubeShakeBehavior = BM.FindBehaviorByID(BehaviorID::FeedingReactCubeShake);
  DEV_ASSERT(_reactCubeShakeBehavior != nullptr, "ActivityFeeding.FeedingReactCubeShakeBehavior.NotFound");

  _reactCubeShakeBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactCubeShake_Severe);
  DEV_ASSERT(_reactCubeShakeBehavior_Severe != nullptr, "ActivityFeeding.FeedingReactCubeShake_SevereBehavior.NotFound");
  
  _reactFullCubeBehavior = BM.FindBehaviorByID(BehaviorID::FeedingReactFullCube);
  DEV_ASSERT(_reactFullCubeBehavior != nullptr, "ActivityFeeding.FeedingReactFullCubeBehavior.NotFound");

  _reactFullCubeBehavior_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactFullCube_Severe);
  DEV_ASSERT(_reactFullCubeBehavior_Severe != nullptr, "ActivityFeeding.FeedingReactFullCube_SevereBehavior.NotFound");
  
  _reactSeeCharged = BM.FindBehaviorByID(BehaviorID::FeedingReactSeeCharged);
  DEV_ASSERT(_reactSeeCharged != nullptr, "ActivityFeeding.FeedingReactSeeChargedBehavior.NotFound");

  _reactSeeCharged_Severe = BM.FindBehaviorByID(BehaviorID::FeedingReactSeeCharged_Severe);
  DEV_ASSERT(_reactSeeCharged_Severe != nullptr, "ActivityFeeding.FeedingReactSeeCharged_SevereBehavior.NotFound");
  
  ////////
  /// Setup UniversalChooser
  ////////
  {
    const Json::Value& universalChooserJSON = config[kUniversalChooser];
    _universalResponseChooser = BSRunnableChooserFactory::CreateBSRunnableChooser(
                                    robot,
                                    universalChooserJSON);
    DEV_ASSERT(_universalResponseChooser != nullptr,
               "ActivityFeeding.UniversalChooserNotSpecified");
  }
  
  ////////
  /// Setup Lights
  ////////
  
  // Make the activity a listener for the eating behavior so that it's notified
  // when the eating process starts
  _eatFoodBehavior->AddListener(static_cast<IFeedingListener*>(this));
  
  // Setup choose behavior map
  _stageToBehaviorMap = {
    {FeedingActivityStage::SearchForFace,                 _searchingForFaceBehavior},
    {FeedingActivityStage::SearchForFace_Severe,          _searchingForFaceBehavior_Severe},
    {FeedingActivityStage::TurningToFace,                 _turnToFaceBehavior},
    {FeedingActivityStage::TurningToFace_Severe,          _turnToFaceBehavior_Severe},
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
  
  // TODO:(bn) maybe this should be a function with a switch statement instead?
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
  if( ! ANKI_VERIFY(_currIdle == AnimationTrigger::Count,
                    "ActivityFeeding.OnSelectedInternal.IdleAlreadySet", "") ) {
    _currIdle = AnimationTrigger::Count;
    _hasSetIdle = false;
  }
  
  _eatingComplete = false;
  SmartDisableReactionsWithLock(robot, GetIDStr(), kFeedingActivityAffectedArray);
  
  const NeedId currentSevereExpression = robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
  if(currentSevereExpression == NeedId::Energy){
    SetupSevereAnims(robot);
  }else{
    ClearSevereAnims(robot);
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
    filter.SetAllowedFamilies({{ObjectFamily::LightCube, ObjectFamily::Block}});
    robot.GetBlockWorld().FindConnectedActiveMatchingObjects(filter, connectedObjs);
    
    for(auto& entry: connectedObjs){
      _cubeControllerMap.emplace(std::make_pair(
                            entry->GetID(),
                            std::make_unique<FeedingCubeController>(robot, entry->GetID())));
    }
  }
  
  // Setup messages to listen for. Only listen while we are selected, these handles will be cleared on
  // Deselect
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
  _DASFeedingSessionsPerConnectedSession++;
  Anki::Util::sEvent("meta.feeding_times_started",
                     {},
                     std::to_string(_DASFeedingSessionsPerConnectedSession).c_str());

  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const float needLevel = currNeedState.GetNeedLevel(NeedId::Energy);
  const NeedBracketId needBracket = currNeedState.GetNeedBracket(NeedId::Energy);
  Anki::Util::sEvent("meta.feeding_energy_at_start",
                     {{DDATA, NeedBracketIdToString(needBracket)}},
                     std::to_string(needLevel).c_str());

  // Start with searching for the face. If a face is already available, we'll immediately transition to
  // turning in the TransitionToBestActivityStage call below
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  if(isNeedSevere){
    SET_STAGE(robot, SearchForFace_Severe);
  }else{
    SET_STAGE(robot, SearchForFace);
  }

  // The activity is only allowed to search for a certain maximum amount of time
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeFaceSearchShouldEnd_s = currentTime_s + kTimeSearchForFace;

  // update the stage time to now so that we don't report a transition
  _lastStageChangeTime_s = currentTime_s;
  
  // Select the best activity stage, which may overwrite the search stage
  TransitionToBestActivityStage(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::OnDeselectedInternal(Robot& robot)
{
  if(_severeBehaviorLocksSet){
    ClearSevereAnims(robot);
  }

  if( _hasSetIdle ) {
    robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
    _currIdle = AnimationTrigger::Count;
    _hasSetIdle = false;
  }
  
  for(auto& entry: _cubeControllerMap){
    using CS = FeedingCubeController::ControllerState;
    entry.second->SetControllerState(robot, CS::Deactivated);
  }
  _cubeControllerMap.clear();
  _eventHandlers.clear();
  
  // Clear the public state broadcaster
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(BehaviorStageTag::Count, 0);
  
  // DAS Events
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const float needLevel = currNeedState.GetNeedLevel(NeedId::Energy);
  const NeedBracketId needBracket = currNeedState.GetNeedBracket(NeedId::Energy);
  Anki::Util::sEvent("meta.feeding_energy_at_end",
                     {{DDATA, NeedBracketIdToString(needBracket)}},
                     std::to_string(needLevel).c_str());
  
  Anki::Util::sEvent("meta.feeding_cubes_per_feeding",
                     {},
                     std::to_string(_DASCubesPerFeeding).c_str());
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityFeeding::GetDesiredActiveBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr bestBehavior;

  // First check for universal responses - eg drive off charger
  if(_universalResponseChooser){
    bestBehavior = _universalResponseChooser->GetDesiredActiveBehavior(robot, currentRunningBehavior);
  }
  
  if(bestBehavior != nullptr){
    return bestBehavior;
  }

  // consult the stage -> behavior map to get the best behavior
  bestBehavior = GetBestBehaviorFromMap();
  
  // Since transitioning to new stages generally occurs in the Update function
  // check whether the desired behavior has started running - when it's run
  // at least one tick and stops running re-evaluate the best stage to be at
  if(HasSingleBehaviorStageStarted(bestBehavior) &&
     !bestBehavior->IsRunning()){
    TransitionToBestActivityStage(robot);
    bestBehavior = GetBestBehaviorFromMap();   
  }
  
  return bestBehavior;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityFeeding::Update(Robot& robot)
{
  // Maintain appropriate music state and disables
  const NeedId currentSevereExpression = robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression();
  if(!_severeBehaviorLocksSet &&
     (currentSevereExpression == NeedId::Energy)){
    SetupSevereAnims(robot);
  }else if(_severeBehaviorLocksSet &&
           (currentSevereExpression != NeedId::Energy)){
    ClearSevereAnims(robot);
  }
  
  // Update cubes
  for(auto& entry: _cubeControllerMap){
    entry.second->Update(robot);
  }

  SendCubeDasEventsIfNeeded();

  // logic to check if there's a cube we should eat. If so, it'll update the stage automatically
  UpdateCubeToEat(robot);

  // process transitions and checks for the current stage
  UpdateCurrentStage(robot);
  
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateCurrentStage(Robot& robot)
{
  // Determine whether any cubes are charged or charging
  bool anyCubesToSearchFor = false;
  bool anyCubesCharged = false;
  bool anyCubesCharging = false;
  for(const auto& entry: _cubeControllerMap){
    const bool alreadySearchedForCube = (_cubesSearchCouldntFind.find(entry.first) != _cubesSearchCouldntFind.end());
    anyCubesToSearchFor = anyCubesToSearchFor || (entry.second->IsCubeCharged() && !alreadySearchedForCube);
    anyCubesCharged = anyCubesCharged || entry.second->IsCubeCharged();
    anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
  }

  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  switch(_activityStage) {

    case FeedingActivityStage::SearchForFace: 
    case FeedingActivityStage::SearchForFace_Severe: {
      if( HasFaceToTurnTo(robot) ) {
        if(isNeedSevere){
          SET_STAGE(robot, TurningToFace_Severe);
        }
        else {
          SET_STAGE(robot, TurningToFace);
        }
      }
      else if( currentTime_s >= _timeFaceSearchShouldEnd_s ) {
        // if we've been searching too long, give up and go to the next stage
        TransitionToBestActivityStage(robot);
      }else if(anyCubesCharging){
        // Allow cube shaking to interrupt searching for face
        if(isNeedSevere){
          SET_STAGE(robot, ReactingToShake_Severe);
        }else{
          SET_STAGE(robot, ReactingToShake);
        }
      }
      break;
    }
      
    case FeedingActivityStage::TurningToFace: 
    case FeedingActivityStage::TurningToFace_Severe: 
    case FeedingActivityStage::WaitingForShake:
      // if a shake (aka charge) started, transition here
      if(anyCubesCharging){
        if(isNeedSevere){
          SET_STAGE(robot, ReactingToShake_Severe);
        }else{
          SET_STAGE(robot, ReactingToShake);
        }
      }

      break;
        
    case FeedingActivityStage::WaitingForFullyCharged:
      if(anyCubesToSearchFor){
        if(isNeedSevere){
          SET_STAGE(robot, ReactingToFullyCharged_Severe);
        }else{
          SET_STAGE(robot, ReactingToFullyCharged);
        }
      }else if(!anyCubesCharging){
        SET_STAGE(robot, WaitingForShake);
      }
      // else keep waiting for the charge to be full
      break;
        
    case FeedingActivityStage::SearchingForCube:
      // if charged cube disconnected, it won't be charged anymore, so transition out
      if(!anyCubesCharged){
        TransitionToBestActivityStage(robot);
      }
      else if( HasSingleBehaviorStageStarted(_searchForCubeBehavior) &&
               !_searchForCubeBehavior->IsRunning()) {

        PRINT_CH_INFO("Feeding",
                      "FeedingActivity.Update.SearchingForCube.SearchFailed",
                      "Search behavior stopped without finding cube, so clearing cubeIDToSearchFor %d",
                      _cubeIDToSearchFor.GetValue());          
          
        // if the search behavior has stopped, mark as cube search failed and transition out
        _cubesSearchCouldntFind.insert(_cubeIDToSearchFor);
        _cubeIDToSearchFor.UnSet();
        TransitionToBestActivityStage(robot);
      }
      break;

    case FeedingActivityStage::EatFood:
      if(_cubeIDToEat.IsSet()) {
        
        if( HasSingleBehaviorStageStarted(_eatFoodBehavior) &&
            !_eatFoodBehavior->IsRunning() ){
          // we ate the target cube, so remove it from the ones we couldn't find, so that we will be allowed
          // to do a full search for it again in the future if it gets shaken again
          _cubesSearchCouldntFind.erase(_cubeIDToEat);
          _cubeIDToEat.UnSet();

          TransitionToBestActivityStage(robot);
        }
      }
      else {
        // we lost the cube for some reason, so just go to whatever stage seems best
        TransitionToBestActivityStage(robot);
      }

    case FeedingActivityStage::ReactingToShake: 
    case FeedingActivityStage::ReactingToShake_Severe:
    case FeedingActivityStage::ReactingToFullyCharged: 
    case FeedingActivityStage::ReactingToFullyCharged_Severe:
    case FeedingActivityStage::ReactingToSeeCharged:
    case FeedingActivityStage::ReactingToSeeCharged_Severe:
      // nothing to do for reacting states, just wait for behavior to finish which will automatically call
      // TransitionToBestActivityStage in GetDesiredActiveBehavior()
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::UpdateCubeToEat(Robot& robot)
{
  if( _activityStage == FeedingActivityStage::EatFood ||
      _activityStage == FeedingActivityStage::ReactingToSeeCharged ||
      _activityStage == FeedingActivityStage::ReactingToSeeCharged_Severe ||
      _eatFoodBehavior->IsRunning() ) {
    // nothing to do, we're already eating or about to eat
    return;
  }

  if( _cubeIDToEat.IsSet() ) {
    // if the current target cube is invalid, clear it

    // cube must be located
    if( robot.GetBlockWorld().GetLocatedObjectByID(_cubeIDToEat) == nullptr ) {
      PRINT_CH_INFO("Feeding", "ActivityFeeding.UpdateCubeToEat.LostCube",
                    "Lost track of cube %d (it's no longer located)",
                    _cubeIDToEat.GetValue());
      _cubeIDToEat.UnSet();
    }

    // cube must be charged
    const auto it = _cubeControllerMap.find(_cubeIDToEat);
    if( it == _cubeControllerMap.end() ||
        ! it->second->IsCubeCharged() ) {
      PRINT_CH_INFO("Feeding", "AcitivityFeeding.UpdateCubeToEat.CubeNotCharged",
                    "Cube %d is no longer charged, not a valid cube to eat anymore",
                    _cubeIDToEat.GetValue());
      _cubeIDToEat.UnSet();
    }
  }

  // now go through each cube and see if it's valid for eating
  for(const auto& entry: _cubeControllerMap){
    if( entry.second->IsCubeCharged() ) {
      _eatFoodBehavior->SetTargetObject(entry.first);
      if( _eatFoodBehavior->IsRunnable(robot)) {

        // just eat the first cube we can. It might be better to eat the closest cube, or the most recently
        // seen cube, but any cube will do        
        _cubeIDToEat = entry.first;
          
        PRINT_CH_INFO("Feeding",
                      "ActivityFeeding.UpdateCubeToEat.FoundFube",
                      "Cozmo can eat cube %d",
                      _cubeIDToEat.GetValue());

        // set the stage now so we go right to eating
          
        NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
        const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
        if( isNeedSevere )  {
          SET_STAGE(robot, ReactingToSeeCharged_Severe);
        }
        else {
          SET_STAGE(robot, ReactingToSeeCharged);
        }
        
        return;
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::TransitionToBestActivityStage(Robot& robot)
{
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // Transition into turn to face if one is available and we aren't yet past that stage
  if( _activityStage < FeedingActivityStage::TurningToFace &&
      currentTime_s < _timeFaceSearchShouldEnd_s ) {

    if( HasFaceToTurnTo(robot) ) {
      NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
      const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);

      if(isNeedSevere){
        SET_STAGE(robot, TurningToFace_Severe);
      }
      else {
        SET_STAGE(robot, TurningToFace);
      }
    }
    // else, keep searching (until update decides it's searched long enough

    return;
  }

  // if we have a located cube to eat, go to eating
  if( _cubeIDToEat.IsSet() ) {
    if( _eatFoodBehavior->IsRunning() ) {
      // behavior already running, keep it going
      SET_STAGE(robot, EatFood);
      return;
    }
    else {
      _eatFoodBehavior->SetTargetObject(_cubeIDToEat);
      if( _eatFoodBehavior->IsRunnable(robot) ) {
        SET_STAGE(robot, EatFood);
        return;
      }
      else {
        PRINT_NAMED_WARNING("ActivityFeeding.TransitionToBestActivityStage.HaveTargetButCantEat",
                            "We have a target cube id %d, but the eating behavior isn't runnable",
                            _cubeIDToEat.GetValue());
        // clear cube to eat, in case it's no longer valid (it'll get reset automatically if it is)
        _cubeIDToEat.UnSet();
      }
    }
  }  
  
  // if we have a cube to search for, make sure it's still valid
  if( _cubeIDToSearchFor.IsSet() ) {
    // cube must be charged
    const auto it = _cubeControllerMap.find(_cubeIDToSearchFor);
    if( it == _cubeControllerMap.end() ||
        ! it->second->IsCubeCharged() ) {
      PRINT_CH_INFO("Feeding", "AcitivityFeeding.TransitionToBestActivityStage.ClearSearchingForId",
                    "Cube %d is no longer charged, not a valid cube to search for anymore",
                    _cubeIDToSearchFor.GetValue());
      _cubeIDToSearchFor.UnSet();
    }
  }

  // Jump to appropriate waiting/animation stage based on cube charge state
  // check for cubes that are fully charged or partially charged
  bool anyCubesCharging = false;
  for(const auto& entry: _cubeControllerMap){
    const bool alreadySearchedForCube =
           (_cubesSearchCouldntFind.find(entry.first) != _cubesSearchCouldntFind.end());
    if(entry.second->IsCubeCharged() && !alreadySearchedForCube){
      _cubeIDToSearchFor = entry.first;
    }
    anyCubesCharging = anyCubesCharging || entry.second->IsCubeCharging();
  }
  
  if(_cubeIDToSearchFor.IsSet()){

    PRINT_CH_INFO("Feeding",
                  "FeedingActivity.TransitionToBestActivityStage.SetcubeIDToSearchFor",
                  "Robot will search for charged cube %d",
                  _cubeIDToSearchFor.GetValue());
    
    SET_STAGE(robot, SearchingForCube);
  }else if(anyCubesCharging){
    SET_STAGE(robot, WaitingForFullyCharged);
  }else{
    SET_STAGE(robot, WaitingForShake);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr ActivityFeeding::GetBestBehaviorFromMap() const
{
  const auto& iter = _stageToBehaviorMap.find(_activityStage);
  if(iter != _stageToBehaviorMap.end()){
    return iter->second;
  }
  else {
    PRINT_NAMED_WARNING("ActivityFeeding.GetBestBehaviorFromMap.NoMappedBehavior",
                        "No behavior in the map for stage #%d",
                        Util::EnumToUnderlying(_activityStage));
    return nullptr;
  }
}

// Implementation of IFeedingListener
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::StartedEating(Robot& robot, const float duration_s)
{
  using CS = FeedingCubeController::ControllerState;
  _cubeControllerMap[_cubeIDToEat]->SetControllerState(robot, CS::DrainCube, duration_s);
  _DASCubesPerFeeding++;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::EatingComplete(Robot& robot)
{
  using CS = FeedingCubeController::ControllerState;
  const auto it = _cubeControllerMap.find(_cubeIDToEat);
  if( it != _cubeControllerMap.end()){
    it->second->SetControllerState(robot, CS::Deactivated);
    it->second->SetControllerState(robot, CS::Activated);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::EatingInterrupted(Robot& robot)
{
  using CS = FeedingCubeController::ControllerState;
  const auto it = _cubeControllerMap.find(_cubeIDToEat);
  if( it != _cubeControllerMap.end()){
    it->second->SetControllerState(robot, CS::Deactivated);
    it->second->SetControllerState(robot, CS::Activated);
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
       (_activityStage >= FeedingActivityStage::WaitingForShake)){
      mapObjIter = emplaceRes.first;
      mapObjIter->second->SetControllerState(robot, CS::Activated);
    }
  }else{
    if(mapObjIter != _cubeControllerMap.end()){
      mapObjIter->second->SetControllerState(robot,CS::Deactivated);
      _cubeControllerMap.erase(objID);
    }
    
    if(_cubeIDToEat.GetValue() == connectionState.objectID){
      PRINT_CH_INFO("Feeding",
                    "FeedingActivity.HandleObjectConnectionStateChange.LostConnectionToInteractID",
                    "object %d lost connection, clearing cubeIDToEat",
                    _cubeIDToEat.GetValue());
      
      _cubeIDToEat.UnSet();
    }
  }
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::SetActivityStage(Robot& robot,
                                       FeedingActivityStage newStage,
                                       const std::string& newStageName)
{

  // warn if we transition to the same stage, except in the case of SearchForFace since it's the default
  // initial value
  if( _activityStage == newStage && _activityStage != FeedingActivityStage::SearchForFace) {
    PRINT_NAMED_WARNING("ActivityFeeding.SelfTransition",
                        "Updating stage from #%d to itself!",
                        Util::EnumToUnderlying(_activityStage));
    return;
  }
                        
  
  PRINT_CH_INFO("Feeding",
                "ActivityFeeding.SetActivityStage",
                "Entering feeding stage %s",
                newStageName.c_str());

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // figure out if our old stage was one we want to send a das event about
  const char* previousStageName = nullptr;

  switch( _activityStage ) {
    case FeedingActivityStage::WaitingForShake:
      if( newStage == FeedingActivityStage::ReactingToShake ||
          newStage == FeedingActivityStage::ReactingToShake_Severe ) {
        previousStageName = kWaitShakeDASKey;
      }
      break;

    case FeedingActivityStage::WaitingForFullyCharged:
      if( newStage == FeedingActivityStage::ReactingToFullyCharged ||
          newStage == FeedingActivityStage::ReactingToFullyCharged_Severe ) {
        previousStageName = kShakeFullDASKey;
      }
      break;

    case FeedingActivityStage::SearchingForCube:
      if( newStage == FeedingActivityStage::ReactingToSeeCharged ||
          newStage == FeedingActivityStage::ReactingToSeeCharged_Severe) {
        previousStageName = kFindCubeDASKey;
      }
      break;

    case FeedingActivityStage::EatFood:
      previousStageName = kEatingDASKey;
      break;

    case FeedingActivityStage::SearchForFace: 
    case FeedingActivityStage::SearchForFace_Severe:
      if( newStage != FeedingActivityStage::SearchForFace &&
          newStage != FeedingActivityStage::SearchForFace_Severe) {
        previousStageName = kFindFaceDASKey;
      }
      break;
      
    case FeedingActivityStage::TurningToFace:
    case FeedingActivityStage::TurningToFace_Severe:
    case FeedingActivityStage::ReactingToShake: 
    case FeedingActivityStage::ReactingToShake_Severe: 
    case FeedingActivityStage::ReactingToFullyCharged: 
    case FeedingActivityStage::ReactingToFullyCharged_Severe: 
    case FeedingActivityStage::ReactingToSeeCharged: 
    case FeedingActivityStage::ReactingToSeeCharged_Severe:
      // no DAS events for these states, since they are just simple actions or animations
      break;

  }

  if(previousStageName != nullptr){
    const float timeInStage = currentTime_s - _lastStageChangeTime_s;

    Anki::Util::sEvent("meta.feeding_stage_time",
                       {{DDATA, previousStageName}},
                       std::to_string(timeInStage).c_str());
  }

  _activityStage = newStage;
  _lastStageChangeTime_s = currentTime_s;

  SetIdleForCurrentStage(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::SetIdleForCurrentStage(Robot& robot)
{
  // Set the appropriate idle
  const auto& severeNeedsComponent = robot.GetAIComponent().GetSevereNeedsComponent();
  const bool isNeedSevere = (NeedId::Energy == severeNeedsComponent.GetSevereNeedExpression());

  AnimationTrigger desiredIdle = AnimationTrigger::Count;

  switch(_activityStage) {
    
    case FeedingActivityStage::SearchForFace: 
      desiredIdle = AnimationTrigger::FeedingIdleSearchForFaces_Normal;
      break;
      
    case FeedingActivityStage::SearchForFace_Severe:
      desiredIdle = AnimationTrigger::FeedingIdleSearchForFaces_Severe;
      break;

    case FeedingActivityStage::WaitingForFullyCharged:
      // use special idles here which work during shaking (they look up)
      if(isNeedSevere){
        desiredIdle = AnimationTrigger::FeedingIdleWaitForFullCube_Severe;
      }
      else {
        desiredIdle = AnimationTrigger::FeedingIdleWaitForFullCube_Normal;
      }
      break;

    // FeedingIdleWaitForShake_Severe moves the head, so be careful to only use that one where head motion is
    // expected in the wait stage
    case FeedingActivityStage::WaitingForShake: 
      if(isNeedSevere){
        desiredIdle = AnimationTrigger::FeedingIdleWaitForShake_Severe;
      }
      else {
        desiredIdle = AnimationTrigger::FeedingIdleWaitForShake_Normal;
      }
      break;

    // For the rest of the states, use idles that don't move the head or lift
    case FeedingActivityStage::TurningToFace: 
    case FeedingActivityStage::TurningToFace_Severe: 
    case FeedingActivityStage::ReactingToShake: 
    case FeedingActivityStage::ReactingToShake_Severe: 
    case FeedingActivityStage::ReactingToFullyCharged: 
    case FeedingActivityStage::ReactingToFullyCharged_Severe: 
    case FeedingActivityStage::SearchingForCube: 
    case FeedingActivityStage::ReactingToSeeCharged: 
    case FeedingActivityStage::ReactingToSeeCharged_Severe: 
    case FeedingActivityStage::EatFood:
      if(isNeedSevere){
        desiredIdle = AnimationTrigger::FeedingIdleWaitForShakeNoHead_Severe;
      }
      else {
        desiredIdle = AnimationTrigger::Count;
      }
      break;
  }

  if( desiredIdle != _currIdle || !_hasSetIdle ) {
    if( _hasSetIdle ) {
      robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
    }

    PRINT_CH_INFO("Feeding",
                  "ActivityFeeding.SwitchIDle",
                  "Switching from feeding idle '%s' to '%s'",
                  AnimationTriggerToString(_currIdle),
                  AnimationTriggerToString(desiredIdle));
    
    robot.GetAnimationStreamer().PushIdleAnimation(desiredIdle, GetIDStr());
    _currIdle = desiredIdle;
    _hasSetIdle = true;
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
bool ActivityFeeding::HasFaceToTurnTo(Robot& robot) const
{
  const TimeStamp_t lastImgTimestamp = robot.GetLastImageTimeStamp();

  TimeStamp_t maxFaceAge = 0;
  if( lastImgTimestamp > kMaxFaceAgeToTurnTowards_ms ) {
    maxFaceAge = lastImgTimestamp - kMaxFaceAgeToTurnTowards_ms;
  }
    
  auto facesObserved = robot.GetFaceWorld().GetFaceIDsObservedSince( maxFaceAge);
  const bool hasFace = ! facesObserved.empty();

  return hasFace;    
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::SetupSevereAnims(Robot& robot)
{
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(
       BehaviorStageTag::Feeding, static_cast<int>(FeedingStage::SevereEnergy));
  SmartDisableReactionsWithLock(robot, kSevereFeedingDisableLock, kSevereFeedingDisables);
  _severeBehaviorLocksSet = true;
  SetIdleForCurrentStage(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::ClearSevereAnims(Robot& robot)
{
  robot.GetPublicStateBroadcaster().UpdateBroadcastBehaviorStage(
       BehaviorStageTag::Feeding, static_cast<int>(FeedingStage::MildEnergy));
  SmartRemoveDisableReactionsLock(robot, kSevereFeedingDisableLock);
  _severeBehaviorLocksSet = false;
  SetIdleForCurrentStage(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFeeding::SendCubeDasEventsIfNeeded()
{
  int countCubesCharged = 0;
  for(const auto& entry: _cubeControllerMap){
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


} // namespace Cozmo
} // namespace Anki
