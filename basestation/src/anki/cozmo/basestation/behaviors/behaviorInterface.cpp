/**
 * File: behaviorInterface.cpp
 *
 * Author: Andrew Stein
 * Date:   7/30/15
 *
 * Description: Defines interface for a Cozmo "Behavior".
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/actionInterface.h"
#include "anki/cozmo/basestation/actions/dockActions.h"
#include "anki/cozmo/basestation/actions/driveToActions.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/components/progressionUnlockComponent.h"
#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/enums/stringToEnumMapper.hpp"
#include "util/math/numericCast.h"


namespace Anki {
namespace Cozmo {

// Static initializations
const char* IBehavior::kBaseDefaultName = "no_name";
  
static const char* kNameKey                    = "name";
static const char* kEmotionScorersKey          = "emotionScorers";
static const char* kFlatScoreKey               = "flatScore";
static const char* kRepetitionPenaltyKey       = "repetitionPenalty";
static const char* kRunningPenaltyKey          = "runningPenalty";
static const char* kBehaviorGroupsKey          = "behaviorGroups";
static const char* kRequiredUnlockKey          = "requiredUnlockId";
static const char* kRequiredDriveOffChargerKey = "requiredRecentDriveOffCharger_sec";
static const char* kRequiredParentSwitchKey    = "requiredRecentSwitchToParent_sec";
static const char* kDisableReactionaryDefault  = "disableByDefault";
static const char* kShouldStreamline           = "shouldStreamline";

  
IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _behaviorType(BehaviorType::NoneBehavior)
  , _requiredUnlockId( UnlockId::Count )
  , _requiredRecentDriveOffCharger_sec(-1.0f)
  , _requiredRecentSwitchToParent_sec(-1.0f)
  , _moodScorer()
  , _flatScore(0.0f)
  , _robot(robot)
  , _startedRunningTime_s(0.0)
  , _lastRunTime_s(0.0)
  , _extraRunningScore(0.0f)
  , _isRunning(false)
  , _isOwnedByFactory(false)
  , _enableRepetitionPenalty(true)
  , _enableRunningPenalty(true)
{
  ReadFromJson(config);

  if(_robot.HasExternalInterface()) {
    // NOTE: this won't get sent down to derived classes (unless they also subscribe)
    _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(
                              EngineToGameTag::RobotCompletedAction,
                              [this](const EngineToGameEvent& event) {
                                ASSERT_NAMED(event.GetData().GetTag() == EngineToGameTag::RobotCompletedAction,
                                             "Wrong event type from callback");
                                HandleActionComplete(event.GetData().Get_RobotCompletedAction());
                              } ));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IBehavior::ReadFromJson(const Json::Value& config)
{
  const Json::Value& nameJson = config[kNameKey];
  _name = nameJson.isString() ? nameJson.asCString() : kBaseDefaultName;

  // - - - - - - - - - -
  // Required unlock
  // - - - - - - - - - -
  const Json::Value& requiredUnlockJson = config[kRequiredUnlockKey];
  if ( !requiredUnlockJson.isNull() )
  {
    ASSERT_NAMED(requiredUnlockJson.isString(), "IBehavior.ReadFromJson.NonStringUnlockId");
    
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
  if ( !requiredDriveOffChargerJson.isNull() )
  {
    ASSERT_NAMED_EVENT(requiredDriveOffChargerJson.isNumeric(), "IBehavior.ReadFromJson", "Not a float: %s", kRequiredDriveOffChargerKey);
    _requiredRecentDriveOffCharger_sec = requiredDriveOffChargerJson.asFloat();
  }
  
  // - - - - - - - - - -
  // Required recent parent switch
  const Json::Value& requiredSwitchToParentJson = config[kRequiredParentSwitchKey];
  if ( !requiredSwitchToParentJson.isNull() ) {
    ASSERT_NAMED_EVENT(requiredSwitchToParentJson.isNumeric(), "IBehavior.ReadFromJson", "Not a float: %s", kRequiredParentSwitchKey);
    _requiredRecentSwitchToParent_sec = requiredSwitchToParentJson.asFloat();
  }

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
  ASSERT_NAMED( flatScoreJson.isNull() || _moodScorer.IsEmpty(), "IBehavior.ReadFromJson.MultipleScorers" );
  
  // - - - - - - - - - -
  // Repetition penalty
  // - - - - - - - - - -
  
  _repetitionPenalty.Clear();
    
  const Json::Value& repetitionPenaltyJson = config[kRepetitionPenaltyKey];
  if (!repetitionPenaltyJson.isNull())
  {
    if (!_repetitionPenalty.ReadFromJson(repetitionPenaltyJson))
    {
      PRINT_NAMED_WARNING("IBehavior.BadRepetitionPenalty",
                          "Behavior '%s': %s failed to read",
                          _name.c_str(),
                          kRepetitionPenaltyKey);
    }
  }
    
  // Ensure there is a valid graph
  if (_repetitionPenalty.GetNumNodes() == 0)
  {
    _repetitionPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
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
                          _name.c_str(),
                          kRunningPenaltyKey);
    }
  }
    
  // Ensure there is a valid graph
  if (_runningPenalty.GetNumNodes() == 0)
  {
    _runningPenalty.AddNode(0.0f, 1.0f); // no penalty for any value
  }
  
  // - - - - - - - - - -
  // Streamline behavior
  // - - - - - - - - - -
  
  _shouldStreamline = config.get(kShouldStreamline, false).asBool();

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
    
  return true;
}

IBehavior::~IBehavior()
{
  ASSERT_NAMED(!IsOwnedByFactory(), "Behavior must be removed from factory before destroying it!");
}
  
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
  
void IBehavior::SubscribeToTags(const uint32_t robotId, std::set<RobotInterface::RobotToEngineTag> &&tags)
{
  auto handlerCallback = [this](const RobotToEngineEvent& event) {
    HandleEvent(event);
  };
  
  for(auto tag : tags) {
    _eventHandles.push_back(_robot.GetRobotMessageHandler()->Subscribe(robotId, tag, handlerCallback));
  }
}

Result IBehavior::Init()
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".Init").c_str(), "Starting...");

  if(_robot.GetActionList().GetNumQueues() > 0 && _robot.GetActionList().GetQueueLength(0) > 0) {
    PRINT_NAMED_WARNING("IBehavior.Init.ActionsInQueue",
                        "Initializing %s: %zu actions already in queue",
                        GetName().c_str(), _robot.GetActionList().GetQueueLength(0));
  }
  
  _isRunning = true;
  _startedRunningTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  Result initResult = InitInternal(_robot);
  if ( initResult != RESULT_OK ) {
    _isRunning = false;
  }
  else {
    _startCount++;
  }
  return initResult;
}

Result IBehavior::Resume()
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".Resume").c_str(), "Resuming...");
  
  _isRunning = true;
  _startedRunningTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  Result initResult = ResumeInternal(_robot);
  if ( initResult != RESULT_OK ) {
    _isRunning = false;
  }
  return initResult;
}


IBehavior::Status IBehavior::Update()
{
  ASSERT_NAMED(IsRunning(), "IBehavior::UpdateNotRunning");
  return UpdateInternal(_robot);
}

void IBehavior::Stop()
{
  PRINT_CH_INFO("Behaviors", (GetName() + ".Stop").c_str(), "Stopping...");

  _isRunning = false;
  StopInternal(_robot);
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StopActing(false);
}

bool IBehavior::IsRunnable(const Robot& robot) const
{
  // first check the unlock
  if ( _requiredUnlockId != UnlockId::Count )
  {
    // ask progression component if the unlockId is currently unlocked
    const ProgressionUnlockComponent& progressionComp = robot.GetProgressionUnlockComponent();
    const bool isUnlocked = progressionComp.IsUnlocked( _requiredUnlockId );
    if ( !isUnlocked ) {
      return false;
    }
  }
  
  // if there's a timer requiring a recent drive off the charger, check with whiteboard
  const bool requiresRecentDriveOff = FLT_GE(_requiredRecentDriveOffCharger_sec, 0.0f);
  if ( requiresRecentDriveOff )
  {
    const float lastDriveOff = robot.GetBehaviorManager().GetWhiteboard().GetTimeAtWhichRobotGotOffCharger();
    const bool hasDrivenOff = FLT_GE(lastDriveOff, 0.0f);
    if ( !hasDrivenOff ) {
      // never driven off the charger, can't run
      return false;
    }
    
    const float curTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
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
    return isSwitchRecent;
  }
  
  //check if the behavior runs while in the air
  if(robot.IsPickedUp() && !ShouldRunWhilePickedUp()){
    return false;
  }
  
  //check if the behavior can handle holding a block
  if(robot.IsCarryingObject() && !CarryingObjectHandledInternally()){
    return false;
  }

  // no unlock or unlock passed, ask subclass
  const bool isRunnable = IsRunnableInternal(robot);
  return isRunnable;
}

Util::RandomGenerator& IBehavior::GetRNG() const {
  return _robot.GetRNG();
}

void IBehavior::SetDefaultName(const char* inName)
{
  if (_name == kBaseDefaultName) {
    _name = inName;
  }
}

IBehavior::Status IBehavior::UpdateInternal(Robot& robot)
{
  if( IsActing() ) {
    return Status::Running;
  }

  return Status::Complete;
}
  
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

Result IBehavior::ResumeInternal(Robot& robot)
{
  // by default, if we are runnable again, initialize and start over
  Result resumeResult = RESULT_FAIL;
  if ( IsRunnable(robot) ) {
    resumeResult = InitInternal(robot);
  }
  return resumeResult;
}
   
// EvaluateScoreInternal is virtual and can optionally be overriden by subclasses
float IBehavior::EvaluateScoreInternal(const Robot& robot) const
{
  float score = _flatScore;
  if ( !_moodScorer.IsEmpty() ) {
    score = _moodScorer.EvaluateEmotionScore(robot.GetMoodManager());
  }
  return score;
}

// EvaluateScoreInternal is virtual and can optionally be overriden by subclasses
float IBehavior::EvaluateRunningScoreInternal(const Robot& robot) const
{
  // unless specifically overriden it should mimic the non-running score
  const float nonRunningScore = EvaluateScoreInternal(robot);
  return nonRunningScore;
}
  
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
  
float IBehavior::EvaluateScore(const Robot& robot) const
{
  if (IsRunnable(robot) || IsRunning())
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
      const float repetitionPenalty = EvaluateRepetitionPenalty();
      score *= repetitionPenalty;
    }
      
    return score;
  }
    
  return 0.0f;
}

bool IBehavior::StartActing(IActionRunner* action, RobotCompletedActionCallback callback)
{
  if( ! IsRunning() ) {
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
  _extraRunningScore = 0.0f;
  _robot.GetActionList().QueueActionNow(action);
  return true;
}

bool IBehavior::StartActing(IActionRunner* action, ActionResultCallback callback)
{
  return StartActing(action,
                     [callback](const ExternalInterface::RobotCompletedAction& msg) {
                       callback(msg.result);
                     });
}

bool IBehavior::StartActing(IActionRunner* action, SimpleCallback callback)
{
  return StartActing(action, [callback](ActionResult ret){ callback(); });
}

bool IBehavior::StartActing(IActionRunner* action, SimpleCallbackWithRobot callback)
{
  return StartActing(action, [this, callback](ActionResult ret){ callback(_robot); });
}

void IBehavior::HandleActionComplete(const ExternalInterface::RobotCompletedAction& msg)
{
  if( IsActing() && msg.idTag == _lastActionTag ) {
    _lastActionTag = ActionConstants::INVALID_TAG;
    _extraRunningScore = 0.0f;

    if( IsRunning() && _actingCallback ) {
      _actingCallback(msg);
    }
  }
}

void IBehavior::IncreaseScoreWhileActing(float extraScore)
{
  if( IsActing() ) {
    _extraRunningScore += extraScore;
  }
}

bool IBehavior::StopActing(bool allowCallback)
{
  _extraRunningScore = 0.0f;

  if( IsActing() ) {
    u32 tagToCancel = _lastActionTag;
      
    if( ! allowCallback ) {
      // if we want to block the callback, clear the tag here, before the cancel
      _lastActionTag = ActionConstants::INVALID_TAG;
    }
      
    bool ret = _robot.GetActionList().Cancel( tagToCancel );

    // note that the callback, if there was one (and it was allowed to run), should have already been called
    // at this point, so it's safe to clear the tag. Also, if the cancel itself failed, that is probably a
    // bug, but somehow the action is gone, so no sense keeping the tag around (and it clearly isn't
    // running)
    _lastActionTag = ActionConstants::INVALID_TAG;
    return ret;
  }

  return false;
}
  
void IBehavior::BehaviorObjectiveAchieved()
{
  //TODO: SEND das event and notify Sparks Goal Chooser about successful completions
  PRINT_CH_INFO("Behaviors", "IBehavior.BehaviorObjectiveAchieved", "Behavior:%s", GetName().c_str());
}


ActionResult IBehavior::UseSecondClosestPreActionPose(DriveToObjectAction* action,
                                                      ActionableObject* object,
                                                      std::vector<Pose3d>& possiblePoses,
                                                      bool& alreadyInPosition)
{
  action->GetPossiblePoses(object, possiblePoses, alreadyInPosition);
  
  for(auto iter = possiblePoses.begin(); iter != possiblePoses.end(); )
  {
    if(iter->IsSameAs(_robot.GetPose(), kSamePreactionPoseDistThresh_mm,
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


#pragma mark --- IReactionaryBehavior ----
  
IReactionaryBehavior::IReactionaryBehavior(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
  , _reactionIsLiftTrackLocked(false)
{
  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTags({
    GameToEngineTag::RequestEnableReactionaryBehavior
  });
  LoadConfig(robot, config);
}

Result IReactionaryBehavior::InitInternal(Robot& robot)
{
  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::ReactionaryBehaviorTransition>(GetType(), true);
  robot.AbortAll();
  
  if(robot.GetMoveComponent().AreAnyTracksLocked((u8)AnimTrackFlag::ALL_TRACKS) &&
     !robot.GetMoveComponent().IsDirectDriving())
  {
    PRINT_NAMED_WARNING("IReactionaryBehavior.InitInternal",
                        "Some tracks are locked, unlocking them");
    robot.GetMoveComponent().CompletelyUnlockAllTracks();
  }
  
  // Reactionary behaviors will prevent DirectDrive messages and external action queueing messages
  // from doing anything
  robot.GetMoveComponent().IgnoreDirectDriveMessages(true);
  robot.GetContext()->GetRobotManager()->GetRobotEventHandler().IgnoreExternalActions(true);
  
  //Prevent reactionary behaviors from moving lift track if carrying block
  if(robot.IsCarryingObject()){
    robot.GetMoveComponent().LockTracks((u8)AnimTrackFlag::LIFT_TRACK);
    _reactionIsLiftTrackLocked = true;
  }
  
  return InitInternalReactionary(robot);
}
  
Result IReactionaryBehavior::ResumeInternal(Robot& robot)
{
  //Never Called - reactionary behaviors don't resume
  ASSERT_NAMED(false, "IReactionaryBehavior.ResumeInternal - reactionary behaviors should not resume");
  return Result::RESULT_OK;
}

void IReactionaryBehavior::StopInternal(Robot& robot)
{
  robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::ReactionaryBehaviorTransition>(GetType(), false);
  robot.GetMoveComponent().IgnoreDirectDriveMessages(false);
  robot.GetContext()->GetRobotManager()->GetRobotEventHandler().IgnoreExternalActions(false);
  
  //Allow lift to move now that reaction has finished playing
  if(_reactionIsLiftTrackLocked){
    _reactionIsLiftTrackLocked = false;
    robot.GetMoveComponent().UnlockTracks((u8)AnimTrackFlag::LIFT_TRACK);
  }
  
  StopInternalReactionary(robot);
}
  
void IReactionaryBehavior::LoadConfig(Robot& robot, const Json::Value& config)
{
  _isDisabledByDefault = config.get(kDisableReactionaryDefault, false).asBool();

}
  
void IReactionaryBehavior::HandleDisableByDefault(Robot& robot)
{
  if(_isDisabledByDefault) {
    PRINT_NAMED_DEBUG("IReactionaryBehavior.LoadConfig.DisableBehavior",
                      "Reactionary Behavior %s is being disabled", GetName().c_str());
    
    std::string id = "default_disabled";
    robot.GetBehaviorManager().RequestEnableReactionaryBehavior(id, GetType(), false);
  }
  
}


void IReactionaryBehavior::SubscribeToTriggerTags(std::set<EngineToGameTag>&& tags)
{
  _engineToGameTags.insert(tags.begin(), tags.end());
  SubscribeToTags(std::move(tags));
}
  
void IReactionaryBehavior::SubscribeToTriggerTags(std::set<GameToEngineTag>&& tags)
{
  _gameToEngineTags.insert(tags.begin(), tags.end());
  SubscribeToTags(std::move(tags));
}
  
void IReactionaryBehavior::SubscribeToTriggerTags(const uint32_t robotId, std::set<RobotInterface::RobotToEngineTag>&& tags)
{
  _robotToEngineTags.insert(tags.begin(), tags.end());
  SubscribeToTags(robotId, std::move(tags));
}

void IReactionaryBehavior::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  //Currently no default behavior, using Always Handle Internal for consistency
  AlwaysHandleInternal(event, robot);
  
}
  
void IReactionaryBehavior::AlwaysHandle(const GameToEngineEvent& event, const Robot& robot)
{
  //Turn off reactionary behaviors based on name
  GameToEngineTag tag = event.GetData().GetTag();
  
  if (tag == GameToEngineTag::RequestEnableReactionaryBehavior){
    std::string requesterID = event.GetData().Get_RequestEnableReactionaryBehavior().requesterID;
    BehaviorType behaviorRequest = event.GetData().Get_RequestEnableReactionaryBehavior().behavior;
    bool enable = event.GetData().Get_RequestEnableReactionaryBehavior().enable;
    
    BehaviorType behaviorType = GetType();

    if(behaviorType == behaviorRequest){
      UpdateDisableIDs(requesterID, enable);
    }
    
  }else{
    AlwaysHandleInternal(event, robot);
  }
}
  
  
void IReactionaryBehavior::AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot)
{
  //Currently no default behavior, using Always Handle Internal for consistency
  AlwaysHandleInternal(event, robot);
}
  
void IReactionaryBehavior::UpdateDisableIDs(std::string& requesterID, bool enable)
{
  if(enable){
    int countRemoved = (int)_disableIDs.erase(requesterID);
    if(!countRemoved){
      PRINT_NAMED_WARNING("BehaviorInterface.ReactionaryBehavior.UpdateDisableIDs",
                          "Attempted to enable reactionary behavior with invalid ID");
    }
    
  }else{
    int countInList = (int)_disableIDs.count(requesterID);
    if(countInList){
      PRINT_NAMED_WARNING("BehaviorInterface.ReactionaryBehavior.UpdateDisableIDs",
                          "Attempted to disable reactionary behavior with ID previously registered");
    }else{
      _disableIDs.insert(requesterID);
    }

  }
  
}
  
bool IReactionaryBehavior::IsRunnableInternal(const Robot& robot) const
{
  return IsReactionEnabled() && IsRunnableInternalReactionary(robot);
}

  
} // namespace Cozmo
} // namespace Anki
