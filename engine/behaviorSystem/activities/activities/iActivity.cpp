/**
 * File: IActivity
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Activities provide persistence of cozmo's character and game related
 * world state across behaviors.  They may set lights/music and select what behavior
 * should be running each tick
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "engine/behaviorSystem/activities/activities/iActivity.h"

#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/audio/robotAudioClient.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/behaviorSystem/activities/activityStrategies/activityStrategyFactory.h"
#include "engine/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"
#include "engine/behaviorSystem/behaviorChoosers/behaviorChooserFactory.h"
#include "engine/behaviorSystem/behaviorChoosers/iBehaviorChooser.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/reactionTriggerStrategies/reactionTriggerHelpers.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/cubeLightComponent.h"
#include "engine/components/publicStateBroadcaster.h"
#include "engine/cozmoContext.h"
#include "engine/drivingAnimationHandler.h"
#include "engine/events/animationTriggerHelpers.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"


#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace {
static const char* kActivityIDConfigKey               = "activityID";
static const char* kActivityTypeConfigKey             = "activityType";
static const char* kNeedsActionIDKey                  = "needsActionID";
static const char* kBehaviorChooserConfigKey          = "behaviorChooser";
static const char* kInterludeBehaviorChooserConfigKey = "interludeBehaviorChooser";
static const char* kStrategyConfigKey                 = "activityStrategy";
static const char* kRequiresSparkKey                  = "requireSpark";
static const char* kSmartReactionLockSuffix           = "_activityLock";
static const std::string kIdleLockPrefix              = "Activity_";

} // end namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivity::IActivity(Robot& robot, const Json::Value& config)
: _driveStartAnimTrigger(AnimationTrigger::Count)
, _driveLoopAnimTrigger(AnimationTrigger::Count)
, _driveEndAnimTrigger(AnimationTrigger::Count)
, _idleAnimTrigger(AnimationTrigger::Count)
, _infoAnalysisProcess(AIInformationAnalysis::EProcess::Invalid)
, _requiredSpark(UnlockId::Count)
, _hasSetIdle(false)
, _lastTimeActivityStartedSecs(-1.0f)
, _lastTimeActivityStoppedSecs(-1.0f)
{
  ReadConfig(robot, config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivity::~IActivity()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityID IActivity::ExtractActivityIDFromConfig(const Json::Value& config,
                                                  const std::string& fileName)
{
  const std::string debugName = "ActivityID.NoActivityIdSpecified";
  const std::string activityID_str = JsonTools::ParseString(config, kActivityIDConfigKey, debugName);
  
  // To make it easy to find activities, assert that the file name and activityID match
  if(ANKI_DEV_CHEATS && !fileName.empty()){
    std::string jsonFileName = Util::FileUtils::GetFileName(fileName);
    // remove extension
    auto dotIndex = jsonFileName.find_last_of(".");
    std::string lowerFileName = dotIndex == std::string::npos ? jsonFileName : jsonFileName.substr(0, dotIndex);
    std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);
    
    std::string activityIDLower = activityID_str;
    std::transform(activityIDLower.begin(), activityIDLower.end(), activityIDLower.begin(), ::tolower);
    DEV_ASSERT_MSG(activityIDLower == lowerFileName,
                   "RobotDataLoader.LoadActivities.ActivityIDFileNameMismatch",
                   "File name %s does not match BehaviorID %s",
                   fileName.c_str(),
                   activityID_str.c_str());
  }
  
  return ActivityIDFromString(activityID_str);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityType IActivity::ExtractActivityTypeFromConfig(const Json::Value& config)
{
  std::string activityTypeJSON;
  JsonTools::GetValueOptional(config, kActivityTypeConfigKey, activityTypeJSON);
  return ActivityTypeFromString(activityTypeJSON);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeedsActionId IActivity::ExtractNeedsActionIDFromConfig(const Json::Value& config)
{
  std::string needsActionIDString;
  if (JsonTools::GetValueOptional(config, kNeedsActionIDKey, needsActionIDString))
  {
    return NeedsActionIdFromString(needsActionIDString);
  }
  else
  {
    return NeedsActionId::NoAction;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::ReadConfig(Robot& robot, const Json::Value& config)
{
  // read info from config
  _id = ExtractActivityIDFromConfig(config);

  _needsActionId = ExtractNeedsActionIDFromConfig(config);
  
  // - - - - - - - - - -
  // Needs Sparks
  // - - - - - - - - - -
  
  std::string sparkString;
  if( JsonTools::GetValueOptional(config,kRequiresSparkKey,sparkString) )
  {
    _requiredSpark = UnlockIdFromString(sparkString.c_str());
  }
  
  // driving animation triggers
  std::string animTriggerStr;
  if ( JsonTools::GetValueOptional(config, "driveStartAnimTrigger", animTriggerStr) ) {
    _driveStartAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  if ( JsonTools::GetValueOptional(config, "driveLoopAnimTrigger", animTriggerStr) ) {
    _driveLoopAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  if ( JsonTools::GetValueOptional(config, "driveEndAnimTrigger", animTriggerStr) ) {
    _driveEndAnimTrigger = animTriggerStr.empty() ? AnimationTrigger::Count : AnimationTriggerFromString(animTriggerStr.c_str());
  }

  if ( JsonTools::GetValueOptional(config, "idleAnimTrigger", animTriggerStr) ) {
    _idleAnimTrigger = animTriggerStr.empty()
      ? AnimationTrigger::Count
      : AnimationTriggerFromString(animTriggerStr.c_str());
  }
  
  #if (DEV_ASSERT_ENABLED)
    {
      // check that triggers are all or nothing
      const bool hasAnyDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) ||
      (_driveLoopAnimTrigger  != AnimationTrigger::Count) ||
      (_driveEndAnimTrigger   != AnimationTrigger::Count);
      const bool hasAllDrivingAnim = (_driveStartAnimTrigger != AnimationTrigger::Count) &&
      (_driveLoopAnimTrigger  != AnimationTrigger::Count) &&
      (_driveEndAnimTrigger   != AnimationTrigger::Count);
      DEV_ASSERT(hasAllDrivingAnim || !hasAnyDrivingAnim, "IActivity.Init.InvalidDrivingAnimTriggers_AllOrNothing");
    }
  #endif
  
  // information analyzer process
  std::string inanProcessStr;
  JsonTools::GetValueOptional(config, "infoAnalyzerProcess", inanProcessStr);
  _infoAnalysisProcess = inanProcessStr.empty() ?
  AIInformationAnalysis::EProcess::Invalid :
  AIInformationAnalysis::EProcessFromString(inanProcessStr.c_str());
  
  // configure chooser and set in pointer if specified
  // otherwise if ChooseNextBehaviorInternal hasn't been overridden assert should hit below
  _behaviorChooserPtr.reset();
  const Json::Value& chooserConfig = config[kBehaviorChooserConfigKey];
  if(!chooserConfig.isNull()){
    IBehaviorChooser* newChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, chooserConfig);
    _behaviorChooserPtr.reset( newChooser );
  }

  // configure the interlude behavior chooser, to specify behaviors that can run in between other behaviors.
  _interludeBehaviorChooserPtr.reset();
  const Json::Value& interludeChooserConfig = config[kInterludeBehaviorChooserConfigKey];
  if( !interludeChooserConfig.isNull() ) {
    IBehaviorChooser* newChooser = BehaviorChooserFactory::CreateBehaviorChooser(robot, interludeChooserConfig);
    _interludeBehaviorChooserPtr.reset(newChooser);
  }
    
  // strategy
  const Json::Value& strategyConfig = config[kStrategyConfigKey];
  IActivityStrategy* newStrategy = ActivityStrategyFactory::CreateActivityStrategy(robot, strategyConfig);
  _strategy.reset( newStrategy );
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::OnSelected(Robot& robot)
{
  _lastTimeActivityStartedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_behaviorChooserPtr.get() != nullptr){
    _behaviorChooserPtr->OnSelected();
  }
  
  // Update Activity state for the PublicStateBroadcaster
  // Note: This only applies to freeplay goals, spark goals do nothing
  robot.GetPublicStateBroadcaster().UpdateActivity(_id);
  
  // set driving animations for this activity if specified in config
  const bool hasDrivingAnims = HasDrivingAnimTriggers();
  if ( hasDrivingAnims ) {
    robot.GetDrivingAnimationHandler().PushDrivingAnimations(
      {_driveStartAnimTrigger,
       _driveLoopAnimTrigger,
       _driveEndAnimTrigger},
        GetIDStr());
  }
  
  _hasSetIdle = false;
  // Set idle animation if desired
  if( _idleAnimTrigger != AnimationTrigger::Count ) {
    SmartPushIdleAnimation(robot, _idleAnimTrigger);
  }
  
  // request analyzer process
  if ( _infoAnalysisProcess != AIInformationAnalysis::EProcess::Invalid ) {
    robot.GetAIComponent().GetAIInformationAnalyzer().AddEnableRequest(_infoAnalysisProcess, GetIDStr());
  }
  
  
  // log event to das - note freeplay_goal is a legacy name for Activities left
  // in place so that data is queriable - please do not change
  Util::sEventF("robot.freeplay_goal_started", {}, "%s", ActivityIDToString(_id));
  OnSelectedInternal(robot);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::OnDeselected(Robot& robot)
{
  _lastTimeActivityStoppedSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(_behaviorChooserPtr.get() != nullptr){
    _behaviorChooserPtr->OnDeselected();
  }

  // clear idle if it was set
  if( _idleAnimTrigger != AnimationTrigger::Count ) {
    // NOTE: assumes _idleAnimTrigger doesn't change while this was running
    robot.GetAnimationStreamer().RemoveIdleAnimation(GetIDStr());
  }      
  
  // clear driving animations for this activity if specified in config
  const bool hasDrivingAnims = HasDrivingAnimTriggers();
  if ( hasDrivingAnims ) {
    robot.GetDrivingAnimationHandler().RemoveDrivingAnimations(GetIDStr());
  }
  
  // (un)request analyzer process
  if ( _infoAnalysisProcess != AIInformationAnalysis::EProcess::Invalid ) {
    robot.GetAIComponent().GetAIInformationAnalyzer().RemoveEnableRequest(_infoAnalysisProcess, GetIDStr());
  }
  
  // remove any locks set through SmartDisable
  while(!_smartLockIDs.empty()){
    SmartRemoveDisableReactionsLock(robot, *_smartLockIDs.begin());
  }
  _smartLockIDs.clear();
  
  if(_hasSetIdle){
    SmartRemoveIdleAnimation(robot);
  }

  // clear the interlude behavior, if it was set
  _lastChosenInterludeBehavior = nullptr;
  
  // We're changing what cozmo's doing at a high level, so we don't want to
  // communicate the sparks reward to the user, just pretend we have
  if(robot.GetContext()->GetNeedsManager()->IsPendingSparksRewardMsg()){
    robot.GetContext()->GetNeedsManager()->SparksRewardCommunicatedToUser();
  }
  
  
  // log event to das
  int nSecs = Util::numeric_cast<int>(_lastTimeActivityStoppedSecs - _lastTimeActivityStartedSecs);
  if (nSecs < 0) { // Attempt to fix COZMO-7862
    PRINT_NAMED_ERROR("IActivity.Exit.NegativeDuration",
                      "Negative duration (%i secs, started at %f and stopped at %f",
                      nSecs, _lastTimeActivityStartedSecs, _lastTimeActivityStoppedSecs);
    nSecs = 0;
  }
  
  // log event to das - note freeplay_goal is a legacy name for Activities left
  // in place so that data is queriable - please do not change
  Util::sEventF("robot.freeplay_goal_ended",
                {{DDATA, std::to_string(nSecs).c_str()}},
                "%s", ActivityIDToString(_id));
  
  OnDeselectedInternal(robot);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr IActivity::ChooseNextBehavior(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{

  // if an intlude behavior was chosen and is still running, return that
  if( _lastChosenInterludeBehavior != nullptr &&
      _lastChosenInterludeBehavior == currentRunningBehavior ) {
    return _lastChosenInterludeBehavior;
  }
  else {
    _lastChosenInterludeBehavior.reset();
  }
  
  IBehaviorPtr ret = ChooseNextBehaviorInternal(robot, currentRunningBehavior);

  const bool hasInterludeChooser = _interludeBehaviorChooserPtr != nullptr;
  const bool switchingBehaviors = ret != currentRunningBehavior;
  
  if( hasInterludeChooser && switchingBehaviors ) {
    // if we are changing behaviors, give the interlude choose an opportunity to run something. Pass in the
    // behavior that we would otherwise choose (The one that would run next) as "current"
    _lastChosenInterludeBehavior = _interludeBehaviorChooserPtr->ChooseNextBehavior(robot, ret);
    if(_lastChosenInterludeBehavior != nullptr) {
      PRINT_CH_INFO("Behaviors", "IActivity.ChooseInterludeBehavior",
                    "Activity %s is inserting interlude %s between behaviors %s and %s",
                    GetIDStr(),
                    _lastChosenInterludeBehavior->GetIDStr().c_str(),
                    currentRunningBehavior ? currentRunningBehavior->GetIDStr().c_str() : "NULL",
                    ret ? ret->GetIDStr().c_str() : "NULL");
      return _lastChosenInterludeBehavior;
    }
    else {
      // set to null to avoid confusing with None (see COZMO-12095)
      _lastChosenInterludeBehavior = nullptr;
    }
  }
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr IActivity::ChooseNextBehaviorInternal(Robot& robot, const IBehaviorPtr currentRunningBehavior)
{
  IBehaviorPtr ret;
  if(ANKI_VERIFY(_behaviorChooserPtr.get() != nullptr,
                 "IActivity.ChooseNextBehaviorInternal.ChooserNotOverwritten",
                 "ChooseNextBehaviorInternal called without behavior chooser overwritten")){
    // at the moment delegate on chooser. At some point we'll have intro/outro and other reactions
    // note we pass
    IBehaviorPtr ret = _behaviorChooserPtr->ChooseNextBehavior(robot, currentRunningBehavior);
    return ret;
  }
  
  return nullptr;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::SmartPushIdleAnimation(Robot& robot, AnimationTrigger animation)
{
  if(ANKI_VERIFY(!_hasSetIdle,
                 "IActivity.SmartPushIdleAnimation.IdleAlreadySet",
                 "Activity %s has already set an idle animation",
                 GetIDStr())){
    robot.GetAnimationStreamer().PushIdleAnimation(animation, kIdleLockPrefix + GetIDStr());
    _hasSetIdle = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::SmartRemoveIdleAnimation(Robot& robot)
{
  if(ANKI_VERIFY(_hasSetIdle,
                 "IActivity.SmartRemoveIdleAnimation.NoIdleSet",
                 "Activity %s is trying to remove an idle, but none is currently set",
                 GetIDStr())){
    robot.GetAnimationStreamer().RemoveIdleAnimation(kIdleLockPrefix + GetIDStr());
    _hasSetIdle = false;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::SmartDisableReactionsWithLock(Robot& robot,
                                              const std::string& lockID,
                                              const TriggersArray& triggers)
{
  robot.GetBehaviorManager().DisableReactionsWithLock(lockID + kSmartReactionLockSuffix, triggers);
  _smartLockIDs.insert(lockID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void IActivity::SmartRemoveDisableReactionsLock(Robot& robot,
                                                const std::string& lockID)
{
  robot.GetBehaviorManager().RemoveDisableReactionsLock(lockID + kSmartReactionLockSuffix);
  _smartLockIDs.erase(lockID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if ANKI_DEV_CHEATS
void IActivity::SmartDisableReactionWithLock(Robot& robot,
                                             const std::string& lockID,
                                             const ReactionTrigger& trigger)
{
  robot.GetBehaviorManager().DisableReactionWithLock(lockID + kSmartReactionLockSuffix, trigger);
  _smartLockIDs.insert(lockID);
}
#endif

  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivityStrategy* IActivity::DevGetStrategy()
{
  if(ANKI_DEV_CHEATS)
  {
    assert(_strategy);
    return _strategy.get();
  }
  return nullptr;
}

} // namespace Cozmo
} // namespace Anki
