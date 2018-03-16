/**
 * File: behaviorManager.cpp
 *
 * Author: Kevin Yoon
 * Date:   2/27/2014
 * Overhaul: 2016-04-18 Brad Neuman
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2014-2016
 **/

#include "engine/behaviorSystem/behaviorManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/doATrickSelector.h"
#include "engine/aiComponent/requestGameComponent.h"
#include "engine/audio/behaviorAudioClient.h"
#include "engine/behaviorSystem/activities/activities/activityFactory.h"
#include "engine/behaviorSystem/activities/activities/iActivity.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/aiComponent/objectInteractionInfoCache.h"
#include "engine/behaviorSystem/activities/activities/activityFreeplay.h"
#include "engine/behaviorSystem/behaviors/freeplay/gameRequest/behaviorRequestGameSimple.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/bsRunnableChoosers/bsRunnableChooserFactory.h"
#include "engine/behaviorSystem/bsRunnableChoosers/iBSRunnableChooser.h"
#include "engine/behaviorSystem/behaviorContainer.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFactory.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/dockingComponent.h"
#include "engine/components/inventoryComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/messageHelpers.h"
#include "engine/moodSystem/moodDebug.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/viz/vizManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/behaviorSystem/reactionTriggers.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


#define DEBUG_BEHAVIOR_MGR 0

#if ANKI_DEV_CHEATS
  #define VIZ_BEHAVIOR_SELECTION  1
#else
  #define VIZ_BEHAVIOR_SELECTION  0
#endif // ANKI_DEV_CHEATS

#if VIZ_BEHAVIOR_SELECTION
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)  exp
#else
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)
#endif // VIZ_BEHAVIOR_SELECTION

namespace Anki {
namespace Cozmo {
namespace ReactionTriggerHelpers {
// This is declared in reactionTriggerHelpers.h
const ReactionTriggerHelpers::FullReactionArray &GetAffectAllArray() {

  static constexpr const ReactionTriggerHelpers::FullReactionArray kAffectAllArray = {
      {ReactionTrigger::CliffDetected,         true},
      {ReactionTrigger::CubeMoved,             true},
      {ReactionTrigger::FacePositionUpdated,   true},
      {ReactionTrigger::FistBump,              true},
      {ReactionTrigger::Frustration,           true},
      {ReactionTrigger::Hiccup,                true},
      {ReactionTrigger::MotorCalibration,      true},
      {ReactionTrigger::NoPreDockPoses,        true},
      {ReactionTrigger::ObjectPositionUpdated, true},
      {ReactionTrigger::PlacedOnCharger,       true},
      {ReactionTrigger::PetInitialDetection,   true},
      {ReactionTrigger::RobotFalling,          true},
      {ReactionTrigger::RobotPickedUp,         true},
      {ReactionTrigger::RobotPlacedOnSlope,    true},
      {ReactionTrigger::ReturnedToTreads,      true},
      {ReactionTrigger::RobotOnBack,           true},
      {ReactionTrigger::RobotOnFace,           true},
      {ReactionTrigger::RobotOnSide,           true},
      {ReactionTrigger::RobotShaken,           true},
      {ReactionTrigger::Sparked,               true},
      {ReactionTrigger::UnexpectedMovement,    true},
  };

  static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectAllArray),
                "Reaction triggers duplicate or non-sequential");

  return kAffectAllArray;
}

void EnsureFullReactionArrayConversionsValid(const FullReactionArray& reactions, const AllTriggersConsidered& triggersAffected)
{
  DEV_ASSERT(IsSequentialArray(reactions),
             "ReactionTriggerHelpers.ConvertReactionArray.InitialArrayNotSequential");
  FullReactionArray checkCommutativeArray = ALL_TRIGGERS_CONSIDERED_TO_FULL_ARRAY(triggersAffected);
  DEV_ASSERT(IsSequentialArray(checkCommutativeArray),
             "ReactionTriggerHelpers.ConvertReactionArray.NotCommutative");
  // Make sure that no values got swapped around in the conversion process

  for(int index = 0; index < Util::EnumToUnderlying(ReactionTrigger::Count); index++){
    auto& reactionEntry = reactions[index];
    auto& commutativeEntry = checkCommutativeArray[index];
    DEV_ASSERT(reactionEntry.EnumValue() == commutativeEntry.EnumValue(),
               "EnsureFullReactionArrayConversionsValid.ConversionEnumValueMismatch");
    DEV_ASSERT(reactionEntry.Value() == commutativeEntry.Value(),
               "EnsureFullReactionArrayConversionsValid.ConversionValueMismatch");
    switch(reactionEntry.EnumValue()){
      case ReactionTrigger::CliffDetected:
      {
        DEV_ASSERT((triggersAffected.cliffDetected == reactionEntry.Value()) &&
                   (triggersAffected.cliffDetected == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.CliffDetectedIssue");
        break;
      }
      case ReactionTrigger::CubeMoved:
      {
        DEV_ASSERT((triggersAffected.cubeMoved == reactionEntry.Value()) &&
                   (triggersAffected.cubeMoved == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.CubeMoved");
        break;
      }
      case ReactionTrigger::FacePositionUpdated:
      {
        DEV_ASSERT((triggersAffected.facePositionUpdated == reactionEntry.Value()) &&
                   (triggersAffected.facePositionUpdated == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.FacePositionUpdated");
        break;
      }
      case ReactionTrigger::FistBump:
      {
        DEV_ASSERT((triggersAffected.fistBump == reactionEntry.Value()) &&
                   (triggersAffected.fistBump == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.FistBump");
        break;
      }
      case ReactionTrigger::Frustration:
      {
        DEV_ASSERT((triggersAffected.frustration == reactionEntry.Value()) &&
                   (triggersAffected.frustration == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.Frustration");
        break;
      }
      case ReactionTrigger::Hiccup:
      {
        DEV_ASSERT((triggersAffected.hiccup == reactionEntry.Value()) &&
                   (triggersAffected.hiccup == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.Hiccup");
        break;
      }
      case ReactionTrigger::MotorCalibration:
      {
        DEV_ASSERT((triggersAffected.motorCalibration == reactionEntry.Value()) &&
                   (triggersAffected.motorCalibration == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.MotorCalibration");
        break;
      }
      case ReactionTrigger::NoPreDockPoses:
      {
        DEV_ASSERT((triggersAffected.noPreDockPoses == reactionEntry.Value()) &&
                   (triggersAffected.noPreDockPoses == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.NoPreDockPoses");
        break;
      }
      case ReactionTrigger::ObjectPositionUpdated:
      {
        DEV_ASSERT((triggersAffected.objectPositionUpdated == reactionEntry.Value()) &&
                   (triggersAffected.objectPositionUpdated == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.ObjectPositionUpdated");
        break;
      }
      case ReactionTrigger::PlacedOnCharger:
      {
        DEV_ASSERT((triggersAffected.placedOnCharger == reactionEntry.Value()) &&
                   (triggersAffected.placedOnCharger == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.PlacedOnCharger");
        break;
      }
      case ReactionTrigger::PetInitialDetection:
      {
        DEV_ASSERT((triggersAffected.petInitialDetection == reactionEntry.Value()) &&
                   (triggersAffected.petInitialDetection == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.PetInitialDetection");
        break;
      }
      case ReactionTrigger::RobotFalling:
      {
        DEV_ASSERT((triggersAffected.robotFalling == reactionEntry.Value()) &&
                   (triggersAffected.robotFalling == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotFalling");
        break;
      }
      case ReactionTrigger::RobotPickedUp:
      {
        DEV_ASSERT((triggersAffected.robotPickedUp == reactionEntry.Value()) &&
                   (triggersAffected.robotPickedUp == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotPickedUp");
        break;
      }
      case ReactionTrigger::RobotPlacedOnSlope:
      {
        DEV_ASSERT((triggersAffected.robotPlacedOnSlope == reactionEntry.Value()) &&
                   (triggersAffected.robotPlacedOnSlope == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotPlacedOnSlope");
        break;
      }
      case ReactionTrigger::ReturnedToTreads:
      {
        DEV_ASSERT((triggersAffected.returnedToTreads == reactionEntry.Value()) &&
                   (triggersAffected.returnedToTreads == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.ReturnedToTreads");
        break;
      }
      case ReactionTrigger::RobotOnBack:
      {
        DEV_ASSERT((triggersAffected.robotOnBack == reactionEntry.Value()) &&
                   (triggersAffected.robotOnBack == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotOnBack");
        break;
      }
      case ReactionTrigger::RobotOnFace:
      {
        DEV_ASSERT((triggersAffected.robotOnFace == reactionEntry.Value()) &&
                   (triggersAffected.robotOnFace == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotOnFace");
        break;
      }
      case ReactionTrigger::RobotOnSide:
      {
        DEV_ASSERT((triggersAffected.robotOnSide == reactionEntry.Value()) &&
                   (triggersAffected.robotOnSide == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotOnSide");
        break;
      }
      case ReactionTrigger::RobotShaken:
      {
        DEV_ASSERT((triggersAffected.robotShaken == reactionEntry.Value()) &&
                   (triggersAffected.robotShaken == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.RobotShaken");
        break;
      }
      case ReactionTrigger::Sparked:
      {
        DEV_ASSERT((triggersAffected.sparked == reactionEntry.Value()) &&
                   (triggersAffected.sparked == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.Sparked");
        break;
      }
      case ReactionTrigger::UnexpectedMovement:
      {
        DEV_ASSERT((triggersAffected.unexpectedMovement == reactionEntry.Value()) &&
                   (triggersAffected.unexpectedMovement == commutativeEntry.Value()),
                   "EnsureFullReactionArrayConversionsValid.UnexpectedMovement");
        break;
      }
      case ReactionTrigger::Count:
      case ReactionTrigger::NoneTrigger:
      {
        DEV_ASSERT(false, "EnsureFullReactionArrayConversionsValid.InvalidTrigger");
        break;
      }
    }

  }
}

AllTriggersConsidered ConvertReactionArrayToAllTriggersConsidered(const FullReactionArray& reactions)
{
  AllTriggersConsidered affected = AllTriggersConsidered(
       reactions[Util::EnumToUnderlying(ReactionTrigger::CliffDetected)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::CubeMoved)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::FacePositionUpdated)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::FistBump)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::Frustration)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::Hiccup)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::MotorCalibration)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::NoPreDockPoses)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::ObjectPositionUpdated)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::PlacedOnCharger)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::PetInitialDetection)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotFalling)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotPickedUp)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotPlacedOnSlope)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::ReturnedToTreads)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotOnBack)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotOnFace)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotOnSide)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::RobotShaken)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::Sparked)].Value(),
       reactions[Util::EnumToUnderlying(ReactionTrigger::UnexpectedMovement)].Value());
  if(ANKI_DEV_CHEATS){
    EnsureFullReactionArrayConversionsValid(reactions, affected);
  }

  return affected;
}

const AllTriggersConsidered& GetAffectAllReactions() {
  static const AllTriggersConsidered kAffectAllReactions = ConvertReactionArrayToAllTriggersConsidered(GetAffectAllArray());
  return kAffectAllReactions;
}


} // namespace ReactionTriggerHelpers
    
namespace{
// For creating activities and accessing them
static const char* kHighLevelActivityTypeConfigKey    = "highLevelID";
  
// reaction trigger behavior map
static const char* kReactionTriggerBehaviorMapKey = "reactionTriggerBehaviorMap";
static const char* kReactionTriggerKey            = "reactionTrigger";

// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
} while(0) \

constexpr ReactionTriggerHelpers::FullReactionArray kAffectTriggersUIRequestGame = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          true},
  {ReactionTrigger::FistBump,                     true},
  {ReactionTrigger::Frustration,                  true},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               true},
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
  {ReactionTrigger::UnexpectedMovement,           true},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kAffectTriggersUIRequestGame),
              "Reaction triggers duplicate or non-sequential");

const char* kDisableReactionsUIRequestGameLock = "bm_ui_request_game_lock";
  
} // namespace

/////////
// Running/Resume implementation
/////////
  
// struct which defines information about the currently running behavior
struct BehaviorRunningAndResumeInfo{
  void SetCurrentBehavior(IBehaviorPtr newScoredBehavior){_currentBehavior = newScoredBehavior;}
  // return the active behavior based on the category set in the struct
  IBehaviorPtr GetCurrentBehavior() const{return _currentBehavior;}
  
  void SetBehaviorToResume(IBehaviorPtr resumeBehavior){
    _behaviorToResume = resumeBehavior;
  }
  IBehaviorPtr GetBehaviorToResume() const{ return _behaviorToResume;}
  
  void SetCurrentReactionType(ReactionTrigger trigger){
    DEV_ASSERT(trigger != ReactionTrigger::Count, "Invalid ReactionTrigger state");
    _currentReactionType = trigger;
  }
  
  ReactionTrigger GetCurrentReactionTrigger() const{ return _currentReactionType;}
  
private:
  // only one behavior should be active at a time
  // either a scored behavior or a reactionary behavior
  IBehaviorPtr _currentBehavior;
  // the scored behavior to resume once a reactionary behavior ends
  IBehaviorPtr _behaviorToResume;
  ReactionTrigger _currentReactionType = ReactionTrigger::NoneTrigger;
};
  
  
/////////
// TriggerBehaviorInfo implementation
/////////
  
struct TriggerBehaviorInfo{
public:
  using StrategyBehaviorMap = std::pair<std::unique_ptr<IReactionTriggerStrategy>, IBehaviorPtr>;

  bool  IsReactionEnabled() const { return _disableIDs.empty();}
  
  bool AddStrategyMapping(IReactionTriggerStrategy*& strategy, IBehaviorPtr behavior);
  const std::vector<StrategyBehaviorMap>& GetStrategyMap() const { return _strategyBehaviorMap;}
  
  // For enabling/disabling the strategy
  bool  AddDisableLockToTrigger(const std::string& disableID, ReactionTrigger debugTrigger);
  bool  IsTriggerLockedByID(const std::string& disableID) const;
  bool  RemoveDisableLockFromTrigger(const std::string& disableID, ReactionTrigger debugTrigger);
  
  
private:
  std::vector<StrategyBehaviorMap>          _strategyBehaviorMap;
  std::set<std::string>                     _disableIDs;
};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TriggerBehaviorInfo::AddStrategyMapping(IReactionTriggerStrategy*& strategy, IBehaviorPtr behavior)
{
  DEV_ASSERT_MSG(behavior != nullptr, "TriggerBehaviorInfo.BehaviorNullptr",
                 "Nullptr passed in to triggerBehavior info for behavior");
  DEV_ASSERT_MSG(strategy != nullptr, "TriggerBehaviorInfo.StrategyNullptr",
                 "Nullptr passed in to triggerBehavior info for strategy");
  
  if(behavior != nullptr && strategy != nullptr){
    strategy->BehaviorThatStrategyWillTrigger(behavior);
    auto newEntry = std::make_pair(std::unique_ptr<IReactionTriggerStrategy>(strategy),
                                   behavior);
    _strategyBehaviorMap.emplace_back(std::move(newEntry));
    strategy = nullptr;
    return true;
  }
  
  Util::SafeDelete(strategy);
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TriggerBehaviorInfo::IsTriggerLockedByID(const std::string& disableID) const
{
  return _disableIDs.find(disableID) != _disableIDs.end();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TriggerBehaviorInfo::AddDisableLockToTrigger(const std::string& disableID,
                                                   ReactionTrigger debugTrigger)
{
  PRINT_CH_DEBUG("ReactionTriggers",
                 "BehaviorManager.TriggerBehaviorInfo.AddDisableLockToTrigger",
                 "%s: requesting trigger %s be disabled",
                 disableID.c_str(),
                 EnumToString(debugTrigger));
  
  auto disableIDIter =  _disableIDs.find(disableID);
  if(disableIDIter != _disableIDs.end()){
    DEV_ASSERT_MSG(false,
                   "TriggerBehaviorInfo.AddDisableLockToTrigger.DuplicatDisable",
                   "Attempted to disable reaction trigger %s with previously registered ID %s",
                   EnumToString(debugTrigger),
                   disableID.c_str());
    return false;
  }else{
    _disableIDs.insert(disableID);
  }
  
  return true;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TriggerBehaviorInfo::RemoveDisableLockFromTrigger(const std::string& disableID,
                                                        ReactionTrigger debugTrigger)
{
  PRINT_CH_DEBUG("ReactionTriggers",
                 "BehaviorManager.TriggerBehaviorInfo.RemoveDisableLockFromTrigger",
                 "%s: requesting trigger %s be re-enabled",
                 disableID.c_str(),
                 EnumToString(debugTrigger));
  
  int countRemoved = Util::numeric_cast<int>(_disableIDs.erase(disableID));
  if(countRemoved == 0){
    PRINT_NAMED_WARNING("TriggerBehaviorInfo.RemoveDisableLockFromTrigger.InvalidDisable",
                        "Attempted to re-enable reaction trigger %s with invalid ID %s",
                        EnumToString(debugTrigger),
                        disableID.c_str());
    return false;
  }
  
  
  return true;
}

/////////
// BehaviorManager implementation
/////////
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::BehaviorManager(Robot& robot)
: _robot(robot)
, _defaultHeadAngle(kIgnoreDefaultHeadAndLiftState)
, _defaultLiftHeight(kIgnoreDefaultHeadAndLiftState)
, _runningAndResumeInfo(new BehaviorRunningAndResumeInfo())
, _currentHighLevelActivity(HighLevelActivity::Count)
, _uiRequestGameBehavior(nullptr)
, _behaviorContainer(new BehaviorContainer(robot, robot.GetContext()->GetDataLoader()->GetBehaviorJsons()))
, _hasActionQueuedOrSDKReactEnabled(false)
, _lastChooserSwitchTime(-1.0f)
, _audioClient( new Audio::BehaviorAudioClient(robot) )
, _behaviorThatSetLights(BehaviorClass::Wait)
, _behaviorStateLightsPersistOnReaction(false)
{
  // Setup the UnlockID to game request behavior map for ui driven requests
  std::shared_ptr<BehaviorRequestGameSimple> VC_Keepaway;
  FindBehaviorByIDAndDowncast(BehaviorID::VC_RequestKeepAway, BehaviorClass::RequestGameSimple, VC_Keepaway);
  std::shared_ptr<BehaviorRequestGameSimple> VC_QT;
  FindBehaviorByIDAndDowncast(BehaviorID::VC_RequestSpeedTap, BehaviorClass::RequestGameSimple, VC_QT);
  std::shared_ptr<BehaviorRequestGameSimple> VC_MM;
  FindBehaviorByIDAndDowncast(BehaviorID::VC_RequestMemoryMatch, BehaviorClass::RequestGameSimple, VC_MM);

  _uiGameRequestMap.insert(std::make_pair(UnlockId::KeepawayGame, VC_Keepaway));
  _uiGameRequestMap.insert(std::make_pair(UnlockId::QuickTapGame, VC_QT));
  _uiGameRequestMap.insert(std::make_pair(UnlockId::MemoryMatchGame, VC_MM));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::~BehaviorManager()
{
  // the behaviors in_reactionTriggerMap are factory behaviors, so the factory will delete them
  // the reactionTriggers are unique ptrs, so they will be deleted by the clear
  _reactionTriggerMap.clear();
  
  
  // clear out high level choosers before destroying the factory
  for(auto& entry: _highLevelActivityMap){
    entry.second.reset();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::InitConfiguration(const Json::Value &activitiesConfig)
{
  BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
  
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(!_isInitialized, "BehaviorManager.InitConfiguration.AlreadyInitialized");
  
  // create activities
  if ( !activitiesConfig.isNull() )
  {
    const uint32_t numActivities = activitiesConfig.size();
    DEV_ASSERT(numActivities == static_cast<int>(HighLevelActivity::Count),
               "BehaviorManager.InitConfiguration.HighLevelActivityEnumerationMismatch");
    const Json::Value kNullValue;

    for (uint32_t i = 0; i < numActivities; ++i){
      const Json::Value& activityJson = activitiesConfig.get(i, kNullValue);
      
      std::string highLevelActivityStr;
      JsonTools::GetValueOptional(activityJson, kHighLevelActivityTypeConfigKey, highLevelActivityStr);
      HighLevelActivity highLevelID = HighLevelActivityFromString(highLevelActivityStr);
      
      ActivityType activityType = IActivity::ExtractActivityTypeFromConfig(activityJson);
      
      _highLevelActivityMap.insert(
        std::make_pair(highLevelID,
                       std::shared_ptr<IActivity>(
                              ActivityFactory::CreateActivity(_robot,
                                                              activityType,
                                                              activityJson))));
    }
    
    // start with selection that defaults to Wait
    SetCurrentActivity(HighLevelActivity::Selection, true);
  }
  else {
    // no activities config (e.g. unit test), so we aren't in freeplay
    _robot.GetAIComponent().GetFreeplayDataTracker().SetFreeplayPauseFlag(true, FreeplayPauseFlag::GameControl);
  }
  
  if (_robot.HasExternalInterface())
  {
    InitializeEventHandlers();
  }
  
  // Init behavior audio client after parsing config because it relies on parsed values
  _audioClient->Init();
  
  _isInitialized = true;
    
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::InitializeEventHandlers()
{
  IExternalInterface* externalInterface = _robot.GetExternalInterface();
  
  // Disable reaction triggers by locking the specified triggers with the given lockID
  _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                              ExternalInterface::MessageGameToEngineTag::DisableReactionsWithLock,
                              [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                              {
                                const auto& msg = event.GetData().Get_DisableReactionsWithLock();
                                DisableReactionsWithLock(
                                                         msg.lockID,
                                                         ALL_TRIGGERS_CONSIDERED_TO_FULL_ARRAY(msg.triggersAffected),
                                                         true);
                              }));
  
  // Remove a specified lockID from all reaction triggers
  _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                              ExternalInterface::MessageGameToEngineTag::RemoveDisableReactionsLock,
                              [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                              {
                                const auto& msg = event.GetData().Get_RemoveDisableReactionsLock();
                                RemoveDisableReactionsLock(msg.lockID);
                              }));

  // Listen for a lock to disable all reactions with - only used by SDK
  _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                              ExternalInterface::MessageGameToEngineTag::DisableAllReactionsWithLock,
                              [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                              {
                                const auto& msg = event.GetData().Get_DisableAllReactionsWithLock();
                                DisableReactionsWithLock(
                                                         msg.lockID,
                                                         ReactionTriggerHelpers::GetAffectAllArray(),
                                                         true);
                              }));
  
  _eventHandlers.push_back(externalInterface->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::ActivateHighLevelActivity,
                            [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                            {
                              const HighLevelActivity activityType =
                              event.GetData().Get_ActivateHighLevelActivity().activityType;
                              SetCurrentActivity(activityType);
                              if((activityType == HighLevelActivity::Freeplay) &&
                                 (_firstTimeFreeplayStarted < 0.0f)){
                                const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                                _firstTimeFreeplayStarted = currTime_s;
                              }
                              
                              // If we are leaving freeplay, ensure that sparks
                              // have been cleared out
                              if(activityType != HighLevelActivity::Freeplay){
                                _activeSpark = UnlockId::Count;
                                _lastRequestedSpark = UnlockId::Count;
                              }
                              
                            }));
  
  _eventHandlers.push_back(externalInterface->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::SetDefaultHeadAndLiftState,
                            [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                            {
                              bool enable = event.GetData().Get_SetDefaultHeadAndLiftState().enable;
                              float headAngle = event.GetData().Get_SetDefaultHeadAndLiftState().headAngle;
                              float liftHeight = event.GetData().Get_SetDefaultHeadAndLiftState().liftHeight;
                              SetDefaultHeadAndLiftState(enable, headAngle, liftHeight);
                            }));
      
  _eventHandlers.push_back(externalInterface->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::RequestReactionTriggerMap,
                            [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine> &event)
                            {
                              BroadcastReactionTriggerMap();
                            }));
  
  _eventHandlers.push_back(externalInterface->Subscribe(
                            ExternalInterface::MessageGameToEngineTag::BehaviorManagerMessage,
                            [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine> &event)
                            {
                              HandleMessage(event.GetData().Get_BehaviorManagerMessage().BehaviorManagerMessageUnion);
                            }));
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::InitReactionTriggerMap(const Json::Value& config)
{
  // Add strategys from the config
  if ( !config.isNull() )
  {
    const Json::Value& reactionTriggerArray = config[kReactionTriggerBehaviorMapKey];
    for(const auto& triggerMap: reactionTriggerArray){
      const std::string& reactionTriggerString = triggerMap.get(kReactionTriggerKey,
                                                            EnumToString(ReactionTrigger::Count)).asString();
      BehaviorID behaviorID = IBehavior::ExtractBehaviorIDFromConfig(triggerMap);
      
      ReactionTrigger trigger = ReactionTriggerFromString(reactionTriggerString);
      IReactionTriggerStrategy* strategy = ReactionTriggerStrategyFactory::CreateReactionTriggerStrategy(_robot, triggerMap, trigger);
      IBehaviorPtr behavior = _behaviorContainer->FindBehaviorByID(behaviorID);
      
      {
        DEV_ASSERT_MSG(behavior != nullptr, "BehaviorManager.InitReactionTriggerMap.BehaviorNullptr Behavior name",
                       "Behavior name %s returned nullptr from factory", BehaviorIDToString(behaviorID));
        DEV_ASSERT_MSG(strategy != nullptr, "BehaviorManager.InitReactionTriggerMap.StrategyNullptr",
                       "Reaction trigger string %s returned nullptr", reactionTriggerString.c_str());
      }
        
      if(strategy != nullptr && behavior != nullptr){
        PRINT_CH_INFO("ReactionTriggers","BehaviorManager.InitReactionTriggerMap.AddingReactionTrigger",
                      "Strategy %s maps to behavior %s",
                      strategy->GetName().c_str(),
                      BehaviorIDToString(behavior->GetID()));
        
        // Add the strategy to the trigger
        _reactionTriggerMap[trigger].AddStrategyMapping(strategy, behavior);
        DEV_ASSERT(strategy == nullptr,
                   "BehaviorManager.InitReactionTriggerMap.StrategyStillValidAfterAddedToMap");
      }else{
        // Don't leak strategy memory if it doesn't get added to the reaction trigger map
        Util::SafeDelete(strategy);
      }
    }
  }
  
  constexpr auto numReactionTriggers = Util::EnumToUnderlying(ReactionTrigger::Count);
  
  // Currently we load in null configs for tests - so this asserts that if you
  // specify any reaction triggers, you have to specify them all.
  DEV_ASSERT(config.isNull() || (numReactionTriggers == _reactionTriggerMap.size()),
             "BehaviorManager.InitReactionTriggerMap.NoStrategiesAddedForAReactionTrigger");
  
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IBehaviorPtr BehaviorManager::GetCurrentBehavior() const{
  return GetRunningAndResumeInfo().GetCurrentBehavior();
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  bool BehaviorManager::CurrentBehaviorTriggeredAsReaction() const
{
  return GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ReactionTrigger BehaviorManager::GetCurrentReactionTrigger() const
{
  return GetRunningAndResumeInfo().GetCurrentReactionTrigger();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::BroadcastReactionTriggerMap() const {
  std::vector<ReactionTriggerToBehavior> reactionTriggerToBehaviors;

  for(const auto& mapEntry: _reactionTriggerMap){
    ReactionTriggerToBehavior newEntry;
    newEntry.trigger = mapEntry.first;
    
    const std::vector<TriggerBehaviorInfo::StrategyBehaviorMap> &strategyMap = mapEntry.second.GetStrategyMap();
      
    for(const auto& strategy: strategyMap)
    {
        newEntry.behaviorID = strategy.second->GetID();
        reactionTriggerToBehaviors.push_back(newEntry);
    }
  }
  
  _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::RespondReactionTriggerMap>(reactionTriggerToBehaviors);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetDefaultHeadAndLiftState(bool enable, f32 headAngle, f32 liftHeight)
{
  if(enable){
    _defaultHeadAngle = headAngle;
    _defaultLiftHeight = liftHeight;

    PRINT_CH_INFO("Behaviors", "BehaviorManager.DefaultHeadAnfLiftState.Set",
                  "Set to head angle %f, lift height %f",
                  headAngle,
                  liftHeight);
    
    if(_robot.GetActionList().IsEmpty()) {
      IActionRunner* moveHeadAction = new MoveHeadToAngleAction(_robot, _defaultHeadAngle);
      IActionRunner* moveLiftAction = new MoveLiftToHeightAction(_robot, _defaultLiftHeight);
      _robot.GetActionList().QueueAction(QueueActionPosition::NOW,
                                         new CompoundActionParallel(_robot, {moveHeadAction, moveLiftAction}));

      PRINT_CH_INFO("Behaviors", "BehaviorManager.DefaultHeadAnfLiftState.Set.MoveNow",
                    "No action, so doing one now to head angle %f, lift height %f",
                    headAngle,
                    liftHeight);

    }
    
  }else{
    
    PRINT_CH_INFO("Behaviors", "BehaviorManager.DefaultHeadAnfLiftState.Clear",
                  "Clearing default lift and head positions");

    _defaultHeadAngle = kIgnoreDefaultHeadAndLiftState;
    _defaultLiftHeight = kIgnoreDefaultHeadAndLiftState;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::SwitchToBehaviorBase(BehaviorRunningAndResumeInfo& nextBehaviorInfo)
{
  BehaviorRunningAndResumeInfo oldInfo = GetRunningAndResumeInfo();
  IBehaviorPtr nextBehavior = nextBehaviorInfo.GetCurrentBehavior();

  StopAndNullifyCurrentBehavior();
  bool initSuccess = true;
  if( nullptr != nextBehavior ) {
    ANKI_VERIFY(nextBehavior->IsRunnable(_robot),
                "BehaviorManager.SwitchToBehaviorBase.BehaviorNotRunnable",
                "Behavior %s returned but it's not runnable",
                BehaviorIDToString(nextBehavior->GetID()));
    const Result initRet = nextBehavior->Init();
    if ( initRet != RESULT_OK ) {
      // the previous behavior has been told to stop, but no new behavior has been started
      PRINT_NAMED_ERROR("BehaviorManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        BehaviorIDToString(nextBehavior->GetID()));
      nextBehaviorInfo.SetCurrentBehavior(nullptr);
      initSuccess = false;
    }
  }
  
  if( nullptr != nextBehavior ) {
    VIZ_BEHAVIOR_SELECTION_ONLY(
      _robot.GetContext()->GetVizManager()->SendNewBehaviorSelected(
           VizInterface::NewBehaviorSelected( nextBehavior->GetID() )));
  }
  
  SetRunningAndResumeInfo(nextBehaviorInfo);
  SendDasTransitionMessage(oldInfo, GetRunningAndResumeInfo());
  
  return initSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SendDasTransitionMessage(const BehaviorRunningAndResumeInfo& oldBehaviorInfo,
                                               const BehaviorRunningAndResumeInfo& newBehaviorInfo)
{
  // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
  if (!_robot.HasExternalInterface()) {
    return;
  }
  
  const IBehaviorPtr oldBehavior = oldBehaviorInfo.GetCurrentBehavior();
  const IBehaviorPtr newBehavior = newBehaviorInfo.GetCurrentBehavior();
  
  
  BehaviorID oldBehaviorID = nullptr != oldBehavior ? oldBehavior->GetID()  : BehaviorID::Wait;
  BehaviorID newBehaviorID = nullptr != newBehavior ? newBehavior->GetID()  : BehaviorID::Wait;
  
  
  Anki::Util::sEvent("robot.behavior_transition",
                     {{DDATA,
                       BehaviorIDToString(oldBehaviorID)}},
                       BehaviorIDToString(newBehaviorID));

  // if this was the result of a reaction trigger, send that as well
  if( newBehaviorInfo.GetCurrentReactionTrigger() != ReactionTrigger::Count &&
      newBehaviorInfo.GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger ) {
    // s_val = trigger name
    // data = triggered behavior name
    Anki::Util::sEvent("robot.reaction_trigger",
                       {{DDATA, BehaviorIDToString(newBehaviorID)}},
                       ReactionTriggerToString(newBehaviorInfo.GetCurrentReactionTrigger()));
  }
  
  ExternalInterface::BehaviorTransition msg;
  msg.oldBehaviorID = oldBehaviorID;
  msg.oldBehaviorClass = nullptr != oldBehavior ? oldBehavior->GetClass() : BehaviorClass::Wait;
  msg.oldBehaviorExecType = nullptr != oldBehavior ? oldBehavior->GetExecutableType() : ExecutableBehaviorType::Wait;
  msg.newBehaviorID = newBehaviorID;
  msg.newBehaviorClass = nullptr != newBehavior ? newBehavior->GetClass() : BehaviorClass::Wait;
  msg.newBehaviorExecType = nullptr != newBehavior ? newBehavior->GetExecutableType() : ExecutableBehaviorType::Wait;
  msg.newBehaviorDisplayKey = newBehavior ? newBehavior->GetDisplayNameKey() : "";
  _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::BehaviorTransition>(msg);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::SwitchToReactionTrigger(IReactionTriggerStrategy& triggerStrategy, IBehaviorPtr nextBehavior)
{
  // a null here means "no reaction", not "switch to the null behavior"
  if( nullptr == nextBehavior ) {
    return false;
  }
  
  VIZ_BEHAVIOR_SELECTION_ONLY(_robot.GetContext()->GetVizManager()->SendNewReactionTriggered(
                              VizInterface::NewReactionTriggered( triggerStrategy.GetName())));

  BehaviorRunningAndResumeInfo newBehaviorInfo;
  newBehaviorInfo.SetCurrentBehavior(nextBehavior);
  newBehaviorInfo.SetCurrentReactionType(triggerStrategy.GetReactionTrigger());
  
  // Check to see if we should resume the previous behavior
  if(triggerStrategy.ShouldResumeLastBehavior()){
    // if this is the first reactionary behavior, set the current scored behavior as
    // the behavior to resume
    if(nullptr == GetRunningAndResumeInfo().GetBehaviorToResume() &&
       nullptr != GetRunningAndResumeInfo().GetCurrentBehavior()){
      newBehaviorInfo.SetBehaviorToResume(GetRunningAndResumeInfo().GetCurrentBehavior());
    }
    
    // if this is after the first reaction, pass the behavior to resume along
    if(nullptr != GetRunningAndResumeInfo().GetBehaviorToResume()){
      newBehaviorInfo.SetBehaviorToResume(GetRunningAndResumeInfo().GetBehaviorToResume());
    }
  }
  
  return SwitchToBehaviorBase(newBehaviorInfo);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SwitchToUIGameRequestBehavior()
{
  DisableReactionsWithLock(kDisableReactionsUIRequestGameLock, kAffectTriggersUIRequestGame);
  
  BehaviorRunningAndResumeInfo newBehaviorInfo;
  newBehaviorInfo.SetCurrentBehavior(_uiRequestGameBehavior);
  SwitchToBehaviorBase(newBehaviorInfo);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::ChooseNextScoredBehaviorAndSwitch()
{
  // we can't call ChooseNextBehaviorAndSwitch while there's a behavior to resume pending. Call TryToResumeBehavior instead
  DEV_ASSERT(GetRunningAndResumeInfo().GetBehaviorToResume() == nullptr,
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CalledWithResumeBehaviorPending");

  // shouldn't call ChooseNextBehaviorAndSwitch after a reactionary. Call TryToResumeBehavior instead
  DEV_ASSERT(GetRunningAndResumeInfo().GetCurrentReactionTrigger() == ReactionTrigger::NoneTrigger,
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CantSelectAfterReactionary");

  // the current behavior has to be running. Otherwise current should be nullptr
  DEV_ASSERT(GetRunningAndResumeInfo().GetCurrentBehavior() == nullptr ||
             GetRunningAndResumeInfo().GetCurrentBehavior()->IsRunning(),
    "BehaviorManager.ChooseNextBehaviorAndSwitch.CurrentBehaviorIsNotRunning");
 
  // ask the current activity for the next behavior
  IBehaviorPtr nextBehavior = GetCurrentActivity()->
        GetDesiredActiveBehavior(_robot, GetRunningAndResumeInfo().GetCurrentBehavior());
  if(nextBehavior != GetRunningAndResumeInfo().GetCurrentBehavior()){
    BehaviorRunningAndResumeInfo scoredInfo;
    scoredInfo.SetCurrentBehavior(nextBehavior);
    SwitchToBehaviorBase(scoredInfo);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::TryToResumeBehavior()
{
  // Return the robot's head and lift to their height when the behavior that's being resumed was interrupted
  // this is a convenience for behaviors like MeetCozmo when interrupted by reactions with animations
  // we don't care if it actually completes so if the behavior immediately overrides these that's fine
  // if a behavior checks the head or lift angle right as they start this may introduce a race condition
  // blame Brad
  
  if(AreDefaultHeadAndLiftStateSet() && _robot.GetActionList().IsEmpty()) {
    
    PRINT_CH_INFO("Behaviors", "BehaviorManager.DefaultHeadAnfLiftState.ResumeBehavior",
                  "Resuming behavior and don't have an action, so setting head angle %f, lift height %f",
                  _defaultHeadAngle,
                  _defaultLiftHeight);

    IActionRunner* moveHeadAction = new MoveHeadToAngleAction(_robot, _defaultHeadAngle);
    IActionRunner* moveLiftAction = new MoveLiftToHeightAction(_robot, _defaultLiftHeight);
    _robot.GetActionList().QueueAction(QueueActionPosition::NOW,
                                       new CompoundActionParallel(_robot, {moveHeadAction, moveLiftAction}));
  }
    
  if ( GetRunningAndResumeInfo().GetBehaviorToResume() != nullptr)
  {
    StopAndNullifyCurrentBehavior();
    ReactionTrigger resumingFromTrigger = ReactionTrigger::NoneTrigger;
    resumingFromTrigger = GetRunningAndResumeInfo().GetCurrentReactionTrigger();
    
    IBehaviorPtr behaviorToResume = GetRunningAndResumeInfo().GetBehaviorToResume();
    const Result resumeResult = behaviorToResume->Resume(resumingFromTrigger);
    if( resumeResult == RESULT_OK )
    {
      PRINT_CH_INFO("Behaviors", "BehaviorManager.ResumeBehavior", "Successfully resumed '%s'",
        BehaviorIDToString(behaviorToResume->GetID()));
      // the behavior can resume, set as current again
      BehaviorRunningAndResumeInfo newBehaviorInfo;
      newBehaviorInfo.SetCurrentBehavior(behaviorToResume);
      SendDasTransitionMessage(GetRunningAndResumeInfo(), newBehaviorInfo);
      SetRunningAndResumeInfo(newBehaviorInfo);
      // Successfully resumed the previous behavior, return here
      return;
    }
    else {
      PRINT_CH_INFO("Behaviors", "BehaviorManager.ResumeFailed",
        "Tried to resume behavior '%s', but failed. Clearing current behavior",
        BehaviorIDToString(behaviorToResume->GetID()));
    }
  }
  
  // we didn't resume, clear the resume behavior because we won't want to resume it after the next behavior
  GetRunningAndResumeInfo().SetBehaviorToResume(nullptr);
  
  // we did not resume, clear current behavior
  BehaviorRunningAndResumeInfo nullInfo;
  SwitchToBehaviorBase( nullInfo );
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetRunningAndResumeInfo(const BehaviorRunningAndResumeInfo& newInfo)
{
  
  // On any reaction trigger transition, let the game know
  if(_runningAndResumeInfo->GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger ||
     newInfo.GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger){
    _robot.GetExternalInterface()->BroadcastToGame<
      ExternalInterface::ReactionTriggerTransition>(
         _runningAndResumeInfo->GetCurrentReactionTrigger(),
         newInfo.GetCurrentReactionTrigger()
      );
  }
  
  // If switching into or out of reactions fully enable/disable robot properties
  if((newInfo.GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger) &&
     (_runningAndResumeInfo->GetCurrentReactionTrigger() == ReactionTrigger::NoneTrigger))
  {
    UpdateRobotPropertiesForReaction(true);
  }else if((newInfo.GetCurrentReactionTrigger() == ReactionTrigger::NoneTrigger) &&
    (_runningAndResumeInfo->GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger))
  {
    UpdateRobotPropertiesForReaction(false);
  }
  
  //Update behavior light states if appropriate
  if(!_behaviorStateLights.empty() &&
     (newInfo.GetCurrentBehavior() != nullptr) &&
     (newInfo.GetCurrentBehavior()->GetClass() != _behaviorThatSetLights)){
    const bool switchingToReaction =
          (newInfo.GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger) &&
          (_runningAndResumeInfo->GetCurrentReactionTrigger() == ReactionTrigger::NoneTrigger);
    const bool switchingBetweenBehaviors =
          (newInfo.GetCurrentBehavior() != _runningAndResumeInfo->GetCurrentBehavior()) &&
          (newInfo.GetCurrentBehavior() != _runningAndResumeInfo->GetBehaviorToResume());
    
    if(switchingBetweenBehaviors ||
       (switchingToReaction && !_behaviorStateLightsPersistOnReaction)){
      for(const auto& entry: _behaviorStateLights){
        _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(
                    entry.GetAnimationTrigger(), entry.GetObjectID());
      }
      _behaviorStateLights.clear();
    }
  }
  
  *_runningAndResumeInfo = newInfo;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::Update(Robot& robot)
{
  ANKI_CPU_PROFILE("BehaviorManager::Update");
  
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
  
  // Update the currently running activity
  GetCurrentActivity()->Update(robot);
  
  
  // Make sure to clear the current flags if Cozmo is off his treads
  // or if we've transitioned out of freeplay
  if ((robot.GetOffTreadsState() != OffTreadsState::OnTreads) ||
      (_currentHighLevelActivity != HighLevelActivity::Freeplay)) {
    EnsureRequestGameIsClear();
  }
  
  if (_shouldRequestGame){
    // Set IBehaviorPtr for requested game
    SelectUIRequestGameBehavior();
    _shouldRequestGame = false;
    if(GetRunningAndResumeInfo().GetCurrentBehavior() != nullptr &&
       GetRunningAndResumeInfo().GetCurrentBehavior()->GetClass() == BehaviorClass::RequestGameSimple){
      // Transitioning from one request to another - so play an interrupt anim
      _uiRequestGameBehavior->TriggeringAsInterrupt();
    }
  }
  
  
  // Allow reactionary behaviors to request a switch without a message
  const bool switchedFromReactionTrigger = CheckReactionTriggerStrategies();

  if (_uiRequestGameBehavior == nullptr &&
      !switchedFromReactionTrigger)
  {
    const bool currentBehaviorIsReactionary =
                  (GetRunningAndResumeInfo().GetCurrentReactionTrigger() !=
                   ReactionTrigger::NoneTrigger);
    if(!currentBehaviorIsReactionary)
    {
      ChooseNextScoredBehaviorAndSwitch();
    }
  }
  
  // Reaction triggers we just switched to take priority
  if (!switchedFromReactionTrigger &&
      _uiRequestGameBehavior != nullptr)
  {
    auto currentBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
    // Switch into the request game behavior if we haven't already
    if ((_uiRequestGameBehavior != nullptr) && (currentBehavior != _uiRequestGameBehavior)){
      SwitchToUIGameRequestBehavior();
    }
  }
  
  auto currentBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
  if (currentBehavior != nullptr) {
    
    const bool shouldAttemptResume =
      GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger;
    
    // We have a current behavior, update it.
    const IBehavior::Status status = currentBehavior->Update();
     
    switch(status)
    {
      case IBehavior::Status::Running:
        // Nothing to do! Just keep on truckin'....
        break;
          
      case IBehavior::Status::Complete:
        // behavior is complete, switch to null (will also handle stopping current). If it was reactionary,
        // switch now to give the last behavior a chance to resume (if appropriate)
        PRINT_CH_DEBUG("Behaviors", "BehaviorManager.Update.BehaviorComplete",
                       "Behavior '%s' returned  Status::Complete",
                       BehaviorIDToString(currentBehavior->GetID()));
        FinishCurrentBehavior(currentBehavior, shouldAttemptResume);
        break;
          
      case IBehavior::Status::Failure:
        PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                          "Behavior '%s' failed to Update().",
                          BehaviorIDToString(currentBehavior->GetID()));
        // same as the Complete case
        FinishCurrentBehavior(currentBehavior, shouldAttemptResume);
        break;
    } // switch(status)
  }
    
  return lastResult;
} // Update()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetCurrentActivity(HighLevelActivity newActivity, const bool ignorePreviousActivity)
{
  if( _currentHighLevelActivity == newActivity) {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.SetCurrentActivity",
                  "Activity '%s' already set", EnumToString(newActivity));
    return;
  }
  
  if(!ignorePreviousActivity){
    // notify all chooser it's no longer selected
    GetCurrentActivity()->OnDeselected(_robot);
    
    // channeled log and event - legacy event from when activites were just choosers
    // PLEASE DO NOT CHANGE or you will break analytics queries
    LOG_EVENT("BehaviorManager.SetBehaviorChooser",
              "Switching behavior chooser from '%s' to '%s'",
              ActivityIDToString(GetCurrentActivity()->GetID()),
              EnumToString(newActivity));
    
    PRINT_CH_INFO("Behaviors",
                  "BehaviorManager.SetCurrentActivity",
                  "Switching behavior chooser from '%s' to '%s'",
                  ActivityIDToString(GetCurrentActivity()->GetID()),
                  EnumToString(newActivity));
  }
  
  // default head and lift states should not be preserved between choosers
  DEV_ASSERT(!AreDefaultHeadAndLiftStateSet(),
             "BehaviorManager.ChooseNextBehaviorAndSwitch.DefaultHeadAndLiftStatesStillSet");

  const bool currentIsReactionary = CurrentBehaviorTriggeredAsReaction();
  
  // The behavior pointers may no longer be valid, so clear them
  if(!currentIsReactionary){
    BehaviorRunningAndResumeInfo nullInfo;
    SwitchToBehaviorBase(nullInfo);
  }
  
  GetRunningAndResumeInfo().SetBehaviorToResume(nullptr);

  _currentHighLevelActivity = newActivity;
  
  GetCurrentActivity()->OnSelected(_robot);
  
  // mark the time at which the change happened (this is checked by behaviors)
  _lastChooserSwitchTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // tell data tracker what happened
  {
    const bool isPaused = (newActivity != HighLevelActivity::Freeplay);
    _robot.GetAIComponent().GetFreeplayDataTracker().SetFreeplayPauseFlag(isPaused, FreeplayPauseFlag::GameControl);
  }

  // force the new behavior chooser to select something now, instead of waiting for the next tick
  if(!currentIsReactionary){
    ChooseNextScoredBehaviorAndSwitch();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::shared_ptr<IActivity> BehaviorManager::GetCurrentActivity()
{
  DEV_ASSERT(_currentHighLevelActivity < HighLevelActivity::Count,
             "BehaviorManager.GetCurrentActivity.InvalidHighLevelActivity");
  return _highLevelActivityMap[_currentHighLevelActivity];
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RequestCurrentBehaviorEndImmediately(const std::string& stoppedByWhom)
{
  PRINT_CH_INFO("Behaviors",
                "BehaviorManager.RequestCurrentBehaviorEndImmediately",
                "Forcing current behavior to stop: %s",
                stoppedByWhom.c_str());
  BehaviorRunningAndResumeInfo nullInfo;
  SwitchToBehaviorBase(nullInfo);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::StopAndNullifyCurrentBehavior()
{
  IBehaviorPtr currentBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
  
  if ( nullptr != currentBehavior && currentBehavior->IsRunning() ) {
    currentBehavior->Stop();
  }
  
  GetRunningAndResumeInfo().SetCurrentBehavior(nullptr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::FinishCurrentBehavior(IBehaviorPtr currentBehavior, bool shouldAttemptResume)
{
  // Currently we assume that game requests do not have a resume behavior, so we should
  // not attempt to resume if _uiRequestGameBehavior is non-null and just finished
  DEV_ASSERT(!(shouldAttemptResume &&
              ((_uiRequestGameBehavior != nullptr) && (currentBehavior == _uiRequestGameBehavior))),
             "BehaviorManager.FinishCurrentBehavior.ResumingRequestGame");
  
  if (shouldAttemptResume) {
    TryToResumeBehavior();
  }
  else {
    // Make sure the request behavior is cleared out after it finishes
    // running so that we can pick a new behavior
    if ((_uiRequestGameBehavior != nullptr) && (currentBehavior == _uiRequestGameBehavior)) {
      EnsureRequestGameIsClear();
    }
        
    BehaviorRunningAndResumeInfo nullInfo;
    SwitchToBehaviorBase(nullInfo);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SelectUIRequestGameBehavior()
{
  auto& gameSelector = _robot.GetAIComponent().GetRequestGameComponent();
  UnlockId gameRequestID = gameSelector.IdentifyNextGameTypeToRequest(_robot);
  if(gameRequestID != UnlockId::Count) {
    const auto& iter = _uiGameRequestMap.find(gameRequestID);
    if(ANKI_VERIFY(iter != _uiGameRequestMap.end(),
                     "BehaviorManager.Update.ChooseNextRequestBehaviorInternal",
                     "Unlock ID %s not found in ui game request map",
                     UnlockIdToString(gameRequestID))) {
      _uiRequestGameBehavior = iter->second;
      
      // We already check that the player can afford the cost Game side
      const u32 sparkCost = GetSparkCosts(SparkableThings::PlayAGame, 0);
      _robot.GetInventoryComponent().AddInventoryAmount(InventoryType::Sparks, -sparkCost);
      _robot.GetAIComponent().GetWhiteboard().SetCurrentGameRequestUIRequest(true);
      
      // Send DAS event with cost of the random game
      // s_val = name of the game requested
      // data = cost to request game
      Anki::Util::sEvent("meta.spark_random_game",
                         {{DDATA, std::to_string(sparkCost).c_str()}},
                         _uiRequestGameBehavior->GetIDStr().c_str());
      
    }
  }
  else {
    _uiRequestGameBehavior = nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::EnsureRequestGameIsClear()
{
  if(_uiRequestGameBehavior != nullptr){
    RemoveDisableReactionsLock(kDisableReactionsUIRequestGameLock);
  }

  _robot.GetAIComponent().GetWhiteboard().SetCurrentGameRequestUIRequest(false);
  _uiRequestGameBehavior = nullptr;
  _shouldRequestGame = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass BehaviorManager::GetBehaviorClass(IBehaviorPtr behavior) const
{
  return behavior->GetClass();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::CheckReactionTriggerStrategies()
{
  // Hack related to COZMO-14207 - Since we currently have no knowledge of
  // whether Cozmo is starting up with a wakeup that we shouldn't react during
  // or is in DEV webots/SDK mode, and to future proof future paths, don't
  // react until the first action is queued, indicating that there has been time
  // for any desired enables/disables to be put in place by whatever is controlling
  // engine
  _hasActionQueuedOrSDKReactEnabled |= !_robot.GetActionList().IsEmpty();
  if(!_hasActionQueuedOrSDKReactEnabled){
    return false;
  }
  
  // Check to see if any reaction triggers want to activate a behavior
  bool hasAlreadySwitchedThisTick = false;
  bool didSuccessfullySwitch = false;
  
  for(const auto& mapEntry: _reactionTriggerMap){
    // if a reaction is not enabled, skip evaluating its strategies
    if(!mapEntry.second.IsReactionEnabled()){
      continue;
    }
    
    const auto& strategyMap = mapEntry.second.GetStrategyMap();
    
    for(const auto& entry: strategyMap){
      IReactionTriggerStrategy& strategy = *entry.first;
      IBehaviorPtr rBehavior = entry.second;

      bool shouldCheckStrategy = true;
      
      // If there is a current triggered behavior running, make sure
      //  we are allowed to interrupt it.
      const ReactionTrigger currentReactionTrigger = GetCurrentReactionTrigger();
      if (currentReactionTrigger != ReactionTrigger::NoneTrigger)
      {
        shouldCheckStrategy = (currentReactionTrigger == strategy.GetReactionTrigger()) ?
                              strategy.CanInterruptSelf() :
                              strategy.CanInterruptOtherTriggeredBehavior();
      }
      
      if(shouldCheckStrategy &&
         strategy.ShouldTriggerBehavior(_robot, rBehavior)){
        
          _robot.GetMoveComponent().StopAllMotors();

        
          if(_robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::ALL_TRACKS) &&
             !_robot.GetMoveComponent().IsDirectDriving())
          {
            PRINT_NAMED_WARNING("BehaviorManager.CheckReactionTriggerStrategies",
                                "Some tracks are locked, unlocking them");
            _robot.GetMoveComponent().CompletelyUnlockAllTracks();
          }
        
          const bool successfulSwitch = SwitchToReactionTrigger(strategy, rBehavior);
        
          didSuccessfullySwitch |= successfulSwitch;

          if(successfulSwitch){
            PRINT_CH_INFO("ReactionTriggers", "BehaviorManager.CheckReactionTriggerStrategies.SwitchingToReaction",
                          "Trigger strategy %s triggering behavior %s",
                          strategy.GetName().c_str(),
                          BehaviorIDToString(rBehavior->GetID()));
          }else{
            PRINT_CH_INFO("ReactionTriggers", "BehaviorManager.CheckReactionTriggerStrategies.FailedToSwitch",
                          "Trigger strategy %s tried to trigger behavior %s, but init failed",
                          strategy.GetName().c_str(),
                          BehaviorIDToString(rBehavior->GetID()));
          }
      
          if(hasAlreadySwitchedThisTick){
            PRINT_NAMED_WARNING("BehaviorManager.Update.ReactionaryBehaviors",
                              "Multiple behaviors switched to in a single basestation tick");
          }
          hasAlreadySwitchedThisTick = true;
      }
    }
  }
  
  return didSuccessfullySwitch;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::UpdateRobotPropertiesForReaction(bool enablingReaction)
{
  // During reactions prevent DirectDrive messages and external action queueing messages
  // from doing anything
  _robot.GetMoveComponent().IgnoreDirectDriveMessages(enablingReaction);
  _robot.SetIgnoreExternalActions(enablingReaction);
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::HandleMessage(const Anki::Cozmo::ExternalInterface::BehaviorManagerMessageUnion& message)
{
  switch (message.GetTag())
  {
    // Sparks
    case ExternalInterface::BehaviorManagerMessageUnionTag::ActivateSpark:
    {
      const auto& msg = message.Get_ActivateSpark();
      SetRequestedSpark(msg.behaviorSpark, false);
      if(msg.behaviorSpark == UnlockId::Count){
        _didGameRequestSparkEnd = true;
      }
      break;
    }
    
    case ExternalInterface::BehaviorManagerMessageUnionTag::SparkUnlocked:
    {
      const auto& msg = message.Get_SparkUnlocked();
      SetRequestedSpark(msg.behaviorSpark, true);
      break;
    }
    
    // User asked for a random trick via UI
    case ExternalInterface::BehaviorManagerMessageUnionTag::DoATrickRequest:
    {
      _robot.GetAIComponent().GetDoATrickSelector().RequestATrick(_robot);
      
      // We already check that the player can afford the cost Game side
      const u32 sparkCost = GetSparkCosts(SparkableThings::DoATrick, 0);
      _robot.GetInventoryComponent().AddInventoryAmount(InventoryType::Sparks, -sparkCost);
      
      // Send DAS event with cost of the random trick
      if(GetRequestedSpark() != UnlockId::Invalid){
        // s_val = name of the trick selected
        // data = cost to perform trick
        Anki::Util::sEvent("meta.spark_random_trick",
                           {{DDATA, std::to_string(sparkCost).c_str()}},
                           UnlockIdToString(GetRequestedSpark()));
      }
      
      break;
    }
    
    // User asked for a random game via UI
    case ExternalInterface::BehaviorManagerMessageUnionTag::PlayAGameRequest:
    {
      if (_uiRequestGameBehavior == nullptr ||
          _uiRequestGameBehavior->IsRunning()) {
        _shouldRequestGame = true;
      }
      break;
    }

    default:
    {
      PRINT_NAMED_ERROR("BehaviorManager.HandleEvent.UnhandledMessageUnionTag",
                        "Unexpected tag %u '%s'", (uint32_t)message.GetTag(),
                        BehaviorManagerMessageUnionTagToString(message.GetTag()));
      assert(0);
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::CalculateActivityFreeplayFromObjects()
{
  // TODO: this will no longer be necessary after COZMO-10658
  // design: the choosers are generic, but this only makes sense if the freeplay chooser is an ActivityFreeplay. I
  // think that we should not be able to data-drive it, but there may be a case for it, and this method
  // should just not work in that case. Right now I feel like using dynamic_cast, but we could provide default
  // implementation in chooser interface that asserts to not be used.
  std::shared_ptr<IActivity> freeplayActivity = _highLevelActivityMap[HighLevelActivity::Freeplay];

  IActivity* freeplayActivityRaw = freeplayActivity.get();
  ActivityFreeplay* ActivityFreeplayEvaluator = dynamic_cast<ActivityFreeplay*>(freeplayActivityRaw);
  if ( nullptr != ActivityFreeplayEvaluator )
  {
    ActivityFreeplayEvaluator->CalculateDesiredActivityFromObjects();
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorManager.CalculateActivityFreeplayFromObjects.NotUsingActivityFreeplay",
      "The current freeplay chooser is not a freeplay activity.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetRequestedSpark(UnlockId spark, bool softSpark)
{
  _lastRequestedSpark = spark;
  _isRequestedSparkSoft = softSpark;

  PRINT_CH_INFO("Behaviors", "BehaviorManager.SetRequestedSpark",
                "requested %s spark is '%s'",
                _isRequestedSparkSoft ? "soft" : "hard",
                UnlockIdToString(_lastRequestedSpark));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::IsReactionTriggerEnabled(ReactionTrigger reaction) const
{
  auto triggerMapIter = _reactionTriggerMap.find(reaction);
  if(ANKI_VERIFY(triggerMapIter != _reactionTriggerMap.end(),
                 "BehaviorManager.IsReactionTriggerEnabled.ReachedEndOfMap",
                 "Reached the end of the reaction trigger map searching for %d",
                 Util::numeric_cast<int>(reaction))){
    return triggerMapIter->second.IsReactionEnabled();
  }
  
  return false;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorManager::FindBehaviorByID(BehaviorID behaviorID) const
{
  return _behaviorContainer->FindBehaviorByID(behaviorID);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorManager::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  return _behaviorContainer->FindBehaviorByExecutableType(type);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const UnlockId BehaviorManager::SwitchToRequestedSpark()
{
  PRINT_CH_INFO("Behaviors", "BehaviorManager.SwitchToRequestedSpark",
                "switching active spark from '%s' -> '%s'. From %s -> %s",
                UnlockIdToString(_activeSpark),
                UnlockIdToString(_lastRequestedSpark),
                _isSoftSpark ? "soft" : "hard",
                _isRequestedSparkSoft ? "soft" : "hard");

  _lastChooserSwitchTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  
  _activeSpark = _lastRequestedSpark;
  _isSoftSpark = _isRequestedSparkSoft;
  _didGameRequestSparkEnd = false;
  
  return _activeSpark;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::DisableReactionsWithLock(
          const std::string& lockID,
          const ReactionTriggerHelpers::FullReactionArray& triggersAffected,
          bool stopCurrent)
{
  const bool sdkDifferentParadigmLock = (lockID == "sdk");
  /// Iterate over all reaction triggers to see if they're affected by this request
  for(auto& entry: _reactionTriggerMap){
    const ReactionTrigger triggerEnum = entry.first;
    auto& allStrategyMaps = entry.second;
    
    const bool lockNotAlreadyInUse = !allStrategyMaps.IsTriggerLockedByID(lockID);
    // SDK wants to be allowed to lock a reaction with the same id multiple times
    // and then have remove lock remove all instances.  So only DEV_ASSERT if
    // we aren't in SDK mode
    DEV_ASSERT_MSG(sdkDifferentParadigmLock || lockNotAlreadyInUse,
                   "BehaviorManager.DisableReactionsWithLock.LockAlreadyInUse",
                   "Attempted to disable reactions with ID %s which is already in use",
                   lockID.c_str());
    
    if(ReactionTriggerHelpers::IsTriggerAffected(triggerEnum, triggersAffected)){
      PRINT_CH_INFO("ReactionTriggers",
                    "BehaviorManager.DisableReactionsWithLock.DisablingWithLock",
                    "Trigger %s is being disabled by %s",
                    EnumToString(triggerEnum),
                    lockID.c_str());
      

      if(allStrategyMaps.IsReactionEnabled()){
        //About to disable, notify reaction trigger strategy
        const auto& allEntries = allStrategyMaps.GetStrategyMap();
        for(auto& entry : allEntries){
          entry.first->EnabledStateChanged(_robot, false);
        }
      }
      
      // This check prevents an invalid DEV_ASSERT hit caused by the fact
      // that this function is overloaded to support two different paradigms
      // In the case of SDK lock, we should not call the add disableLockToTrigger
      // because they follow a different paradigm
      // otherwise, we should have crashed on the DEV_ASSERT above
      // The DEV_ASSERT is preserved within AddDisableLockToTrigger just in case
      // in the future some other function tries to add disable locks
      if(lockNotAlreadyInUse){
        allStrategyMaps.AddDisableLockToTrigger(lockID, triggerEnum);
      }
      
      // If the currently running behavior was triggered as a reaction
      // and we are supposed to stop current, stop the current behavior
      if(stopCurrent &&
         _runningAndResumeInfo->GetCurrentReactionTrigger() == triggerEnum){
        PRINT_CH_INFO("ReactionTriggers",
                      "BehaviorManager.DisableReactionsWithLock",
                      "Disabling reaction triggers - stopping currently running one");
        BehaviorRunningAndResumeInfo nullInfo;
        SwitchToBehaviorBase(nullInfo);
      }
    }
  }
}

#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::DisableReactionWithLock(const std::string& lockID,
                                              const ReactionTrigger& trigger,
                                              bool stopCurrent)
{
  /// Iterate over all reaction triggers to see if they're affected by this request
  for(auto& entry: _reactionTriggerMap)
  {
    const ReactionTrigger triggerEnum = entry.first;
    auto& allStrategyMaps = entry.second;
    
    if(allStrategyMaps.IsReactionEnabled())
    {
      //About to disable, notify reaction trigger strategy
      const auto& allEntries = allStrategyMaps.GetStrategyMap();
      for(auto& entry : allEntries)
      {
        entry.first->EnabledStateChanged(_robot, false);
      }
    }
    
    allStrategyMaps.AddDisableLockToTrigger(lockID, triggerEnum);
    
    // If the currently running behavior was triggered as a reaction
    // and we are supposed to stop current, stop the current behavior
    if(stopCurrent &&
       _runningAndResumeInfo->GetCurrentReactionTrigger() == triggerEnum)
    {
      IBehaviorPtr currentBehavior = _runningAndResumeInfo->GetCurrentBehavior();
      if(currentBehavior!= nullptr && currentBehavior->IsRunning())
      {
        currentBehavior->Stop();
      }
    }
  }
}
#endif

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RemoveDisableReactionsLock(const std::string& lockID)
{
  // If SDK lock is being removed, ensure the special action queued/reaction
  // tracker is set appropriately
  const bool sdkDifferentParadigmLock = (lockID == "sdk");
  _hasActionQueuedOrSDKReactEnabled |= sdkDifferentParadigmLock;
  
  /// Iterate over all reaction triggers to see if they're affected by this request
  for(auto& entry: _reactionTriggerMap){
    const ReactionTrigger triggerEnum = entry.first;
    auto& allStrategyMaps = entry.second;
    
    if(allStrategyMaps.IsTriggerLockedByID(lockID)){
      PRINT_CH_INFO("ReactionTriggers",
                    "BehaviorManager.RemoveDisableReactionsLock.RemovingLock",
                    "Lock %s is being removed from trigger %s",
                    lockID.c_str(),
                    EnumToString(triggerEnum));
      
      allStrategyMaps.RemoveDisableLockFromTrigger(lockID, triggerEnum);
      
      if(allStrategyMaps.IsReactionEnabled()){
        PRINT_CH_INFO("ReactionTriggers",
                      "BehaviorManager.RemoveDisableReactionsLock.ReactionReEnabled",
                      "No remaining locks on trigger %s",
                      EnumToString(triggerEnum));
        
        //About to enable, notify reaction trigger strategy
        const auto& allEntries = allStrategyMaps.GetStrategyMap();
        for(auto& entry : allEntries){
          entry.first->EnabledStateChanged(_robot, true);
        }
      }
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RequestCurrentBehaviorEndOnNextActionComplete()
{
  if(nullptr != GetRunningAndResumeInfo().GetCurrentBehavior()){
    GetRunningAndResumeInfo().GetCurrentBehavior()->StopOnNextActionComplete();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetBehaviorStateLights(BehaviorClass classSettingLights,
                                             const std::vector<BehaviorStateLightInfo>& structToSet,
                                             bool persistOnReaction)
{
  _behaviorStateLightsPersistOnReaction = persistOnReaction;
  PRINT_CH_INFO("Behaviors", "SetBehaviorStateLights.UpdatingStateLights",
                "Updating behavior state lights");
  // If there are current light states, either transition cleanly between
  // the old light state and the new light state, or stop the old light state
  std::set<u32> objectsSet;
  for(const auto& old: _behaviorStateLights){
    bool lightsStopped = false;
    for(const auto& newStruct : structToSet){
      if(old.GetObjectID() == newStruct.GetObjectID()){
        ObjectLights lightModifier;
        const bool hasModifier = newStruct.GetLightModifier(lightModifier);
        if(!_robot.GetCubeLightComponent().
             StopAndPlayLightAnim(newStruct.GetObjectID(),
                                  old.GetAnimationTrigger(),
                                  newStruct.GetAnimationTrigger(),
                                  nullptr,
                                  hasModifier,
                                  lightModifier)){
          PRINT_CH_INFO("Behaviors", "SetBehaviorStateLights.UpdatingStateLights.FailedToPlayLights",
                        "Failed to play light trigger %d on cube %d",
                        newStruct.GetAnimationTrigger(), newStruct.GetObjectID().GetValue());
        }
        lightsStopped = true;
        objectsSet.insert(newStruct.GetObjectID());
        break;
      }
    }
    
    if(!lightsStopped){
      _robot.GetCubeLightComponent().
        StopLightAnimAndResumePrevious(old.GetAnimationTrigger(), old.GetObjectID());
    }
  }
  
  
  // Start the lights playing on any remaining cubes
  for(const auto& entry: structToSet){
    if(objectsSet.find(entry.GetObjectID()) == objectsSet.end()){
      ObjectLights lightModifier;
      const bool hasModifier = entry.GetLightModifier(lightModifier);
      if(!_robot.GetCubeLightComponent().PlayLightAnim(entry.GetObjectID(),
                                                       entry.GetAnimationTrigger(),
                                                       nullptr,
                                                       hasModifier,
                                                       lightModifier)){
        PRINT_CH_INFO("Behaviors", "SetBehaviorStateLights.UpdatingStateLights.FailedToPlayLights",
                      "Failed to play light trigger %d on cube %d",
                      entry.GetAnimationTrigger(), entry.GetObjectID().GetValue());
      }
    }
  }
  
  _behaviorStateLights = structToSet;
  _behaviorThatSetLights = classSettingLights;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::OnRobotDelocalized()
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::list<const IActivity* const> BehaviorManager::GetNonSparkFreeplayActivities() const
{
  std::list<const IActivity* const> activities;
  
  const auto& freeplayActivity = _highLevelActivityMap.find(HighLevelActivity::Freeplay);
  if(freeplayActivity != _highLevelActivityMap.end())
  {
    const ActivityFreeplay* freeplay = static_cast<ActivityFreeplay*>(freeplayActivity->second.get());
    if(freeplay != nullptr)
    {
      const auto& freeplaySubActivities = freeplay->GetFreeplayActivities();
      const auto& nonSparkActivities = freeplaySubActivities.find(UnlockId::Count);
      if(nonSparkActivities != freeplaySubActivities.end())
      {
        for(auto iter = nonSparkActivities->second.begin();
            iter != nonSparkActivities->second.end(); ++iter)
        {
          activities.push_back(static_cast<const IActivity* const>(iter->get()));
        }
      }
    }
  }
  
  return activities;
}

  
} // namespace Cozmo
} // namespace Anki
