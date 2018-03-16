/**
 * File: iBehavior.cpp
 *
 * Author: Andrew Stein : Kevin M. Karol
 * Date:   7/30/15  : 12/1/16
 *
 * Description: Defines interface for a Cozmo "Behavior"
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "engine/behaviorSystem/behaviors/iBehavior.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/actions/actionInterface.h"
#include "engine/actions/dockActions.h"
#include "engine/actions/driveToActions.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/severeNeedsComponent.h"
#include "engine/aiComponent/behaviorHelperComponent.h"
#include "engine/audio/behaviorAudioClient.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/wantsToRunStrategies/wantsToRunStrategyFactory.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/carryingComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/components/progressionUnlockComponent.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"


#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/enums/stringToEnumMapper.hpp"
#include "util/fileUtils/fileUtils.h"
#include "util/math/numericCast.h"
#include "engine/components/pathComponent.h"


namespace Anki {
namespace Cozmo {

namespace {
static const char* kBehaviorClassKey                 = "behaviorClass";
static const char* kBehaviorIDConfigKey              = "behaviorID";
static const char* kNeedsActionIDKey                 = "needsActionID";
static const char* kDisplayNameKey                   = "displayNameKey";

static const char* kRequiredUnlockKey                = "requiredUnlockId";
static const char* kRequiredSevereNeedsStateKey      = "requiredSevereNeedsState";
static const char* kRequiredDriveOffChargerKey       = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey          = "requiredRecentSwitchToParent_sec";
static const char* kExecutableBehaviorTypeKey        = "executableBehaviorType";
static const char* kSparkedBehaviorDisableLock       = "SparkBehaviorDisables";
static const char* kSmartReactionLockSuffix          = "_behaviorLock";
static const char* kAlwaysStreamlineKey              = "alwaysStreamline";
static const char* kWantsToRunStrategyConfigKey      = "wantsToRunStrategyConfig";
static const std::string kIdleLockPrefix             = "Behavior_";


static const int kMaxResumesFromCliff                = 2;
static const float kCooldownFromCliffResumes_sec     = 15.0;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kObjectTapInteractionDisablesArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    true},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        false},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kObjectTapInteractionDisablesArray),
              "Reaction triggers duplicate or non-sequential");
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr ReactionTriggerHelpers::FullReactionArray kSparkBehaviorDisablesArray = {
  {ReactionTrigger::CliffDetected,                false},
  {ReactionTrigger::CubeMoved,                    false},
  {ReactionTrigger::FacePositionUpdated,          false},
  {ReactionTrigger::FistBump,                     false},
  {ReactionTrigger::Frustration,                  false},
  {ReactionTrigger::Hiccup,                       false},
  {ReactionTrigger::MotorCalibration,             false},
  {ReactionTrigger::NoPreDockPoses,               false},
  {ReactionTrigger::ObjectPositionUpdated,        true},
  {ReactionTrigger::PlacedOnCharger,              false},
  {ReactionTrigger::PetInitialDetection,          false},
  {ReactionTrigger::RobotFalling,                 false},
  {ReactionTrigger::RobotPickedUp,                false},
  {ReactionTrigger::RobotPlacedOnSlope,           false},
  {ReactionTrigger::ReturnedToTreads,             false},
  {ReactionTrigger::RobotOnBack,                  false},
  {ReactionTrigger::RobotOnFace,                  false},
  {ReactionTrigger::RobotOnSide,                  false},
  {ReactionTrigger::RobotShaken,                  false},
  {ReactionTrigger::Sparked,                      false},
  {ReactionTrigger::UnexpectedMovement,           false},
};

static_assert(ReactionTriggerHelpers::IsSequentialArray(kSparkBehaviorDisablesArray),
              "Reaction triggers duplicate or non-sequential");
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Json::Value IBehavior::CreateDefaultBehaviorConfig(BehaviorClass behaviorClass, BehaviorID behaviorID)
{
  Json::Value config;
  config[kBehaviorClassKey] = BehaviorClassToString(behaviorClass);
  config[kBehaviorIDConfigKey] = BehaviorIDToString(behaviorID);
  return config;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorID IBehavior::ExtractBehaviorIDFromConfig(const Json::Value& config,
                                                  const std::string& fileName)
{
  const std::string debugName = "IBeh.NoBehaviorIdSpecified";
  const std::string behaviorID_str = JsonTools::ParseString(config, kBehaviorIDConfigKey, debugName);
  
  // To make it easy to find behaviors, assert that the file name and behaviorID match
  if(ANKI_DEV_CHEATS && !fileName.empty()){
    std::string jsonFileName = Util::FileUtils::GetFileName(fileName);
    auto dotIndex = jsonFileName.find_last_of(".");
    std::string lowerFileName = dotIndex == std::string::npos ? jsonFileName : jsonFileName.substr(0, dotIndex);
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
    
    std::string behaviorIDLower = behaviorID_str;
    std::transform(behaviorIDLower.begin(), behaviorIDLower.end(), behaviorIDLower.begin(), ::tolower);
    DEV_ASSERT_MSG(behaviorIDLower == lowerFileName,
                   "RobotDataLoader.LoadBehaviors.BehaviorIDFileNameMismatch",
                   "File name %s does not match BehaviorID %s",
                   fileName.c_str(),
                   behaviorID_str.c_str());
  }
  
  return BehaviorIDFromString(behaviorID_str);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass IBehavior::ExtractBehaviorClassFromConfig(const Json::Value& config)
{
  const Json::Value& behaviorTypeJson = config[kBehaviorClassKey];
  const char* behaviorTypeString = behaviorTypeJson.isString() ? behaviorTypeJson.asCString() : "";
  return BehaviorClassFromString(behaviorTypeString);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeedsActionId IBehavior::ExtractNeedsActionIDFromConfig(const Json::Value& config)
{
  const Json::Value& needsActionIDJson = config[kNeedsActionIDKey];
  const char* needsActionIDString = needsActionIDJson.isString() ? needsActionIDJson.asCString() : "";
  if (!needsActionIDString[0])
  {
    return NeedsActionId::NoAction;
  }
  return NeedsActionIdFromString(needsActionIDString);
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::IBehavior(Robot& robot, const Json::Value& config)
: IBSRunnable(BehaviorIDToString(ExtractBehaviorIDFromConfig(config)))
, _requiredProcess( AIInformationAnalysis::EProcess::Invalid )
, _robot(robot)
, _lastRunTime_s(0.0f)
, _startedRunningTime_s(0.0f)
, _wantsToRunStrategy(nullptr)
, _id(ExtractBehaviorIDFromConfig(config))
, _idString(BehaviorIDToString(_id))
, _behaviorClassID(ExtractBehaviorClassFromConfig(config))
, _needsActionID(ExtractNeedsActionIDFromConfig(config))
, _executableType(ExecutableBehaviorType::Count)
, _requiredUnlockId( UnlockId::Count )
, _requiredSevereNeed( NeedId::Count )
, _requiredRecentDriveOffCharger_sec(-1.0f)
, _requiredRecentSwitchToParent_sec(-1.0f)
, _isRunning(false)
, _isResuming(false)
, _hasSetIdle(false)
{
  if(!ReadFromJson(config)){
    PRINT_NAMED_WARNING("IBehavior.ReadFromJson.Failed",
                        "Something went wrong reading from JSON");
  }

  if(_robot.HasExternalInterface()) {
    // NOTE: this won't get sent down to derived classes (unless they also subscribe)
    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                              EngineToGameTag::RobotCompletedAction,
                              [this](const EngineToGameEvent& event) {
                                DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                                           "IBehavior.RobotCompletedAction.WrongEventTypeFromCallback");
                                HandleActionComplete(event.GetData().Get_RobotCompletedAction());
                              } ));

    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                              EngineToGameTag::BehaviorObjectiveAchieved,
                              [this](const EngineToGameEvent& event) {
                                DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::BehaviorObjectiveAchieved,
                                           "IBehavior.BehaviorObjectiveAchieved.WrongEventTypeFromCallback");
                                HandleBehaviorObjective(event.GetData().Get_BehaviorObjectiveAchieved());
                              } ));
  }
  ScoredConstructor(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::ReadFromJson(const Json::Value& config)
{
  JsonTools::GetValueOptional(config, kDisplayNameKey, _displayNameKey);

  // - - - - - - - - - -
  // Required unlock
  // - - - - - - - - - -
  const Json::Value& requiredUnlockJson = config[kRequiredUnlockKey];
  if ( !requiredUnlockJson.isNull() )
  {
    DEV_ASSERT(requiredUnlockJson.isString(), "IBehavior.ReadFromJson.NonStringUnlockId");
    
    // this is probably the only place where we need this, otherwise please refactor to proper header
    const UnlockId requiredUnlock = UnlockIdFromString(requiredUnlockJson.asString());
    if ( requiredUnlock != UnlockId::Count ) {
      PRINT_NAMED_INFO("IBehavior.ReadFromJson.RequiredUnlock", "Behavior '%s' requires unlock '%s'",
                        GetIDStr().c_str(), requiredUnlockJson.asString().c_str() );
      _requiredUnlockId = requiredUnlock;
    } else {
      PRINT_NAMED_ERROR("IBehavior.ReadFromJson.InvalidUnlockId", "Could not convert string to unlock id '%s'",
        requiredUnlockJson.asString().c_str());
    }
  }

  // - - - - - - - - - - - - - -
  // Required severe needs state
  // - - - - - - - - - - - - - -
  const Json::Value& requiredSevereNeedJson = config[kRequiredSevereNeedsStateKey];
  if( !requiredSevereNeedJson.isNull() ) {
    DEV_ASSERT(requiredSevereNeedJson.isString(), "IBehavior.ReadFromJson.NonStringRequiredNeedsState");

    _requiredSevereNeed = NeedIdFromString(requiredSevereNeedJson.asString());
    ANKI_VERIFY(_requiredSevereNeed != NeedId::Count,
                "IBehavior.ReadFromJson.ConfigError.InvalidRequiredSevereNeed",
                "%s: Required severe need '%s' converted to Count. This field is optional",
                GetIDStr().c_str(),
                requiredSevereNeedJson.asCString());
  }
  
  // - - - - - - - - - -
  // Got off charger timer
  const Json::Value& requiredDriveOffChargerJson = config[kRequiredDriveOffChargerKey];
  if (!requiredDriveOffChargerJson.isNull())
  {
    DEV_ASSERT_MSG(requiredDriveOffChargerJson.isNumeric(), "IBehavior.ReadFromJson", "Not a float: %s",
                   kRequiredDriveOffChargerKey);
    _requiredRecentDriveOffCharger_sec = requiredDriveOffChargerJson.asFloat();
  }
  
  // - - - - - - - - - -
  // Required recent parent switch
  const Json::Value& requiredSwitchToParentJson = config[kRequiredParentSwitchKey];
  if (!requiredSwitchToParentJson.isNull()) {
    DEV_ASSERT_MSG(requiredSwitchToParentJson.isNumeric(), "IBehavior.ReadFromJson", "Not a float: %s",
                   kRequiredParentSwitchKey);
    _requiredRecentSwitchToParent_sec = requiredSwitchToParentJson.asFloat();
  }
    
  const Json::Value& executableBehaviorTypeJson = config[kExecutableBehaviorTypeKey];
  if (executableBehaviorTypeJson.isString())
  {
    _executableType = ExecutableBehaviorTypeFromString(executableBehaviorTypeJson.asCString());
  }
  
  JsonTools::GetValueOptional(config, kAlwaysStreamlineKey, _alwaysStreamline);
  
  if(config.isMember(kWantsToRunStrategyConfigKey)){
    const Json::Value& wantsToRunConfig = config[kWantsToRunStrategyConfigKey];
    _wantsToRunStrategy.reset(WantsToRunStrategyFactory::CreateWantsToRunStrategy(_robot, wantsToRunConfig));
  }
  
  // Doesn't actually read anything from behavior config, but sets defaults
  // for certain scoring metrics in case no score json is loaded
  return ReadFromScoredJson(config, false);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::~IBehavior()
{
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SubscribeToTags(std::set<GameToEngineTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const GameToEngineEvent& event) {
      HandleEvent(event);
    };
      
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SubscribeToTags(std::set<EngineToGameTag> &&tags)
{
  if(_robot.HasExternalInterface()) {
    auto handlerCallback = [this](const EngineToGameEvent& event) {
      HandleEvent(event);
    };
      
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SubscribeToTags(const uint32_t robotId, std::set<RobotInterface::RobotToEngineTag> &&tags)
{
  auto handlerCallback = [this](const RobotToEngineEvent& event) {
    HandleEvent(event);
  };
  
  for(auto tag : tags) {
    _eventHandles.push_back(_robot.GetRobotMessageHandler()->Subscribe(robotId, tag, handlerCallback));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IBehavior::Init()
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Init").c_str(), "Starting...");

  // Check if there are any engine-generated actions in the action list, because there shouldn't be!
  // If there is, a behavior probably didn't use StartActing() where it should have.
  bool engineActionStillRunning = false;
  for (auto listIt = _robot.GetActionList().begin();
       listIt != _robot.GetActionList().end() && !engineActionStillRunning;
       ++listIt) {
  
    const ActionQueue& q = listIt->second;
    
    // Start with current action list
    const IActionRunner* currRunningAction = q.GetCurrentRunningAction();
    if ((nullptr != currRunningAction) &&
        (currRunningAction->GetTag() >= ActionConstants::FIRST_ENGINE_TAG) &&
        (currRunningAction->GetTag() <= ActionConstants::LAST_ENGINE_TAG)) {
      engineActionStillRunning = true;
    }
    
    // Check actions in queue
    for (auto it = q.begin(); it != q.end() && !engineActionStillRunning; ++it) {
      u32 tag = (*it)->GetTag();
      if ((tag >= ActionConstants::FIRST_ENGINE_TAG) && (tag <= ActionConstants::LAST_ENGINE_TAG)) {
        engineActionStillRunning = true;
      }
    }
  }
  
  if (engineActionStillRunning) {
    PRINT_NAMED_WARNING("IBehavior.Init.ActionsInQueue",
                        "Initializing %s: %zu actions already in queue",
                        GetIDStr().c_str(), _robot.GetActionList().GetQueueLength(0));
  }

  // Streamline all IBehaviors when a spark is active
  if(_robot.GetBehaviorManager().GetActiveSpark() != UnlockId::Count
     && !_robot.GetBehaviorManager().IsActiveSparkSoft()){
    _sparksStreamline = true;
  } else {
    _sparksStreamline = false;
  }
  
  _isRunning = true;
  _stopRequestedAfterAction = false;
  _actingCallback = nullptr;
  _startedRunningTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _hasSetIdle = false;
  Result initResult = InitInternal(_robot);
  if ( initResult != RESULT_OK ) {
    _isRunning = false;
  }
  else {
    _startCount++;
  }
  
  // Disable Acknowledge object if this behavior is the sparked version
  if(_requiredUnlockId != UnlockId::Count
       && _requiredUnlockId == _robot.GetBehaviorManager().GetActiveSpark())
  {
    SmartDisableReactionsWithLock(kSparkedBehaviorDisableLock, kSparkBehaviorDisablesArray);
  }
  
  InitScored(_robot);
  
  return initResult;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IBehavior::Resume(ReactionTrigger resumingFromType)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Resume").c_str(), "Resuming...");
  DEV_ASSERT(!_isRunning, "IBehavior.Resume.ShouldNotBeRunningIfWeTryToResume");
  
  // Check and update the number of times we've resumed from cliff or unexpected movement
  if(resumingFromType == ReactionTrigger::CliffDetected
     || resumingFromType == ReactionTrigger::UnexpectedMovement)
  {
    _timesResumedFromPossibleInfiniteLoop++;
    if(_timesResumedFromPossibleInfiniteLoop >= kMaxResumesFromCliff){
      const float currTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      _timeCanRunAfterPossibleInfiniteLoopCooldown_sec = currTime + kCooldownFromCliffResumes_sec;
      _robot.GetMoodManager().TriggerEmotionEvent("TooManyResumesCliffOrMovement", currTime);
      return Result::RESULT_FAIL;
    }
  }
  
  _isResuming = true;
  _startedRunningTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  Result initResult = ResumeInternal(_robot);
  _isResuming = false;
  
  if ( initResult == RESULT_OK ) {
    // default implementation of ResumeInternal also sets it to true, but behaviors that override it
    // might not set it
    _isRunning = true;
    
    // Disable Acknowledge object if this behavior is the sparked version
    if(_requiredUnlockId != UnlockId::Count
       && _requiredUnlockId == _robot.GetBehaviorManager().GetActiveSpark()){
      SmartDisableReactionsWithLock(kSparkedBehaviorDisableLock, kSparkBehaviorDisablesArray);
    }
    
  } else {
    _isRunning = false;
  }
  
  return initResult;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IBehavior::Update()
{

  if(_stopRequestedAfterAction && !IsActing()) {
    // we've been asked to stop, don't bother ticking update
    return Status::Complete;
  }
  
  DEV_ASSERT(IsRunning(), "IBehavior::UpdateNotRunning");  
  return UpdateInternal(_robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::Stop()
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".Stop").c_str(), "Stopping...");

  // If the behavior delegated off to a helper, stop that first
  if(!_currentHelperHandle.expired()){
    StopHelperWithoutCallback();
  }
  
  _isRunning = false;
  StopInternal(_robot);
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StopActing(false);
  
  // Re-enable any reactionary behaviors which the behavior disabled and didn't have a chance to
  // re-enable before stopping
  while(!_smartLockIDs.empty()){
    SmartRemoveDisableReactionsLock(*_smartLockIDs.begin());
  }

  if(_hasSetIdle){
    SmartRemoveIdleAnimation(_robot);
  }
  
  // clear the path component motion profile if it was set by the behavior
  if( _hasSetMotionProfile ) {
    SmartClearMotionProfile();
  }

  // Unlock any tracks which the behavior hasn't had a chance to unlock
  for(const auto& entry: _lockingNameToTracksMap){
    _robot.GetMoveComponent().UnlockTracks(entry.second, entry.first);
  }
  
  
  _lockingNameToTracksMap.clear();
  _customLightObjects.clear();
  
  DEV_ASSERT(_smartLockIDs.empty(), "IBehavior.Stop.DisabledReactionsNotEmpty");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::StopOnNextActionComplete()
{
  if( !_stopRequestedAfterAction ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".StopOnNextActionComplete").c_str(),
                  "Behavior has been asked to stop on the next complete action");
  }
  
  // clear the callback and don't let any new actions start
  _stopRequestedAfterAction = true;
  _actingCallback = nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::IsRunnable(const Robot& robot) const
{
  if(IsRunnableBase(_robot)){
    return IsRunnableInternal(robot);
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::IsRunnableBase(const Robot& robot) const
{
  // Some reaction trigger strategies allow behaviors to interrupt themselves.
  DEV_ASSERT(!IsRunning(), "IBehavior.IsRunnableCalledOnRunningBehavior");
  if (IsRunning()) {
    PRINT_CH_DEBUG("Behaviors", "IBehavior.IsRunnableBase", "Behavior %s is already running", GetIDStr().c_str());
    return true;
  }
  
  // check if required processes are running
  if ( _requiredProcess != AIInformationAnalysis::EProcess::Invalid )
  {
    const bool isProcessOn = robot.GetAIComponent().GetAIInformationAnalyzer().IsProcessRunning(_requiredProcess);
    if ( !isProcessOn ) {
      PRINT_NAMED_ERROR("IBehavior.IsRunnable.RequiredProcessNotFound",
        "Required process '%s' is not enabled for '%s'",
        AIInformationAnalysis::StringFromEProcess(_requiredProcess),
        GetIDStr().c_str());
      return false;
    }
  }

  // check if a severe needs state is required
  if( _requiredSevereNeed != NeedId::Count &&
      _requiredSevereNeed != robot.GetAIComponent().GetSevereNeedsComponent().GetSevereNeedExpression() ) {
    // not in the correct state
    return false;
  }

  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // first check the unlock
  if ( _requiredUnlockId != UnlockId::Count )
  {
    // ask progression component if the unlockId is currently unlocked
    const ProgressionUnlockComponent& progressionComp = robot.GetProgressionUnlockComponent();
    const bool forFreeplay = true;
    const bool isUnlocked = progressionComp.IsUnlocked( _requiredUnlockId, forFreeplay );
    if ( !isUnlocked ) {
      return false;
    }
  }
  
  // if there's a timer requiring a recent drive off the charger, check with whiteboard
  const bool requiresRecentDriveOff = FLT_GE(_requiredRecentDriveOffCharger_sec, 0.0f);
  if ( requiresRecentDriveOff )
  {
    const float lastDriveOff = robot.GetAIComponent().GetWhiteboard().GetTimeAtWhichRobotGotOffCharger();
    const bool hasDrivenOff = FLT_GE(lastDriveOff, 0.0f);
    if ( !hasDrivenOff ) {
      // never driven off the charger, can't run
      return false;
    }
    
    const bool isRecent = FLT_LE(curTime, (lastDriveOff + _requiredRecentDriveOffCharger_sec));
    if ( !isRecent ) {
      // driven off, but not recently enough
      return false;
    }
  }
  
  // if there's a timer requiring a recent parent switch
  const bool requiresRecentParentSwitch = FLT_GE(_requiredRecentSwitchToParent_sec, 0.0);
  if ( requiresRecentParentSwitch ) {
    const float lastTime = robot.GetBehaviorManager().GetLastBehaviorChooserSwitchTime();
    const float changedAgoSecs = curTime - lastTime;
    const bool isSwitchRecent = FLT_LE(changedAgoSecs, _requiredRecentSwitchToParent_sec);
    if ( !isSwitchRecent ) {
      return false;
    }
  }
  
  //check if the behavior runs while in the air
  if(robot.GetOffTreadsState() != OffTreadsState::OnTreads
     && !ShouldRunWhileOffTreads()){
    return false;
  }

  //check if the behavior can run from the charger platform (don't want most to run because they could damage
  //the robot by moving too much, and also will generally look dumb if they try to turn)
  if(robot.IsOnChargerPlatform() && !ShouldRunWhileOnCharger()) {
    return false;
  }
  
  //check if the behavior can handle holding a block
  if(robot.GetCarryingComponent().IsCarryingObject() && !CarryingObjectHandledInternally()){
    return false;
  }
  
  if((_wantsToRunStrategy != nullptr) &&
     !_wantsToRunStrategy->WantsToRun(robot)){
    return false;
  }
     
  return IsRunnableScored();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IBehavior::GetRNG() const {
  return _robot.GetRNG();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IBehavior::UpdateInternal(Robot& robot)
{
  if( IsActing() ) {  
    return Status::Running;
  }

  return Status::Complete;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::GetRunningDuration() const
{  
  if (_isRunning)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceStarted = currentTime_sec - _startedRunningTime_s;
    return timeSinceStarted;
  }
  return 0.0f;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IBehavior::ResumeInternal(Robot& robot)
{
  // by default, if we are runnable again, initialize and start over
  Result resumeResult = RESULT_FAIL;
  
  if ( IsRunnable(robot) ) {
    _isRunning = true;
    resumeResult = InitInternal(robot);
  }
  return resumeResult;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, RobotCompletedActionCallback callback)
{
  // needed for StopOnNextActionComplete to work properly, don't allow starting new actions if we've requested
  // the behavior to stop
  if( _stopRequestedAfterAction ) {
    delete action;
    return false;
  }
  
  if( !IsResuming() && !IsRunning() ) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotRunning",
                        "Behavior '%s' can't start %s action because it is not running",
                        GetIDStr().c_str(), action->GetName().c_str());
    delete action;
    return false;
  }

  if( IsActing() ) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.AlreadyActing",
                        "Behavior '%s' can't start %s action because it is already running an action in state %s",
                        GetIDStr().c_str(), action->GetName().c_str(),
                        GetDebugStateName().c_str());
    delete action;
    return false;
  }

  _actingCallback = callback;
  _lastActionTag = action->GetTag();
  
  ScoredActingStateChanged(true);

  Result result = _robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);
  if (RESULT_OK != result) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotQueued",
                        "Behavior '%s' can't queue action '%s' (error %d)",
                        GetIDStr().c_str(), action->GetName().c_str(), result);
    delete action;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, BehaviorRobotCompletedActionWithRobotCallback callback)
{
  return StartActing(action,
                     [this, callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg, _robot);
                     });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, ActionResultCallback callback)
{
  return StartActing(action,
                     [callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg.result);
                     });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, ActionResultWithRobotCallback callback)
{
  return StartActing(action,
                     [this, callback = std::move(callback)](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg.result, _robot);
                     });
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, SimpleCallback callback)
{
  return StartActing(action, [callback = std::move(callback)](ActionResult ret){ callback(); });
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StartActing(IActionRunner* action, SimpleCallbackWithRobot callback)
{
  return StartActing(action, [this, callback = std::move(callback)](ActionResult ret){ callback(_robot); });
}

void IBehavior::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
{
  if( IsActing() && msg.idTag == _lastActionTag ) {
    _lastActionTag = ActionConstants::INVALID_TAG;
    ScoredActingStateChanged(false);

    if( IsRunning() && _actingCallback) {
      _actingCallback(msg);
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StopActing(bool allowCallback, bool allowHelperToContinue)
{
  ScoredActingStateChanged(false);

  if( !allowHelperToContinue ) {
    // stop the helper first. This is generally what we want, because if someone else is stopping an action,
    // the helper likely won't know how to respond. Note that helpers can't call StopActing
    if( !_currentHelperHandle.expired() ) {
      PRINT_CH_INFO("Behaviors", (GetIDStr() + ".StopActing.WithoutCallback.StopHelper").c_str(),
                    "Stopping behavior helper because action stopped without callback");
    }
    StopHelperWithoutCallback();
  }    
  
  if( IsActing() ) {
    u32 tagToCancel = _lastActionTag;
      
    if( ! allowCallback ) {
      // if we want to block the callback, clear the tag here, before the cancel
      _lastActionTag = ActionConstants::INVALID_TAG;
    }

    // NOTE: if we are allowing callbacks, this cancel could run a bunch of behavior code
    bool ret = _robot.GetActionList().Cancel( tagToCancel );

    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running). If the callback called StartActing, we may have a new action tag, so only clear this if the
    // cancel didn't change it
    if( _lastActionTag == tagToCancel ) {
      _lastActionTag = ActionConstants::INVALID_TAG;
    }
    
    return ret;
  }

  return false;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved, bool broadcastToGame) const
{
  if(broadcastToGame && _robot.HasExternalInterface()){
    _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(objectiveAchieved);
  }
  PRINT_CH_INFO("Behaviors", "IBehavior.BehaviorObjectiveAchieved", "Behavior:%s, Objective:%s", GetIDStr().c_str(), EnumToString(objectiveAchieved));
  // send das event
  Util::sEventF("robot.freeplay_objective_achieved", {{DDATA, EnumToString(objectiveAchieved)}}, "%s", GetIDStr().c_str());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::NeedActionCompleted(NeedsActionId needsActionId)
{
  if (needsActionId == NeedsActionId::NoAction)
  {
    needsActionId = _needsActionID;
  }

  _robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(needsActionId);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartPushIdleAnimation(Robot& robot, AnimationTrigger animation)
{
  if(ANKI_VERIFY(!_hasSetIdle,
                 "IBehavior.SmartPushIdleAnimation.IdleAlreadySet",
                 "Behavior %s has already set an idle animation",
                 GetIDStr().c_str())){
    robot.GetAnimationStreamer().PushIdleAnimation(animation, kIdleLockPrefix + GetIDStr());
    _hasSetIdle = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartRemoveIdleAnimation(Robot& robot)
{
  if(ANKI_VERIFY(_hasSetIdle,
                 "IBehavior.SmartRemoveIdleAnimation.NoIdleSet",
                 "Behavior %s is trying to remove an idle, but none is currently set",
                 GetIDStr().c_str())){
    robot.GetAnimationStreamer().RemoveIdleAnimation(kIdleLockPrefix + GetIDStr());
    _hasSetIdle = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartDisableReactionsWithLock(const std::string& lockID,
                                              const TriggersArray& triggers)
{
  _robot.GetBehaviorManager().DisableReactionsWithLock(lockID + kSmartReactionLockSuffix, triggers);
  _smartLockIDs.insert(lockID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartRemoveDisableReactionsLock(const std::string& lockID)
{
  _robot.GetBehaviorManager().RemoveDisableReactionsLock(lockID + kSmartReactionLockSuffix);
  _smartLockIDs.erase(lockID);
}

#if ANKI_DEV_CHEATS
void IBehavior::SmartDisableReactionWithLock(const std::string& lockID, const ReactionTrigger& trigger)
{
  _robot.GetBehaviorManager().DisableReactionWithLock(lockID + kSmartReactionLockSuffix, trigger);
  _smartLockIDs.insert(lockID);
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartSetMotionProfile(const PathMotionProfile& motionProfile)
{
  ANKI_VERIFY(!_hasSetMotionProfile,
              "IBehavior.SmartSetMotionProfile.AlreadySet",
              "a profile was already set and not cleared");
  
  _robot.GetPathComponent().SetCustomMotionProfile(motionProfile);
  _hasSetMotionProfile = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartClearMotionProfile()
{
  ANKI_VERIFY(_hasSetMotionProfile,
              "IBehavior.SmartClearMotionProfile.NotSet",
              "a profile was not set, so can't be cleared");

  _robot.GetPathComponent().ClearCustomMotionProfile();
  _hasSetMotionProfile = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::SmartLockTracks(u8 animationTracks, const std::string& who, const std::string& debugName)
{
  // Only lock the tracks if we haven't already locked them with that key
  const bool insertionSuccessfull = _lockingNameToTracksMap.insert(std::make_pair(who, animationTracks)).second;
  if(insertionSuccessfull){
    _robot.GetMoveComponent().LockTracks(animationTracks, who, debugName);
    return true;
  }else{
    PRINT_NAMED_WARNING("IBehavior.SmartLockTracks.AttemptingToLockTwice", "Attempted to lock tracks with key named %s but key already exists", who.c_str());
    return false;
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::SmartUnLockTracks(const std::string& who)
{
  auto mapIter = _lockingNameToTracksMap.find(who);
  if(mapIter == _lockingNameToTracksMap.end()){
    PRINT_NAMED_WARNING("IBehavior.SmartUnlockTracks.InvalidUnlock", "Attempted to unlock tracks with key named %s but key not found", who.c_str());
    return false;
  }else{
    _robot.GetMoveComponent().UnlockTracks(mapIter->second, who);
    mapIter = _lockingNameToTracksMap.erase(mapIter);
    return true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::SmartSetCustomLightPattern(const ObjectID& objectID,
                                           const CubeAnimationTrigger& anim,
                                           const ObjectLights& modifier)
{
  if(std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID) == _customLightObjects.end()){
    _robot.GetCubeLightComponent().PlayLightAnim(objectID, anim, nullptr, true, modifier);
    _customLightObjects.push_back(objectID);
    return true;
  }else{
    PRINT_NAMED_INFO("IBehavior.SmartSetCustomLightPattern.LightsAlreadySet",
                        "A custom light pattern has already been set on object %d", objectID.GetValue());
    return false;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::SmartRemoveCustomLightPattern(const ObjectID& objectID,
                                              const std::vector<CubeAnimationTrigger>& anims)
{
  auto objectIter = std::find(_customLightObjects.begin(), _customLightObjects.end(), objectID);
  if(objectIter != _customLightObjects.end()){
    for(const auto& anim : anims)
    {
      _robot.GetCubeLightComponent().StopLightAnimAndResumePrevious(anim, objectID);
    }
    _customLightObjects.erase(objectIter);
    return true;
  }else{
    PRINT_NAMED_INFO("IBehavior.SmartRemoveCustomLightPattern.LightsNotSet",
                        "No custom light pattern is set for object %d", objectID.GetValue());
    return false;
  }
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::SmartDelegateToHelper(Robot& robot,
                                      HelperHandle handleToRun,
                                      SimpleCallbackWithRobot successCallback,
                                      SimpleCallbackWithRobot failureCallback)
{
  PRINT_CH_INFO("Behaviors", (GetIDStr() + ".SmartDelegateToHelper").c_str(),
                "Behavior requesting to delegate to helper %s", handleToRun->GetName().c_str());

  if(!_currentHelperHandle.expired()){
   PRINT_NAMED_WARNING("IBehavior.SmartDelegateToHelper",
                       "Attempted to start a handler while handle already running, stopping running helper");
   StopHelperWithoutCallback();
  }
  const bool delegateSuccess = _robot.GetAIComponent().GetBehaviorHelperComponent().
                               DelegateToHelper(robot,
                                                handleToRun,
                                                successCallback,
                                                failureCallback);

  if( delegateSuccess ) {
    _currentHelperHandle = handleToRun;
  }
  else {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + "SmartDelegateToHelper.Failed").c_str(),
                  "Failed to delegate to helper");
  }
  
  return delegateSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SetBehaviorStateLights(const std::vector<BehaviorStateLightInfo>& structToSet, bool persistOnReaction)
{
  _robot.GetBehaviorManager().SetBehaviorStateLights(GetClass(), structToSet, persistOnReaction);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorHelperFactory& IBehavior::GetBehaviorHelperFactory()
{
  return _robot.GetAIComponent().GetBehaviorHelperComponent().GetBehaviorHelperFactory();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::StopHelperWithoutCallback()
{
  bool handleStopped = false;
  auto handle = _currentHelperHandle.lock();
  if( handle ) {
    PRINT_CH_INFO("Behaviors", (GetIDStr() + ".SmartStopHelper").c_str(),
                  "Behavior stopping its helper");

    handleStopped = _robot.GetAIComponent().GetBehaviorHelperComponent().StopHelperWithoutCallback(handle);
  }
  
  return handleStopped;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult IBehavior::UseSecondClosestPreActionPose(DriveToObjectAction* action,
                                                      ActionableObject* object,
                                                      std::vector<Pose3d>& possiblePoses,
                                                      bool& alreadyInPosition)
{
  auto result = action->GetPossiblePoses(object, possiblePoses, alreadyInPosition);
  if (result != ActionResult::SUCCESS) {
    return result;
  }

  if( possiblePoses.size() > 1 && IDockAction::RemoveMatchingPredockPose( _robot.GetPose(), possiblePoses ) ) {
    alreadyInPosition = false;
  }
    
  return ActionResult::SUCCESS;
}


////////
//// Scored Behavior functions
///////
namespace {
  static const char* kEmotionScorersKey            = "emotionScorers";
  static const char* kFlatScoreKey                 = "flatScore";
  static const char* kRepetitionPenaltyKey         = "repetitionPenalty";
  static const char* kRunningPenaltyKey            = "runningPenalty";
  static const char* kCooldownOnObjectiveKey       = "considerThisHasRunForBehaviorObjective";
  static const constexpr float kDisableRepetitionPenaltyOnStop_s = 1.f;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::StopWithoutImmediateRepetitionPenalty()
{
  Stop();
  _nextTimeRepetitionPenaltyApplies_s  = kDisableRepetitionPenaltyOnStop_s + BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EvaluateScoreInternal is virtual and can optionally be overridden by subclasses
float IBehavior::EvaluateScoreInternal(const Robot& robot) const
{
  float score = _flatScore;
  if ( !_moodScorer.IsEmpty() ) {
    score = _moodScorer.EvaluateEmotionScore(robot.GetMoodManager());
  }
  return score;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// EvaluateRunningScoreInternal is virtual and can optionally be overridden by subclasses
float IBehavior::EvaluateRunningScoreInternal(const Robot& robot) const
{
  // unless specifically overridden it should mimic the non-running score
  const float nonRunningScore = EvaluateScoreInternal(robot);
  return nonRunningScore;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateRepetitionPenalty() const
{
  if (_lastRunTime_s > 0.0f)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceRun = currentTime_sec - _lastRunTime_s;
    const float repetitionPenalty = _repetitionPenalty.EvaluateY(timeSinceRun);
    return repetitionPenalty;
  }
  
  return 1.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateRunningPenalty() const
{
  if (_startedRunningTime_s > 0.0f)
  {
    const float currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceStarted = currentTime_sec - _startedRunningTime_s;
    const float runningPenalty = _runningPenalty.EvaluateY(timeSinceStarted);
    return runningPenalty;
  }
  
  return 1.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateScore(const Robot& robot) const
{
#if ANKI_DEV_CHEATS
  DEV_ASSERT_MSG(!_scoringValuesCached.isNull(),
                 "IBehavior.EvaluateScore.ScoreNotSet",
                 "No score was loaded in for behavior name %s",
                 GetIDStr().c_str());
#endif
  if (IsRunning() || IsRunnable(robot))
  {
    const bool isRunning = IsRunning();
    
    float score = 0.0f;
    
    if (isRunning)
    {
      score = EvaluateRunningScoreInternal(robot) + _extraRunningScore;
    }
    else
    {
      score = EvaluateScoreInternal(robot);
    }
    
    // use running penalty while running, repetition penalty otherwise
    if (_enableRunningPenalty && isRunning)
    {
      const float runningPenalty = EvaluateRunningPenalty();
      score *= runningPenalty;
    }

    
    if (_enableRepetitionPenalty && !isRunning)
    {
      const bool shouldIgnorePenaltyThisTick = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() < _nextTimeRepetitionPenaltyApplies_s;
      if(!shouldIgnorePenaltyThisTick){
        const float repetitionPenalty = EvaluateRepetitionPenalty();
        score *= repetitionPenalty;
      }
    }
    
    return score;
  }
  
  return 0.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::HandleBehaviorObjective(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  if( _cooldownOnObjective != BehaviorObjective::Count && msg.behaviorObjective == _cooldownOnObjective ) {
    // set last run time now (even though this behavior may not have run) so that it will incur repetition
    // penalty as if it had run. This is useful as a way to sort of "share" cooldowns between multiple
    // behaviors which are trying to do the same thing (but only if they succeed)
    _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::IncreaseScoreWhileActing(float extraScore)
{
  if( IsActing() ) {
    _extraRunningScore += extraScore;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::ReadFromScoredJson(const Json::Value& config, const bool fromScoredChooser)
{
  
  #if ANKI_DEV_CHEATS
    if(_scoringValuesCached.isNull() && fromScoredChooser){
      _scoringValuesCached = config;
    }else if(fromScoredChooser){
      DEV_ASSERT_MSG(_scoringValuesCached == config,
                     "IBehavior.ReadFromScoredJson.Scoring config Mismatch",
                     "Behavior named %s has two differenc scoring configs",
                     GetIDStr().c_str());
      // we've already loaded in scores, don't need to do it again
      return true;
    }
  #endif
  
  // - - - - - - - - - -
  // Mood scorer
  // - - - - - - - - - -
  _moodScorer.ClearEmotionScorers();
  
  const Json::Value& emotionScorersJson = config[kEmotionScorersKey];
  if (!emotionScorersJson.isNull())
  {
    _moodScorer.ReadFromJson(emotionScorersJson);
  }
  
  // - - - - - - - - - -
  // Flat score
  // - - - - - - - - - -
  
  const Json::Value& flatScoreJson = config[kFlatScoreKey];
  if (!flatScoreJson.isNull()) {
    _flatScore = flatScoreJson.asFloat();
  }
  
  // make sure we only set one scorer (flat or mood)
  DEV_ASSERT( flatScoreJson.isNull() || _moodScorer.IsEmpty(), "IBehavior.ReadFromJson.MultipleScorers" );
  
  // - - - - - - - - - -
  // Repetition penalty
  // - - - - - - - - - -
  
  _repetitionPenalty.Clear();
  
  const Json::Value& repetitionPenaltyJson = config[kRepetitionPenaltyKey];
  if (!repetitionPenaltyJson.isNull())
  {
    if (!_repetitionPenalty.ReadFromJson(repetitionPenaltyJson))
    {
      PRINT_NAMED_WARNING("IScoredBehavior.BadRepetitionPenalty",
                          "Behavior '%s': %s failed to read",
                          GetIDStr().c_str(),
                          kRepetitionPenaltyKey);
    }
  }
  
  
  // Ensure there is a valid graph
  if (_repetitionPenalty.GetNumNodes() == 0)
  {
    _repetitionPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
  }
  
  // - - - - - - - - - -
  // cooldown on other objective
  // - - - - - - - - - -
  
  const Json::Value& cooldownOnObjectiveJson = config[kCooldownOnObjectiveKey];
  if (!cooldownOnObjectiveJson.isNull()) {
    const char* objectiveStr = cooldownOnObjectiveJson.asCString();
    _cooldownOnObjective = BehaviorObjectiveFromString(objectiveStr);
    if( _cooldownOnObjective == BehaviorObjective::Count ) {
      PRINT_NAMED_WARNING("IScoredBehavior.BadBehaviorObjective",
                          "could not convert '%s' to valid behavior objective",
                          objectiveStr);
    }
  }
  
  
  // - - - - - - - - - -
  // Running penalty
  // - - - - - - - - - -
  
  _runningPenalty.Clear();
  
  const Json::Value& runningPenaltyJson = config[kRunningPenaltyKey];
  if (!runningPenaltyJson.isNull())
  {
    if (!_runningPenalty.ReadFromJson(runningPenaltyJson))
    {
      PRINT_NAMED_WARNING("IBehavior.BadRunningPenalty",
                          "Behavior '%s': %s failed to read",
                          GetIDStr().c_str(),
                          kRunningPenaltyKey);
    }
  }
  
  // Ensure there is a valid graph
  if (_runningPenalty.GetNumNodes() == 0)
  {
    _runningPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
  }
  return true;
}

  
  
void IBehavior::ScoredActingStateChanged(bool isActing)
{
  _extraRunningScore = 0.0f;
}

  
void IBehavior::ScoredConstructor(Robot& robot)
{
  if(_robot.HasExternalInterface()) {
    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(
       EngineToGameTag::BehaviorObjectiveAchieved,
       [this](const EngineToGameEvent& event) {
         DEV_ASSERT(event.GetData().GetTag() == EngineToGameTag::BehaviorObjectiveAchieved,
                    "IBehavior.ScoredConstructor.WrongEventTypeFromCallback");
         HandleBehaviorObjective(event.GetData().Get_BehaviorObjectiveAchieved());
       } ));
  }

}
  
  
void IBehavior::InitScored(Robot& robot)
{
  _timesResumedFromPossibleInfiniteLoop = 0;
}
  
  
bool IBehavior::IsRunnableScored() const
{
  // Currently we only resume from scored behaviors, which is why we have this
  // logic separated out
  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // check to see if we're on a cliff cooldown
  if(curTime < _timeCanRunAfterPossibleInfiniteLoopCooldown_sec){
    return false;
  }
  
  return true;
}


  
} // namespace Cozmo
} // namespace Anki
