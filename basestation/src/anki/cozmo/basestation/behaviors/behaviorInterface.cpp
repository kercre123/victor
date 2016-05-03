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
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"

#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include "util/math/numericCast.h"

namespace Anki {
namespace Cozmo {

// Static initializations
const char* IBehavior::kBaseDefaultName = "no_name";
  
static const char* kNameKey              = "name";
static const char* kEmotionScorersKey    = "emotionScorers";
static const char* kFlatScoreKey         = "flatScore";
static const char* kRepetitionPenaltyKey = "repetitionPenalty";
static const char* kBehaviorGroupsKey    = "behaviorGroups";
  
  
IBehavior::IBehavior(Robot& robot, const Json::Value& config)
  : _moodScorer()
  , _flatScore(0.0f)
  , _robot(robot)
  , _startedRunningTime_s(0.0)
  , _lastRunTime_s(0.0)
  , _overrideScore(-1.0f)
  , _extraRunningScore(0.0f)
  , _isRunning(false)
  , _isOwnedByFactory(false)
  , _isChoosable(true)
  , _enableRepetitionPenalty(true)
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
      this->HandleEvent(event);
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
      this->HandleEvent(event);
    };
      
    for(auto tag : tags) {
      _eventHandles.push_back(_robot.GetExternalInterface()->Subscribe(tag, handlerCallback));
    }
  }
}

Result IBehavior::Init()
{
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
  _isRunning = false;
  StopInternal(_robot);
  _lastRunTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  StopActing(false);
}

Util::RandomGenerator& IBehavior::GetRNG() const {
  static Util::RandomGenerator sRNG;
  return sRNG;
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
  float score = _flatScore;
  if ( !_moodScorer.IsEmpty() ) {
    score = _moodScorer.EvaluateEmotionScore(robot.GetMoodManager());
  }
  return score;
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
  
float IBehavior::EvaluateScore(const Robot& robot) const
{
  if (IsRunnable(robot) || IsRunning())
  {
    const bool doOverrideScore = (_overrideScore >= 0.0f);
    const bool isRunning = IsRunning();
                
    float score = 0.0f;
      
    if (doOverrideScore)
    {
      score = _overrideScore;
    }
    else if (isRunning)
    {
      score = EvaluateRunningScoreInternal(robot) + _extraRunningScore;
    }
    else
    {
      score = EvaluateScoreInternal(robot);
    }
      
    // no repetition penalty when running
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

#pragma mark --- IReactionaryBehavior ----
  
IReactionaryBehavior::IReactionaryBehavior(Robot& robot, const Json::Value& config)
  : IBehavior(robot, config)
{
  SetBehaviorGroup(BehaviorGroup::Reactionary, true);
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
  
  
} // namespace Cozmo
} // namespace Anki
