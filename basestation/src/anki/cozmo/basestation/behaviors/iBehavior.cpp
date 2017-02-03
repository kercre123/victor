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

#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/audio/behaviorAudioClient.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "anki/cozmo/basestation/components/cubeLightComponent.h"
#include "anki/cozmo/basestation/components/movementComponent.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotManager.h"


#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/enums/stringToEnumMapper.hpp"
#include "util/math/numericCast.h"


namespace Anki {
namespace Cozmo {

  
// Static initializations
const char* IBehavior::kBaseDefaultName = "no_name";

namespace {
  
static const char* kNameKey                      = "name";
static const char* kDisplayNameKey               = "displayNameKey";
static const char* kBehaviorGroupsKey            = "behaviorGroups";
static const char* kRequiredUnlockKey            = "requiredUnlockId";
static const char* kRequiredDriveOffChargerKey   = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey      = "requiredRecentSwitchToParent_sec";
static const char* kExecutableBehaviorTypeKey    = "executableBehaviorType";
static const char* kRequireObjectTappedKey       = "requireObjectTapped";
static const int kMaxResumesFromCliff            = 2;
static const float kCooldownFromCliffResumes_sec = 15.0;
  
}

IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _requiredProcess( AIInformationAnalysis::EProcess::Invalid )
  , _robot(robot)
  , _lastRunTime_s(0.0)
  , _startedRunningTime_s(0.0)
  , _executableType(ExecutableBehaviorType::Count)
  , _requiredUnlockId( UnlockId::Count )
  , _requiredRecentDriveOffCharger_sec(-1.0f)
  , _requiredRecentSwitchToParent_sec(-1.0f)
  , _isRunning(false)
  , _isResuming(false)
  , _isOwnedByFactory(false)
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
  const Json::Value& nameJson = config[kNameKey];
  _name = nameJson.isString() ? nameJson.asCString() : kBaseDefaultName;
  JsonTools::GetValueOptional(config, kDisplayNameKey, _displayNameKey);

  // - - - - - - - - - -
  // Required unlock
  // - - - - - - - - - -
  const Json::Value& requiredUnlockJson = config[kRequiredUnlockKey];
  if ( !requiredUnlockJson.isNull() )
  {
    DEV_ASSERT(requiredUnlockJson.isString(), "IBehavior.ReadFromJson.NonStringUnlockId");
    
    // this is probably the only place where we need this, otherwise please refactor to proper header
    const UnlockId requiredUnlock = UnlockIdsFromString(requiredUnlockJson.asString());
    if ( requiredUnlock != UnlockId::Count ) {
      PRINT_NAMED_INFO("IBehavior.ReadFromJson.RequiredUnlock", "Behavior '%s' requires unlock '%s'",
                        GetName().c_str(), requiredUnlockJson.asString().c_str() );
      _requiredUnlockId = requiredUnlock;
    } else {
      PRINT_NAMED_ERROR("IBehavior.ReadFromJson.InvalidUnlockId", "Could not convert string to unlock id '%s'",
        requiredUnlockJson.asString().c_str());
    }
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

  // - - - - - - - - - -
  // Behavior Groups
  // - - - - - - - - - -
  
  ClearBehaviorGroups();
    
  const Json::Value& behaviorGroupsJson = config[kBehaviorGroupsKey];
  if (behaviorGroupsJson.isArray())
  {
    const uint32_t numBehaviorGroups = behaviorGroupsJson.size();
      
    const Json::Value kNullValue;
      
    for (uint32_t i = 0; i < numBehaviorGroups; ++i)
    {
      const Json::Value& behaviorGroupJson = behaviorGroupsJson.get(i, kNullValue);
        
      const char* behaviorGroupString = behaviorGroupJson.isString() ? behaviorGroupJson.asCString() : "";
      const BehaviorGroup behaviorGroup = BehaviorGroupFromString(behaviorGroupString);
        
      if (behaviorGroup != BehaviorGroup::Count)
      {
        SetBehaviorGroup(behaviorGroup, true);
      }
      else
      {
        PRINT_NAMED_WARNING("IBehavior.BadBehaviorGroup", "Failed to read group %u '%s'", i, behaviorGroupString);
      }
    }
  }
    
  const Json::Value& executableBehaviorTypeJson = config[kExecutableBehaviorTypeKey];
  if (executableBehaviorTypeJson.isString())
  {
    _executableType = ExecutableBehaviorTypeFromString(executableBehaviorTypeJson.asCString());
  }
  
  JsonTools::GetValueOptional(config, kRequireObjectTappedKey, _requireObjectTapped);
  
  return ReadFromScoredJson(config);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::~IBehavior()
{
  DEV_ASSERT(!IsOwnedByFactory(), "Behavior must be removed from factory before destroying it!");
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
  PRINT_CH_INFO("Behaviors", (GetName() + ".Init").c_str(), "Starting...");

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
                        GetName().c_str(), _robot.GetActionList().GetQueueLength(0));
  }

  
  // Streamline all IBehaviors when a spark is active
  if(_robot.GetBehaviorManager().GetActiveSpark() != UnlockId::Count
     && !_robot.GetBehaviorManager().IsActiveSparkSoft()){
    _shouldStreamline = true;
  }else{
    _shouldStreamline = false;
  }
  
  _isRunning = true;
  _canStartActing = true;
  _actingCallback = nullptr;
  _startedRunningTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
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
    SmartDisableReactionTrigger(ReactionTrigger::ObjectPositionUpdated);
  }
  
  UpdateTappedObjectLights(true);
  InitScored(_robot);
  
  return initResult;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IBehavior::Resume(ReactionTrigger resumingFromType)
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".Resume").c_str(), "Resuming...");
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
  
  if ( initResult != RESULT_OK ) {
    _isRunning = false;
  } else {
    // default implementation of ResumeInternal also sets it to true, but behaviors that override it
    // might not set it
    _isRunning = true;
  }
  
  // Disable Acknowledge object if this behavior is the sparked version
  if(_requiredUnlockId != UnlockId::Count
     && _requiredUnlockId == _robot.GetBehaviorManager().GetActiveSpark()){
    SmartDisableReactionTrigger(ReactionTrigger::ObjectPositionUpdated);
  }
  
  UpdateTappedObjectLights(true);
  
  return initResult;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::Status IBehavior::Update()
{
  DEV_ASSERT(IsRunning(), "IBehavior::UpdateNotRunning");
  // Ensure that behaviors which have been requested to stop, stop
  if(!_canStartActing && !IsActing()){
    UpdateInternal(_robot);
    return Status::Complete;
  }
  
  return UpdateInternal(_robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::Stop()
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".Stop").c_str(), "Stopping...");

  _isRunning = false;
  // If the behavior uses double tapped objects then call StopInternalFromDoubleTap in case
  // StopInternal() does things like fire mood events or sets failures to use
  (RequiresObjectTapped() ? StopInternalFromDoubleTap(_robot) : StopInternal(_robot));
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StopActing(false);
  
  // Re-enable any reactionary behaviors which the behavior disabled and didn't have a chance to
  // re-enable before stopping
  SmartReEnableReactionTrigger(_disabledReactionTriggers);

  // re-enable tap interaction if needed
  if( _tapInteractionDisabled ) {
    SmartReEnableTapInteraction();
  }
  
  // Unlock any tracks which the behavior hasn't had a chance to unlock
  for(const auto& entry: _lockingNameToTracksMap){
    _robot.GetMoveComponent().UnlockTracks(entry.second, entry.first);
  }
  _lockingNameToTracksMap.clear();
  
  // Clear any light patterns which were set on cubes
  _robot.GetCubeLightComponent().StopAllAnims();
  
  _customLightObjects.clear();
  
  DEV_ASSERT(_disabledReactionTriggers.empty(), "IBehavior.Stop.DisabledReactionsNotEmpty");
  DEV_ASSERT(!_tapInteractionDisabled, "IBehavior.Stop.TapInteractionStillDisabled");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::StopOnNextActionComplete()
{
  // clear the callback and don't let any new actions start
  _canStartActing = false;
  _actingCallback = nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::IsRunnableBase(const Robot& robot) const
{
  // Some reaction trigger strategies allow behaviors to interrupt themselves.
  // DEV_ASSERT(!IsRunning(), "IBehavior.IsRunnableCalledOnRunningBehavior");
  if (IsRunning()) {
    //PRINT_CH_DEBUG("Behaviors", "IBehavior.IsRunnableBase", "Behavior %s is already running", GetName().c_str());
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
        GetName().c_str());
      return false;
    }
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
    
    const bool isRecent = FLT_LE(curTime, (lastDriveOff+_requiredRecentDriveOffCharger_sec));
    if ( !isRecent ) {
      // driven off, but not recently enough
      return false;
    }
  }
  
  // if there's a timer requiring a recent parent switch
  const bool requiresRecentParentSwitch = FLT_GE(_requiredRecentSwitchToParent_sec, 0.0);
  if ( requiresRecentParentSwitch ) {
    const float lastTime = robot.GetBehaviorManager().GetLastBehaviorChooserSwitchTime();
    const float curTime  = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
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
  
  //check if the behavior can handle holding a block
  if(robot.IsCarryingObject() && !CarryingObjectHandledInternally()){
    return false;
  }
  
  // check if the behavior needs a tapped object and if there is a tapped object
  if(_requireObjectTapped)
  {
    if(!robot.GetAIComponent().GetWhiteboard().HasTapIntent())
    {
      return false;
    }
    // Otherwise there is a tapped object so we need to make sure this behaviors target blocks are up to
    // date before checking IsRunnableInternal()
    else
    {
      UpdateTargetBlocks(robot);
    }
  }
  
  BehaviorPreReqNone preReqData;
  return IsRunnableScored(preReqData);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Util::RandomGenerator& IBehavior::GetRNG() const {
  return _robot.GetRNG();
}

void IBehavior::SetDefaultName(const char* inName)
{
  if (_name == kBaseDefaultName) {
    _name = inName;
  }
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
double IBehavior::GetRunningDuration() const
{  
  if (_isRunning)
  {
    const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const double timeSinceStarted = currentTime_sec - _startedRunningTime_s;
    return timeSinceStarted;
  }
  return 0.0;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result IBehavior::ResumeInternal(Robot& robot)
{
  // by default, if we are runnable again, initialize and start over
  Result resumeResult = RESULT_FAIL;
  
  // If this behavior needs a tapped object and there is a tapped object make sure
  // to update our target blocks before trying to resume
  if(_requireObjectTapped && robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    UpdateTargetBlocks(robot);
  }
  
  BehaviorPreReqRobot preReqData(robot);
  if ( IsRunnable(preReqData) ) {
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
  if( ! _canStartActing ) {
    return false;
  }
  
  if( !IsResuming() && !IsRunning() ) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.NotRunning",
                        "Behavior '%s' can't start acting because it is not running",
                        GetName().c_str());
    return false;
  }

  if( IsActing() ) {
    PRINT_NAMED_WARNING("IBehavior.StartActing.Failure.AlreadyActing",
                        "Behavior '%s' can't start acting because it is already running an action",
                        GetName().c_str());
    return false;
  }

  _actingCallback = callback;
  _lastActionTag = action->GetTag();
  
  ScoredActingStateChanged(true);
  _robot.GetActionList().QueueAction(QueueActionPosition::NOW, action);
  return true;
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
bool IBehavior::StopActing(bool allowCallback)
{
  ScoredActingStateChanged(false);

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
void IBehavior::BehaviorObjectiveAchieved(BehaviorObjective objectiveAchieved)
{
  if(_robot.HasExternalInterface()){
    _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::BehaviorObjectiveAchieved>(objectiveAchieved);
  }
  PRINT_CH_INFO("Behaviors", "IBehavior.BehaviorObjectiveAchieved", "Behavior:%s, Objective:%s", GetName().c_str(), EnumToString(objectiveAchieved));
  // send das event
  Util::sEventF("robot.freeplay_objective_achieved", {{DDATA, EnumToString(objectiveAchieved)}}, "%s", GetName().c_str());

  UpdateTappedObjectLights(false);

}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartDisableReactionTrigger(ReactionTrigger trigger)
{
  _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(), trigger, false);
  _disabledReactionTriggers.insert(trigger);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartDisableReactionTrigger(const std::set<ReactionTrigger>& triggerList)
{
  for(auto trigger: triggerList){
    SmartDisableReactionTrigger(trigger);
  }
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartReEnableReactionTrigger(ReactionTrigger trigger)
{
  if(_disabledReactionTriggers.find(trigger) != _disabledReactionTriggers.end()) {
    _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(), trigger, true);
    _disabledReactionTriggers.erase(trigger);
  }else{
    PRINT_NAMED_ERROR("IBehavior.SmartReEnableReactionaryBehavior",
                        "Attempted to re-enable reaction that wasn't disabled with smart disable");
  }
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartReEnableReactionTrigger(const std::set<ReactionTrigger> triggerList)
{
  for(auto triger: triggerList){
    SmartReEnableReactionTrigger(triger);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartDisableTapInteraction()
{
  if( !_tapInteractionDisabled ) {
    _robot.GetBehaviorManager().RequestEnableTapInteraction(GetName(), false);
    _tapInteractionDisabled = true;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::SmartReEnableTapInteraction()
{
  if( _tapInteractionDisabled ) {
    _robot.GetBehaviorManager().RequestEnableTapInteraction(GetName(), true);
    _tapInteractionDisabled = false;
  }
  else {
    PRINT_NAMED_WARNING("IBehavior.SmartReEnableTapInteraction.NotDisabled",
                        "Attempted to re-enable tap interaction (manually), but it wasn't disabled");
  }    
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior::ReactionTriggerIter IBehavior::SmartReEnableReactionTrigger(ReactionTriggerIter iter)
{
  _robot.GetBehaviorManager().RequestEnableReactionTrigger(GetName(), *iter, true);
  return  _disabledReactionTriggers.erase(iter);
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
      _robot.GetCubeLightComponent().StopLightAnim(anim, objectID);
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
ActionResult IBehavior::UseSecondClosestPreActionPose(DriveToObjectAction* action,
                                                      ActionableObject* object,
                                                      std::vector<Pose3d>& possiblePoses,
                                                      bool& alreadyInPosition)
{
  auto result = action->GetPossiblePoses(object, possiblePoses, alreadyInPosition);
  if (result != ActionResult::SUCCESS) {
    return result;
  }
  
  for(auto iter = possiblePoses.begin(); iter != possiblePoses.end(); )
  {
    if(possiblePoses.size() > 1 && iter->IsSameAs(_robot.GetPose(), kSamePreactionPoseDistThresh_mm,
                                                  DEG_TO_RAD(kSamePreactionPoseAngleThresh_deg)))
    {
      iter = possiblePoses.erase(iter);
      alreadyInPosition = false;
    } else {
      ++iter;
    }
  }
  
  return ActionResult::SUCCESS;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IBehavior::UpdateTappedObjectLights(const bool on) const
{
  // Prevents enabling/disabling the ReactToCubeMoved behavior multiple times
  static bool behaviorDisabled = false;

  if(_requireObjectTapped &&
     _robot.GetAIComponent().GetWhiteboard().HasTapIntent())
  {
    const ObjectID& _tappedObject = _robot.GetBehaviorManager().GetCurrTappedObject();
    
    _robot.GetCubeLightComponent().StopLightAnim(CubeAnimationTrigger::DoubleTappedKnown);
    _robot.GetCubeLightComponent().StopLightAnim(CubeAnimationTrigger::DoubleTappedUnsure);
    
    if(on)
    {
      _robot.GetCubeLightComponent().SetTapInteractionObject(_tappedObject);
    
      if(!behaviorDisabled)
      {
        _robot.GetBehaviorManager().RequestEnableReactionTrigger("ObjectTapInteraction",
                                                                 ReactionTrigger::CubeMoved,
                                                                 false);
        behaviorDisabled = true;
      }
    }
    else
    {
      if(behaviorDisabled)
      {
        _robot.GetBehaviorManager().RequestEnableReactionTrigger("ObjectTapInteraction",
                                                                 ReactionTrigger::CubeMoved,
                                                                 true);
        behaviorDisabled = false;
      }
      
      _robot.GetBehaviorManager().LeaveObjectTapInteraction();
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::HandleNewDoubleTap(Robot& robot)
{
  Stop();
  UpdateTargetBlocks(robot);
  
  BehaviorPreReqRobot preReqData(robot);
  
  if(!IsRunnable(preReqData))
  {
    return false;
  }
  
  Init();
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::UpdateAudioState(int newAudioState)
{
  return _robot.GetBehaviorManager().GetAudioClient().UpdateBehaviorRound(_requiredUnlockId, newAudioState);
}
  


////////
//// Scored Behavior fuctions
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
// EvaluateScoreInternal is virtual and can optionally be overridden by subclasses
float IBehavior::EvaluateRunningScoreInternal(const Robot& robot) const
{
  // unless specifically overridden it should mimic the non-running score
  const float nonRunningScore = EvaluateScoreInternal(robot);
  return nonRunningScore;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateRepetitionPenalty() const
{
  if (_lastRunTime_s > 0.0)
  {
    const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceRun = Util::numeric_cast<float>(currentTime_sec - _lastRunTime_s);
    const float repetitionPenalty = _repetitionPenalty.EvaluateY(timeSinceRun);
    return repetitionPenalty;
  }
  
  return 1.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateRunningPenalty() const
{
  if (_startedRunningTime_s > 0.0)
  {
    const double currentTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float timeSinceStarted = Util::numeric_cast<float>(currentTime_sec - _startedRunningTime_s);
    const float runningPenalty = _runningPenalty.EvaluateY(timeSinceStarted);
    return runningPenalty;
  }
  
  return 1.0f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float IBehavior::EvaluateScore(const Robot& robot) const
{
  BehaviorPreReqRobot preReqData(robot);
  if (IsRunning() || IsRunnable(preReqData))
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
bool IBehavior::ReadFromScoredJson(const Json::Value& config)
{
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
                          GetName().c_str(),
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
                          GetName().c_str(),
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
  
  
bool IBehavior::IsRunnableScored(const BehaviorPreReqNone& preReqData) const
{
  // Currently we only resume from scorred behaviors, which is why we have this
  // logic seperated out
  const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // check to see if we're on a cliff cooldown
  if(curTime < _timeCanRunAfterPossibleInfiniteLoopCooldown_sec){
    return false;
  }
  
  return true;
}


  
} // namespace Cozmo
} // namespace Anki
