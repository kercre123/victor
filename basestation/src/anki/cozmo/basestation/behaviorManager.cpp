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

#include "anki/cozmo/basestation/behaviorManager.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChooserFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoal.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalEvaluator.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/objectInteractionInfoCache.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/messageHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodDebug.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngineTag.h"
#include "clad/types/behaviorChooserType.h"
#include "clad/types/behaviorTypes.h"
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
  
namespace{
static const char* kSelectionChooserConfigKey = "selectionBehaviorChooserConfig";
static const char* kFreeplayChooserConfigKey = "freeplayBehaviorChooserConfig";
static const char* kMeetCozmoChooserConfigKey = "meetCozmoBehaviorChooserConfig";
static const char* kVoiceCommandChooserConfigKey = "vcBehaviorChooserConfig";
  
// reaction trigger behavior map
static const char* kReactionTriggerBehaviorMapKey = "reactionTriggerBehaviorMap";
static const char* kReactionTriggerKey = "reactionTrigger";
static const char* kReactionBehaviorKey = "behaviorName";

// This macro uses PRINT_NAMED_INFO if the supplied define (first arg) evaluates to true, and PRINT_NAMED_DEBUG otherwise
// All args following the first are passed directly to the chosen print macro
#define BEHAVIOR_VERBOSE_PRINT(_BEHAVIORDEF, ...) do { \
if ((_BEHAVIORDEF)) { PRINT_NAMED_INFO( __VA_ARGS__ ); } \
else { PRINT_NAMED_DEBUG( __VA_ARGS__ ); } \
} while(0) \

}

/////////
// Running/Resume implementation
/////////
  
// struct which defines information about the currently running behavior
struct BehaviorRunningAndResumeInfo{
  void SetCurrentBehavior(IBehavior* newScoredBehavior){_currentBehavior = newScoredBehavior;}
  // return the active behavior based on the category set in the struct
  IBehavior* GetCurrentBehavior() const{return _currentBehavior;}
  
  void SetBehaviorToResume(IBehavior* resumeBehavior){
    _behaviorToResume = resumeBehavior;
  }
  IBehavior* GetBehaviorToResume() const{ return _behaviorToResume;}
  
  void SetCurrentReactionType(ReactionTrigger trigger){
    DEV_ASSERT(trigger != ReactionTrigger::Count, "Invalid ReactionTrigger state");
    _currentReactionType = trigger;
  }
  
  ReactionTrigger GetCurrentReactionTrigger() const{ return _currentReactionType;}
  
private:
  // only one behavior should be active at a time
  // either a scored behavior or a reactionary behavior
  IBehavior* _currentBehavior = nullptr;
  // the scored behavior to resume once a reactionary behavior ends
  IBehavior* _behaviorToResume = nullptr;
  ReactionTrigger _currentReactionType = ReactionTrigger::NoneTrigger;
};
  
  
/////////
// TriggerBehaviorInfo implementation
/////////
  
struct TriggerBehaviorInfo{
public:
  using StrategyBehaviorMap = std::pair<std::unique_ptr<IReactionTriggerStrategy>, IBehavior*>;

  bool  IsReactionEnabled() const { return _disableIDs.empty();}
  
  bool AddStrategyMapping(IReactionTriggerStrategy*& strategy, IBehavior* behavior);
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
bool TriggerBehaviorInfo::AddStrategyMapping(IReactionTriggerStrategy*& strategy, IBehavior* behavior)
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
    PRINT_NAMED_WARNING("TriggerBehaviorInfo.AddDisableLockToTrigger.DuplicatDisable",
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
, _behaviorFactory(new BehaviorFactory())
, _lastChooserSwitchTime(-1.0f)
, _audioClient( new Audio::BehaviorAudioClient(robot) )
, _behaviorThatSetLights(BehaviorClass::NoneBehavior)
, _behaviorStateLightsPersistOnReaction(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorManager::~BehaviorManager()
{
  // the behaviors in_reactionTriggerMap are factory behaviors, so the factory will delete them
  // the reactionTriggers are unique ptrs, so they will be deleted by the clear
  _reactionTriggerMap.clear();
  
  // destroy choosers before factory
  Util::SafeDelete(_freeplayChooser);
  Util::SafeDelete(_selectionChooser);
  Util::SafeDelete(_meetCozmoChooser);
  Util::SafeDelete(_voiceCommandChooser);
  
  Util::SafeDelete(_behaviorFactory);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::InitConfiguration(const Json::Value &config)
{
  BEHAVIOR_VERBOSE_PRINT(DEBUG_BEHAVIOR_MGR, "BehaviorManager.Init.Initializing", "");
  
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(!_isInitialized, "BehaviorManager.InitConfiguration.AlreadyInitialized");

  // create choosers
  if ( !config.isNull() )
  {
    // selection chooser - to force one specific behavior
    const Json::Value& selectionChooserConfigJson = config[kSelectionChooserConfigKey];
    _selectionChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, selectionChooserConfigJson);
    
    // freeplay chooser - AI controls cozmo
    const Json::Value& freeplayChooserConfigJson = config[kFreeplayChooserConfigKey];
    _freeplayChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, freeplayChooserConfigJson);

    // meetCozmo chooser
    const Json::Value& meetCozmoChooserConfigJson = config[kMeetCozmoChooserConfigKey];
    _meetCozmoChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, meetCozmoChooserConfigJson);
    
    // voice command chooser
    const Json::Value& voiceCommandChooserConfigJson = config[kVoiceCommandChooserConfigKey];
    _voiceCommandChooser = BehaviorChooserFactory::CreateBehaviorChooser(_robot, voiceCommandChooserConfigJson);
    
    // start with selection that defaults to NoneBehavior
    SetBehaviorChooser( _selectionChooser );

    BehaviorFactory& behaviorFactory = GetBehaviorFactory();
    
    uint8_t numEntriesOfExecutableType[(size_t)ExecutableBehaviorType::Count] = {0};
    
    for( const auto& it : behaviorFactory.GetBehaviorMap() ) {
      IBehavior* behaviorPtr = it.second;
      const ExecutableBehaviorType executableBehaviorType = behaviorPtr->GetExecutableType();
      const size_t eBT = (size_t)executableBehaviorType;
      if (eBT < (size_t)ExecutableBehaviorType::Count)
      {
        DEV_ASSERT_MSG((numEntriesOfExecutableType[eBT] == 0), "ExecutableBehaviorType.NotUnique",
                       "Multiple behaviors marked as %s including '%s'",
                       EnumToString(executableBehaviorType), it.first.c_str());
        ++numEntriesOfExecutableType[eBT];
      }
    }
    
    #if (DEV_ASSERT_ENABLED)
    for( size_t i = 0; i < (size_t)ExecutableBehaviorType::Count; ++i)
    {
      const ExecutableBehaviorType executableBehaviorType = (ExecutableBehaviorType)i;
      DEV_ASSERT_MSG((numEntriesOfExecutableType[i] == 1), "ExecutableBehaviorType.NotExactlyOne",
                     "Should be exactly 1 behavior marked as %s but found %u",
                     EnumToString(executableBehaviorType), numEntriesOfExecutableType[i]);
    }
    #endif
    
  }
  
  // Populate our list of tapInteraction behaviors which should exist in the factory as
  // InitConfiguration() is called after all behaviors have been loaded
  const auto& behaviorMap = GetBehaviorFactory().GetBehaviorMap();
  for(const auto& pair : behaviorMap)
  {
    if(pair.second->IsBehaviorGroup(BehaviorGroup::ObjectTapInteraction))
    {
      _tapInteractionBehaviors.push_back(pair.second);
    }
  }
  
  if (_robot.HasExternalInterface())
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
                                     ReactionTriggerHelpers::kAffectAllArray,
                                     true);
                            }));
    
    _eventHandlers.push_back(externalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ActivateBehaviorChooser,
                               [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
                               {
                                 const BehaviorChooserType chooserType =
                                   event.GetData().Get_ActivateBehaviorChooser().behaviorChooserType;
                                 switch (chooserType)
                                 {
                                   case BehaviorChooserType::Freeplay:
                                   {
                                     if( _firstTimeFreeplayStarted < 0.0f ) {
                                       const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
                                       _firstTimeFreeplayStarted = currTime_s;
                                     }
                                     SetBehaviorChooser( _freeplayChooser );
                                     break;
                                   }
                                   case BehaviorChooserType::Selection:
                                   {
                                     SetBehaviorChooser( _selectionChooser );
                                     break;
                                   }
                                   case BehaviorChooserType::MeetCozmoFindFaces:
                                   {
                                     SetBehaviorChooser(_meetCozmoChooser);
                                     break;
                                   }
                                   default:
                                   {
                                     PRINT_NAMED_WARNING("BehaviorManager.ActivateBehaviorChooser.InvalidChooser",
                                                         "don't know how to create a chooser of type '%s'",
                                                         BehaviorChooserTypeToString(chooserType));
                                     break;
                                   }
                                 }
                                 
                                 // If we are leaving freeplay, ensure that sparks
                                 // have been cleared out
                                 if(chooserType != BehaviorChooserType::Freeplay){
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
  }
  _isInitialized = true;
    
  return RESULT_OK;
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
      const std::string& behaviorName = triggerMap.get(kReactionBehaviorKey,"").asString();
      
      ReactionTrigger trigger = ReactionTriggerFromString(reactionTriggerString);
      IReactionTriggerStrategy* strategy = ReactionTriggerStrategyFactory::CreateReactionTriggerStrategy(_robot, triggerMap, trigger);
      IBehavior* behavior = _behaviorFactory->FindBehaviorByName(behaviorName);
      
      // If voice recognition is turned off don't bother with the below asserts when we get to the VC reaction trigger
#if (VOICE_RECOG_PROVIDER == VOICE_RECOG_NONE)
      if (trigger != ReactionTrigger::VC)
#endif // (VOICE_RECOG_PROVIDER == VOICE_RECOG_NONE)
      {
        DEV_ASSERT_MSG(behavior != nullptr, "BehaviorManager.InitReactionTriggerMap.BehaviorNullptr Behavior name",
                       "Behavior name %s returned nullptr from factory", behaviorName.c_str());
        DEV_ASSERT_MSG(strategy != nullptr, "BehaviorManager.InitReactionTriggerMap.StrategyNullptr",
                       "Reaction trigger string %s returned nullptr", reactionTriggerString.c_str());
      }
        
      if(strategy != nullptr && behavior != nullptr){
        PRINT_CH_INFO("ReactionTriggers","BehaviorManager.InitReactionTriggerMap.AddingReactionTrigger",
                         "Strategy %s maps to behavior %s",
                         strategy->GetName().c_str(), behavior->GetName().c_str());
        
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
  
  // If voice recognition is turned off, we won't have a config for the voice command reaction trigger
#if (VOICE_RECOG_PROVIDER == VOICE_RECOG_NONE)
  constexpr auto numReactionTriggers = (Util::EnumToUnderlying(ReactionTrigger::Count)) - 1;
#else
  constexpr auto numReactionTriggers = Util::EnumToUnderlying(ReactionTrigger::Count);
#endif // (VOICE_RECOG_PROVIDER == VOICE_RECOG_NONE)
  
  // Currently we load in null configs for tests - so this asserts that if you
  // specify any reaction triggers, you have to specify them all.
  DEV_ASSERT(config.isNull() || (numReactionTriggers == _reactionTriggerMap.size()),
             "BehaviorManager.InitReactionTriggerMap.NoStrategiesAddedForAReactionTrigger");
  
  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorManager::CreateBehaviorFromConfiguration(const Json::Value& behaviorJson)
{
  // try to create behavior, name should be unique here
  IBehavior* newBehavior = _behaviorFactory->CreateBehavior(behaviorJson, _robot, BehaviorFactory::NameCollisionRule::Fail);
  const Result ret = (nullptr != newBehavior) ? RESULT_OK : RESULT_FAIL;
  return ret;
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IBehavior* BehaviorManager::GetCurrentBehavior() const{
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
void BehaviorManager::SetDefaultHeadAndLiftState(bool enable, f32 headAngle, f32 liftHeight)
{
  if(enable){
    _defaultHeadAngle = headAngle;
    _defaultLiftHeight = liftHeight;
    
    if(_robot.GetActionList().GetNumQueues() > 0 && _robot.GetActionList().GetQueueLength(0) == 0) {
      IActionRunner* moveHeadAction = new MoveHeadToAngleAction(_robot, _defaultHeadAngle);
      IActionRunner* moveLiftAction = new MoveLiftToHeightAction(_robot, _defaultLiftHeight);
      _robot.GetActionList().QueueAction(QueueActionPosition::NOW,
                                         new CompoundActionParallel(_robot, {moveHeadAction, moveLiftAction}));
    }
    
  }else{
    _defaultHeadAngle = kIgnoreDefaultHeadAndLiftState;
    _defaultLiftHeight = kIgnoreDefaultHeadAndLiftState;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::SwitchToBehaviorBase(BehaviorRunningAndResumeInfo& nextBehaviorInfo)
{
  BehaviorRunningAndResumeInfo oldInfo = GetRunningAndResumeInfo();
  IBehavior* nextBehavior = nextBehaviorInfo.GetCurrentBehavior();

  StopAndNullifyCurrentBehavior();
  bool initSuccess = true;
  if( nullptr != nextBehavior ) {
    const Result initRet = nextBehavior->Init();
    if ( initRet != RESULT_OK ) {
      // the previous behavior has been told to stop, but no new behavior has been started
      PRINT_NAMED_ERROR("BehaviorManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        nextBehavior->GetName().c_str());
      nextBehaviorInfo.SetCurrentBehavior(nullptr);
      initSuccess = false;
    }
  }
  
  if( nullptr != nextBehavior ) {
    VIZ_BEHAVIOR_SELECTION_ONLY(
      _robot.GetContext()->GetVizManager()->SendNewBehaviorSelected(
           VizInterface::NewBehaviorSelected( nextBehavior->GetName() )));
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
  
  const IBehavior* oldBehavior = oldBehaviorInfo.GetCurrentBehavior();
  const IBehavior* newBehavior = newBehaviorInfo.GetCurrentBehavior();
  
  
  const std::string& oldBehaviorName = nullptr != oldBehavior ? oldBehavior->GetName() : "NULL";
  const std::string& newBehaviorName = nullptr != newBehavior ? newBehavior->GetName() : "NULL";
  
  
  Anki::Util::sEvent("robot.behavior_transition",
                     {{DDATA, oldBehaviorName.c_str()}},
                     newBehaviorName.c_str());
  
  ExternalInterface::BehaviorTransition msg;
  msg.oldBehaviorName = oldBehaviorName;
  msg.oldBehaviorClass = nullptr != oldBehavior ? oldBehavior->GetClass() : BehaviorClass::NoneBehavior;
  msg.oldBehaviorExecType = nullptr != oldBehavior ? oldBehavior->GetExecutableType() : ExecutableBehaviorType::NoneBehavior;
  msg.newBehaviorName = newBehaviorName;
  msg.newBehaviorClass = nullptr != newBehavior ? newBehavior->GetClass() : BehaviorClass::NoneBehavior;
  msg.newBehaviorExecType = nullptr != newBehavior ? newBehavior->GetExecutableType() : ExecutableBehaviorType::NoneBehavior;
  msg.newBehaviorDisplayKey = newBehavior ? newBehavior->GetDisplayNameKey() : "";
  _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::BehaviorTransition>(msg);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::SwitchToReactionTrigger(IReactionTriggerStrategy& triggerStrategy, IBehavior* nextBehavior)
{
  // a null here means "no reaction", not "switch to the null behavior"
  if( nullptr == nextBehavior ) {
    return false;
  }
  
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
bool BehaviorManager::SwitchToVoiceCommandBehavior(IBehavior* nextBehavior)
{
  BehaviorRunningAndResumeInfo newBehaviorInfo;
  newBehaviorInfo.SetCurrentBehavior(nextBehavior);

  // if this is the first interruption, set the current scored behavior as the behavior to resume
  if(nullptr == GetRunningAndResumeInfo().GetBehaviorToResume() &&
     nullptr != GetRunningAndResumeInfo().GetCurrentBehavior())
  {
    newBehaviorInfo.SetBehaviorToResume(GetRunningAndResumeInfo().GetCurrentBehavior());
  }
  
  // if this is after the first, pass the behavior to resume along
  if(nullptr != GetRunningAndResumeInfo().GetBehaviorToResume())
  {
    newBehaviorInfo.SetBehaviorToResume(GetRunningAndResumeInfo().GetBehaviorToResume());
  }
  
  return SwitchToBehaviorBase(newBehaviorInfo);
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
 
  // ask the current chooser for the next behavior
  IBehavior* nextBehavior = _currentChooserPtr->ChooseNextBehavior(_robot, GetRunningAndResumeInfo().GetCurrentBehavior());
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
  
  if(AreDefaultHeadAndLiftStateSet() &&
     (_robot.GetActionList().GetNumQueues() > 0 && _robot.GetActionList().GetQueueLength(0) == 0))
  {
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
    
    IBehavior* behaviorToResume = GetRunningAndResumeInfo().GetBehaviorToResume();
    const Result resumeResult = behaviorToResume->Resume(resumingFromTrigger);
    if( resumeResult == RESULT_OK )
    {
      PRINT_CH_INFO("Behaviors", "BehaviorManager.ResumeBehavior", "Successfully resumed '%s'",
        behaviorToResume->GetName().c_str());
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
        behaviorToResume->GetName().c_str() );
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
    
  _currentChooserPtr->Update(robot);
  _voiceCommandChooser->Update(robot);

  UpdateTappedObject();
  
  // Identify whether there is a voice command-response behavior we want to be running
  IBehavior* voiceCommandBehavior = _voiceCommandChooser->ChooseNextBehavior(robot, GetRunningAndResumeInfo().GetCurrentBehavior());
  
  if (!voiceCommandBehavior)
  {
    // Update the current behavior if a new object was double tapped
    if(_needToHandleObjectTapped)
    {
      UpdateBehaviorWithObjectTapInteraction();
      _needToHandleObjectTapped = false;
    }
    
    const bool currentBehaviorIsReactionary =
                  (GetRunningAndResumeInfo().GetCurrentReactionTrigger() !=
                   ReactionTrigger::NoneTrigger);
    if(!currentBehaviorIsReactionary)
    {
      ChooseNextScoredBehaviorAndSwitch();
    }
  }
  
  // Allow reactionary behaviors to request a switch without a message
  const bool switchedFromReactionTrigger = CheckReactionTriggerStrategies();
  
  // Reaction triggers we just switched to take priority over commands
  if (!switchedFromReactionTrigger && voiceCommandBehavior)
  {
    // TODO: Note this won't work with switching between multiple behaviors that are all part of the same response.
    // The interface to the voice command behavior chooser will need to be updated so that the behavior manager knows whether
    // this new behavior is a brand new voice command response that needs to be 'switched' to, or simply a different behavior
    // in a line of behavior responses that can be used in the normal way (ie switched to using logic below).
    // TODO: figure out if the above TODO's assumptions are true...
    if (GetRunningAndResumeInfo().GetCurrentBehavior() != voiceCommandBehavior)
    {
      SwitchToVoiceCommandBehavior(voiceCommandBehavior);
    }
  }
  
  IBehavior* activeBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
  if (activeBehavior !=  nullptr) {
    
    const bool shouldAttemptResume =
      GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger ||
      voiceCommandBehavior;
    
    // We have a current behavior, update it.
    const IBehavior::Status status = activeBehavior->Update();
     
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
                          activeBehavior->GetName().c_str());
        if (shouldAttemptResume) {
          TryToResumeBehavior();
        }
        else {
          BehaviorRunningAndResumeInfo nullInfo;
          SwitchToBehaviorBase(nullInfo);
        }
        break;
          
      case IBehavior::Status::Failure:
        PRINT_NAMED_ERROR("BehaviorManager.Update.FailedUpdate",
                          "Behavior '%s' failed to Update().",
                          activeBehavior->GetName().c_str());
        // same as the Complete case
        if(shouldAttemptResume) {
          TryToResumeBehavior();
        }
        else {
          BehaviorRunningAndResumeInfo nullInfo;
          SwitchToBehaviorBase(nullInfo);
        }
        break;
    } // switch(status)
  }
    
  return lastResult;
} // Update()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::SetBehaviorChooser(IBehaviorChooser* newChooser)
{
  if( _currentChooserPtr == newChooser) {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.SetBehaviorChooser",
      "BehaviorChooser '%s' already set",
      newChooser ? newChooser->GetName() : "null");
    return;
  }
  
  // notify all chooser it's no longer selected
  if ( _currentChooserPtr ) {
    _currentChooserPtr->OnDeselected();
  }
  
  // default head and lift states should not be preserved between choosers
  DEV_ASSERT(!AreDefaultHeadAndLiftStateSet(),
             "BehaviorManager.ChooseNextBehaviorAndSwitch.DefaultHeadAndLiftStatesStillSet");

  const bool currentIsReactionary = false;
         //_currentBehaviorInfo->currentBehaviorCategory == BehaviorCategory::Reactionary;
  
  // The behavior pointers may no longer be valid, so clear them
  if(!currentIsReactionary){
    BehaviorRunningAndResumeInfo nullInfo;
    SwitchToBehaviorBase(nullInfo);
  }
  
  GetRunningAndResumeInfo().SetBehaviorToResume(nullptr);

  // channeled log and event
  LOG_EVENT("BehaviorManager.SetBehaviorChooser", "Switching behavior chooser from '%s' to '%s'",
            _currentChooserPtr ? _currentChooserPtr->GetName() : "null",
            newChooser->GetName());
  
  PRINT_CH_INFO("Behaviors",
                "BehaviorManager.SetBehaviorChooser",
                "Switching behavior chooser from '%s' to '%s'",
                _currentChooserPtr ? _currentChooserPtr->GetName() : "null",
                newChooser->GetName());

  _currentChooserPtr = newChooser;
  _currentChooserPtr->OnSelected();
  
  // mark the time at which the change happened (this is checked by behaviors)
  _lastChooserSwitchTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // force the new behavior chooser to select something now, instead of waiting for the next tick
  if(!currentIsReactionary){
    ChooseNextScoredBehaviorAndSwitch();
  }
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
  IBehavior* currentBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
  
  if ( nullptr != currentBehavior && currentBehavior->IsRunning() ) {
    currentBehavior->Stop();
  }
  
  GetRunningAndResumeInfo().SetCurrentBehavior(nullptr);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorManager::CheckReactionTriggerStrategies()
{
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
      IBehavior& rBehavior               =  *entry.second;
      
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
         strategy.ShouldTriggerBehavior(_robot, &rBehavior)){
          
          _robot.AbortAll();
          
          if(_robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::ALL_TRACKS) &&
             !_robot.GetMoveComponent().IsDirectDriving())
          {
            PRINT_NAMED_WARNING("BehaviorManager.CheckReactionTriggerStrategies",
                                "Some tracks are locked, unlocking them");
            _robot.GetMoveComponent().CompletelyUnlockAllTracks();
          }
        
          const bool successfulSwitch = SwitchToReactionTrigger(strategy, &rBehavior);
  
          didSuccessfullySwitch |= successfulSwitch;

          if(successfulSwitch){
            PRINT_CH_INFO("ReactionTriggers", "BehaviorManager.CheckReactionTriggerStrategies.SwitchingToReaction",
                          "Trigger strategy %s triggering behavior %s",
                          strategy.GetName().c_str(),
                          rBehavior.GetName().c_str());
          }else{
            PRINT_CH_INFO("ReactionTriggers", "BehaviorManager.CheckReactionTriggerStrategies.FailedToSwitch",
                          "Trigger strategy %s tried to trigger behavior %s, but init failed",
                          strategy.GetName().c_str(),
                          rBehavior.GetName().c_str());
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
    // Available games
    case ExternalInterface::BehaviorManagerMessageUnionTag::SetAvailableGames:
    {
      const auto& msg = message.Get_SetAvailableGames();
      SetAvailableGame(msg.availableGames);
      break;
    }
    
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
      
    case ExternalInterface::BehaviorManagerMessageUnionTag::ActivateSparkedMusic:
    {
      const auto& msg = message.Get_ActivateSparkedMusic();
      if ( !_audioClient->ActivateSparkedMusic(msg.behaviorSpark, msg.musicSate, msg.sparkedMusicState) ) {
        PRINT_NAMED_ERROR("BehaviorManager.HandleMessage.ActivateSparkedMusic.Failed",
                          "UnlockId %s", EnumToString(msg.behaviorSpark));
      }
      break;
    }
      
    case ExternalInterface::BehaviorManagerMessageUnionTag::DeactivateSparkedMusic:
    {
      const auto& msg = message.Get_DeactivateSparkedMusic();
      if ( !_audioClient->DeactivateSparkedMusic(msg.behaviorSpark, msg.musicSate) ) {
        PRINT_NAMED_ERROR("BehaviorManager.HandleMessage.DeactivateSparkedMusic.Failed",
                          "UnlockId %s", EnumToString(msg.behaviorSpark));
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
void BehaviorManager::CalculateFreeplayGoalFromObjects()
{
  // design: the choosers are generic, but this only makes sense if the freeplay chooser is an AIGoalEvaluator. I
  // think that we should not be able to data-drive it, but there may be a case for it, and this method
  // should just not work in that case. Right now I feel like using dynamic_cast, but we could provide default
  // implementation in chooser interface that asserts to not be used.
  AIGoalEvaluator* freeplayGoalEvaluator = dynamic_cast<AIGoalEvaluator*>(_freeplayChooser);
  if ( nullptr != freeplayGoalEvaluator )
  {
    freeplayGoalEvaluator->CalculateDesiredGoalFromObjects();
  }
  else
  {
    PRINT_NAMED_ERROR("BehaviorManager.CalculateFreeplayGoalFromObjects.NotAGoalEvaluator",
      "The current freeplay chooser is not a goal evaluator.");
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
  /// Iterate over all reaction triggers to see if they're affected by this request
  for(auto& entry: _reactionTriggerMap){
    ReactionTrigger triggerEnum = entry.first;
    auto& allStrategyMaps = entry.second;
    DEV_ASSERT_MSG(!allStrategyMaps.IsTriggerLockedByID(lockID),
          "BehaviorManager.RequestDisableReactions.LockAlreadyInUse",
          "Attempted to disable reactions with ID %s which is already in use",
          lockID.c_str());
    
    if(ReactionTriggerHelpers::IsTriggerAffected(triggerEnum, triggersAffected)){
      PRINT_CH_INFO("ReactionTriggers",
                    "BehaviorManager.RequestDisableReactions.AllTriggersConsidered",
                    "Trigger %s is being disabled by %s",
                    EnumToString(triggerEnum),
                    lockID.c_str());
      

      if(allStrategyMaps.IsReactionEnabled()){
        //About to disable, notify reaction trigger strategy
        const auto& allEntries = allStrategyMaps.GetStrategyMap();
        for(auto& entry : allEntries){
          entry.first->EnabledStateChanged(false);
        }
      }
      
      allStrategyMaps.AddDisableLockToTrigger(lockID, triggerEnum);
      
      // If the currently running behavior was triggered as a reaction
      // and we are supposed to stop current, stop the current behavior
      if(stopCurrent &&
         _runningAndResumeInfo->GetCurrentReactionTrigger() == triggerEnum){
        IBehavior* currentBehavior = _runningAndResumeInfo->GetCurrentBehavior();
        if(currentBehavior!= nullptr && currentBehavior->IsRunning()){
          currentBehavior->Stop();
        }
      }
      
    }
    triggerEnum++;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::RemoveDisableReactionsLock(const std::string& lockID)
{
  /// Iterate over all reaction triggers to see if they're affected by this request
  for(auto& entry: _reactionTriggerMap){
    ReactionTrigger triggerEnum = entry.first;
    auto& allStrategyMaps = entry.second;
    
    PRINT_CH_INFO("ReactionTriggers",
                  "BehaviorManager.RequestReEnableReactions.AllTriggersConsidered",
                  "Trigger %s is being enabled by %s",
                  EnumToString(triggerEnum),
                  lockID.c_str());

    if(allStrategyMaps.IsTriggerLockedByID(lockID)){
      allStrategyMaps.RemoveDisableLockFromTrigger(lockID, triggerEnum);
      
      if(allStrategyMaps.IsReactionEnabled()){
        //About to disable, notify reaction trigger strategy
        const auto& allEntries = allStrategyMaps.GetStrategyMap();
        for(auto& entry : allEntries){
          entry.first->EnabledStateChanged(true);
        }
      }
    }
    triggerEnum++;
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
void BehaviorManager::HandleObjectTapInteraction(const ObjectID& objectID)
{
  // Have to be in freeplay and not picking or placing and flat on the ground
  if(_currentChooserPtr != _freeplayChooser ||
     _robot.IsPickingOrPlacing() ||
     _robot.GetOffTreadsState() != OffTreadsState::OnTreads)
  {
    PRINT_CH_INFO("Behaviors", "HandleObjectTapInteraction.CantRun",
                  "Robot is not in freeplay chooser or is picking and placing or not on treads");
    return;
  }

  if(_activeSpark != UnlockId::Count)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.InSpark",
                  "Currently in spark not switching to double tap");
    return;
  }
  
  // We can only interrupt the react to double tap reactionary behavior
  const bool activeBehaviorIsReactionary = GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger;
  if(activeBehaviorIsReactionary &&
      GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::DoubleTapDetected)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.InReaction",
                  "Currently in reaction not switching to double tap");
    return;
  }

  auto doubleTapIter = _reactionTriggerMap.find(ReactionTrigger::DoubleTapDetected);
  if(doubleTapIter == _reactionTriggerMap.end()){
    DEV_ASSERT(false, "BehaviorManager.HandleObjectTapInteraction.FaildToFindDoubleTap");
    return;
  }
  
  const bool isDoubleTapEnabled = doubleTapIter->second.IsReactionEnabled();
  
  if(!isDoubleTapEnabled) {
    PRINT_CH_INFO("Behavior", "BehaviorManager.HandleObjectTapInteraction.Disabled",
                  "Object tap interaction disabled, ignoring tap");
    return;
  }
  
  _needToHandleObjectTapped = true;
  _pendingDoubleTappedObject = objectID;
  
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::LeaveObjectTapInteraction()
{
  if(!_robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.LeaveObjectTapInteraction.NoTapIntent", "");
    return;
  }
  
  if(_currentChooserPtr != nullptr &&
    _currentChooserPtr->SupportsObjectTapInteractions())
  {
    PRINT_CH_INFO("Behaviors", "LeaveObjectTapInteration", "");
    
    auto* chooser = dynamic_cast<AIGoalEvaluator*>(_currentChooserPtr);
    
    // It is possible that we have requested the ObjectTapInteraction goal but it hasn't actually
    // been selected so we need to clear the requested goal
    if(chooser != nullptr)
    {
      chooser->ClearObjectTapInteractionRequestedGoal();
    }
    else
    {
      PRINT_NAMED_ERROR("BehaviorManager.LeaveObjectTapInteraction.NullChooser",
                        "Current chooser is not an AIGoalEvaluator but supports object tap interactions");
    }
  }
  
  _robot.GetAIComponent().GetWhiteboard().ClearObjectTapInteraction();
  
  _lastDoubleTappedObject.UnSet();
  _currDoubleTappedObject.UnSet();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::UpdateTappedObject()
{
  auto doubleTapIter = _reactionTriggerMap.find(ReactionTrigger::DoubleTapDetected);
  if(doubleTapIter == _reactionTriggerMap.end()){
    DEV_ASSERT(false, "BehaviorManager.UpdateTappedObject.FaildToFindDoubleTap");
    return;
  }
  
  const bool isDoubleTapEnabled = doubleTapIter->second.IsReactionEnabled();
  
  // If we have a tap intent and are not currently in ReactToDoubleTap
  if(_robot.GetAIComponent().GetWhiteboard().HasTapIntent() &&
     GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::DoubleTapDetected &&
     isDoubleTapEnabled)
  {
    // If the tapped objects pose becomes unknown then give up and leave object tap interaction
    // (we expect the pose to be unknown/dirty when ReactToDoubleTap is running)
    const ObservableObject* object = _robot.GetBlockWorld().GetLocatedObjectByID(_currDoubleTappedObject);
    if(object != nullptr)
    {
      LeaveObjectTapInteraction();
    }
    // Otherwise if there is no behavior currently running but we have a tap interaction
    // (One of the tap behaviors probably just ended because it realized it couldn't use the tapped object)
    else if(_runningAndResumeInfo->GetCurrentBehavior() == nullptr)
    {
      bool canAnyTapBehaviorUseObject = false;
      
      // For each tapInteraction behavior check if its ObjectUseIntention(s) can still use the tapped object
      const ObjectID& tappedObject = GetCurrTappedObject();
      for(const auto& behavior : _tapInteractionBehaviors)
      {
        const auto& intentions = behavior->GetBehaviorObjectInteractionIntentions();
        for(const auto& intent : intentions)
        {
          auto& objInfoCache = _robot.GetAIComponent().GetObjectInteractionInfoCache();
          if(objInfoCache.IsObjectValidForInteraction(intent, tappedObject))
          {
            canAnyTapBehaviorUseObject = true;
            break;
          }
        }
      }
      
      // If no tap interation behavior can currently use the tapped object then leave object tap interaction
      if(!canAnyTapBehaviorUseObject)
      {
        PRINT_CH_INFO("Behaviors", "BehaviorManager.UpdateTappedObject.NoDoubleTapBehaviorCanUseObject",
                      "Tapped object is no longer valid for any double tap behavior, leaving tap interaction");
        LeaveObjectTapInteraction();
      }
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorManager::UpdateBehaviorWithObjectTapInteraction()
{
  // Copy pending object to be assigned to current double tapped object
  const ObjectID objectID = _pendingDoubleTappedObject;

  if(!_currentChooserPtr->SupportsObjectTapInteractions())
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.SupportTapInteraction",
                  "Current chooser does not support object tap interactions");
    return;
  }
  
  auto* chooser = dynamic_cast<AIGoalEvaluator*>(_currentChooserPtr);
  
  if(chooser == nullptr)
  {
    PRINT_NAMED_ERROR("BehaviorManager.HandleObjectTapInteraction.NullChooser",
                      "Freeplay chooser is not an AIGoalEvaluator or is null");
    return;
  }
  
  // Tell whiteboard we have a object with tap intention
  _robot.GetAIComponent().GetWhiteboard().SetObjectTapInteraction(objectID);
  
  if(_currDoubleTappedObject == objectID)
  {
    PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.CurrTappedObject",
                  "Tapped object %u is the current tapped object not doing anything",
                  objectID.GetValue());
    return;
  }
  
  // If the current goal is not the ObjectTapInteraction goal
  // then update the behavior chooser and request the ObjectTapInteraction goal
  if(!chooser->IsCurrentGoalObjectTapInteraction())
  {
    // Switch to ObjectTapInteraction goal
    chooser->SwitchToObjectTapInteractionGoal();
    
    _lastDoubleTappedObject = _currDoubleTappedObject;
    _currDoubleTappedObject = objectID;
  }
  // Otherwise we are already in the ObjectTapInteraction goal
  else
  {
    bool currentBehaviorIsNoneBehavior = GetRunningAndResumeInfo().GetCurrentBehavior() == nullptr ||
             GetRunningAndResumeInfo().GetCurrentBehavior()->GetClass() == BehaviorClass::NoneBehavior;
    
    IBehavior* activeBehavior = GetRunningAndResumeInfo().GetCurrentBehavior();
    // The current behavior of the goal is not NoneBehavior
    if(nullptr != activeBehavior && !currentBehaviorIsNoneBehavior)
    {
      // Update the interaction object
      _lastDoubleTappedObject = _currDoubleTappedObject;
      _currDoubleTappedObject = objectID;
      
      // If the current behavior can't handle the new tapped object
      if(!activeBehavior->HandleNewDoubleTap(_robot))
      {
        PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.BehaviorNotRunnable",
                      "%s is no longer runnable after updating target blocks",
                      activeBehavior->GetName().c_str());
        
        // If the current behavior is a reactionary behavior (ReactToDoubleTap) then
        // make sure to update _runningReactionaryBehavior and try to resume the last behavior
        if(GetRunningAndResumeInfo().GetCurrentReactionTrigger() != ReactionTrigger::NoneTrigger)
        {
          // The only possible reactionary behavior that can be interrupted with a double tap is
          // ReactToDouble

          DEV_ASSERT(GetRunningAndResumeInfo().GetCurrentReactionTrigger() == ReactionTrigger::DoubleTapDetected,
                     "Current reactionary behavior should only be ReactToDoubleTap");

          TryToResumeBehavior();
          
          // The behavior to resume becomes the currentBehavior so we need to stop, updateTargetBlocks,
          // and re-init it to make sure the resumed behavior has correct target blocks and the cube lights get
          // updated
          // If we are resuming NoneBehavior then just quit object tap interaction
          if(!currentBehaviorIsNoneBehavior)
          {
            if(!activeBehavior->HandleNewDoubleTap(_robot))
            {
              PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.ResumeFailed",
                            "%s can't run after being resumed",
                            activeBehavior->GetName().c_str());
              LeaveObjectTapInteraction();
            }
          }
          else
          {
            PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.ResumingNoneBehavior",
                          "Was in ReactToDoubleTap, got another double tap and am now trying to resume NoneBehavior");
            LeaveObjectTapInteraction();
          }
          return;
        }
        
        // If, for some reason, the object tap interaction behavior that was running is no longer runnable
        // then we should start the ReactToDoubleTap behavior to try to find the tapped object.
        // This is necessary because otherwise the ObjectTapInteraction goal will exit due to not being able
        // to pick a behavior (all of the ObjectTapInteraction behaviors have similar requirements to run) so
        // if the current one can't run then the others probably won't be able to run either.
        
        
        auto doubleTapIter = _reactionTriggerMap.find(ReactionTrigger::DoubleTapDetected);
        if(doubleTapIter == _reactionTriggerMap.end()){
          DEV_ASSERT(false, "BehaviorManager.DoubleTap.FaildToFindDoubleTap");
          return;
        }
        
        const auto& doubleTapEntry = doubleTapIter->second;
        DEV_ASSERT(doubleTapEntry.GetStrategyMap().size() == 1,
                   "BehaviorManager.DoubleTap.MultipleDoubleTapEntriesInMap");

        if(doubleTapEntry.GetStrategyMap().size() >= 1){
          
          
          IBehavior* reactToDoubleTap = doubleTapEntry.GetStrategyMap().begin()->second;
          IReactionTriggerStrategy& strategy = *doubleTapEntry.GetStrategyMap().begin()->first;
          
          const bool isReactionEnabled = doubleTapEntry.IsReactionEnabled();
          const bool shouldTrigger = strategy.ShouldTriggerBehavior(_robot, reactToDoubleTap);

          BehaviorPreReqRobot preReqRobot(_robot);
          if(isReactionEnabled && shouldTrigger)
          {
            PRINT_CH_INFO("Behaviors", "BehaviorManager.HandleObjectTapInteraction.StartingReactToDoubleTap",
                          "Forcing ReactToDoubleTap to run because %s can't run with newly double tapped object",
                          activeBehavior->GetName().c_str());
            
            
            BehaviorRunningAndResumeInfo reactionaryInfo;
            reactionaryInfo.SetCurrentBehavior(reactToDoubleTap);
            reactToDoubleTap->Init();
            SetRunningAndResumeInfo(reactionaryInfo);
            return;
          }
        }
        
        // This will cause the current tapInteraction behavior to end and a new one to be picked
        BehaviorRunningAndResumeInfo nullInfo;
        SetRunningAndResumeInfo(nullInfo);
        return;
      }
    }
  }
}

void BehaviorManager::OnRobotDelocalized()
{
  // If the robot delocalizes and we have a tapped object immediately stop the double tapped lights
  // Normally the lights would be stopped when the tapInteraction goal exits but that is too late
  // Eg. Pick Cozmo up after double tapping a cube, the lights will stay on until he is put down and the
  // tapInteraction goal is kicked out. Instead of having the lights stop when he is put down, they should
  // stop when he is picked up (delocalizes)
  // Can't do this on Stop() because tapInteraction behaviors are stopped and started as different objects get tapped
  if(_robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DoubleTappedKnown);
    _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(CubeAnimationTrigger::DoubleTappedUnsure);
  }
}

  
} // namespace Cozmo
} // namespace Anki
