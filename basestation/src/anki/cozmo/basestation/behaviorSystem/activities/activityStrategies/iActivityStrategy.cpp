/**
 * File: IActivityStrategy
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Interface for activity strategies (when activities become active/inactive)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"

#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/utils/cozmoFeatureGate.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(float, kDefaultActivityMaxDurationSecs, "IActivityStrategy", 60.0f);
CONSOLE_VAR(float, kShortFailureCooldownSecs, "IActivityStrategy", 3.0f);
static const char* kStartMoodScorerConfigKey      = "startMoodScorer";
static const char* kActivityCanEndConfigKey       = "activityCanEndDurationSecs";
static const char* kActivityShouldEndConfigKey    = "activityShouldEndDurationSecs";
static const char* kActivityCooldownBaseConfigKey = "cooldownBaseSecs";
static const char* kActivityCooldownVarianceKey   = "cooldownVarianceSecs";
static const char* kActivityStartInCooldown       = "startInCooldown";
static const char* kActivityFeatureGate           = "featureGate";

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivityStrategy::IActivityStrategy(const Json::Value& config)
: _activityCanEndSecs(-1.0f)
, _activityShouldEndSecs(-1.0f)
, _baseCooldownSecs(-1.0f)
, _cooldownSecs(-1.0f)
, _cooldownVarianceSecs(0.f)
, _startInCooldown(false)
, _startMoodScorer(nullptr)
, _requiredMinStartMoodScore(-1.0f)
, _requiredRecentOnTreadsEvent_secs(-1.0f)
, _featureGate(FeatureType::Invalid)
{
  using namespace JsonTools;
  
  // set a common default for max duration instead of forcing every activity to define its own
  _activityShouldEndSecs = kDefaultActivityMaxDurationSecs;
  
  // timers
  GetValueOptional(config, kActivityCanEndConfigKey,       _activityCanEndSecs);
  GetValueOptional(config, kActivityShouldEndConfigKey,    _activityShouldEndSecs);
  GetValueOptional(config, kActivityCooldownBaseConfigKey, _baseCooldownSecs);
  GetValueOptional(config, kActivityCooldownVarianceKey,   _cooldownVarianceSecs);
  GetValueOptional(config, kActivityStartInCooldown,       _startInCooldown);
  
  _cooldownSecs = _baseCooldownSecs;
  
  // recent onTreads event
  GetValueOptional(config, "requiredRecentOnTreadsEventSecs", _requiredRecentOnTreadsEvent_secs);
  
  const bool hasRequiredMinStartMoodScore =
    GetValueOptional(config, "requiredMinStartMoodScore", _requiredMinStartMoodScore);
  
  // read mood scorer data depending on requiredMinStart
  if ( hasRequiredMinStartMoodScore )
  {
    // if requiredMinStart is set, create and load a mood scorer
    _startMoodScorer.reset( new MoodScorer() );
    const Json::Value& moodScorerConfig = config[kStartMoodScorerConfigKey];
    const bool moodScoreJsonOk = _startMoodScorer->ReadFromJson(moodScorerConfig);
    DEV_ASSERT_MSG(moodScoreJsonOk,
      "IActivityStrategy.MoodScorerConfig",
      "Json config for %s in this strategy is not valid.",
      kStartMoodScorerConfigKey);
  }
  else
  {
    // value is not set, make sure there's no mood scorer data set, this would mean they forgot to set the required score
    DEV_ASSERT_MSG(config[kStartMoodScorerConfigKey].isNull(),
      "IActivityStrategy.MoodScorerConfig",
      "Strategy does not specify requiredMinStartMoodScore, but does have %s (which would be ignored)",
      kStartMoodScorerConfigKey);
  }
  
  std::string featureGate = "";
  if(GetValueOptional(config, kActivityFeatureGate, featureGate))
  {
    _featureGate = FeatureTypeFromString(featureGate);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IActivityStrategy::~IActivityStrategy()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IActivityStrategy::WantsToStart(const Robot& robot,
                                     float lastTimeActivityRanSec,
                                     float lastTimeActivityStartedSec) const
{
  // If the strategy has a corresponding feature gate and it is not enabled
  // we don't want to start
  if(_featureGate != FeatureType::Invalid &&
     !robot.GetContext()->GetFeatureGate()->IsFeatureEnabled(_featureGate))
  {
    return false;
  }

  // check cooldown if set, and if the activity ever ran
  // or if it has never ran but wants to start in cooldown
  if ( FLT_GT(_cooldownSecs, 0.0f) && (FLT_GT(lastTimeActivityRanSec, 0.0f) || _startInCooldown) )
  {

    // if the last time the activity ran it only lasted a couple ticks, give it a short cooldown. This will
    // prevent it from thrashing back and forth failing (when there are no behaviors to run) over and over,
    // but also won't trigger the potentially much longer normal cooldown for when the activity actually runs
    const float lastActivityRanDurationSecs = lastTimeActivityRanSec - lastTimeActivityStartedSec;
    const float twoTicsSecs = 2.0f * BaseStationTimer::getInstance()->GetTimeSinceLastTickInSeconds();
    const bool lastRunWasVeryShort = (lastActivityRanDurationSecs <= twoTicsSecs) &&
                                     (lastActivityRanDurationSecs > 0);
    
    const float cooldownSecs = lastRunWasVeryShort ? kShortFailureCooldownSecs : _cooldownSecs;
    
    const float inCooldownUntilSecs = lastTimeActivityRanSec + cooldownSecs;
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool isCoolingDown = curTimeSecs < inCooldownUntilSecs;
    
    if ( isCoolingDown ) {
      return false;
    }
    
    // Keep randomizing the cooldown until we are actually in cool down
    RandomizeCooldown(robot);
  }
  
  // check if we created a start scorer
  const bool hasMinScoreToStart = (nullptr != _startMoodScorer.get());
  if ( hasMinScoreToStart ) {
    // if we did, get the value, and if the value is not good enough, do not start
    const float curValue = _startMoodScorer->EvaluateEmotionScore( robot.GetMoodManager() );
    const bool isAboveThreshold = FLT_GE(curValue, _requiredMinStartMoodScore);
    if ( !isAboveThreshold ) {
      return false;
    }
  }
  
  // check if we need a recent OnTreads event
  if ( FLT_GT(_requiredRecentOnTreadsEvent_secs, 0.0f) )
  {
    // check if we recorded an event
    const float lastEventSecs = robot.GetAIComponent().GetWhiteboard().GetTimeAtWhichRobotReturnedToTreadsSecs();
    const bool everFiredEvent = FLT_GT(lastEventSecs, 0.0f);
    if ( !everFiredEvent ) {
      // no event, do not start, since it's required
      return false;
    }
    
    // check if we already started for the last event
    const bool alreadyStartedForEvent = FLT_GE(lastTimeActivityStartedSec, lastEventSecs);
    if ( alreadyStartedForEvent ) {
      // already started after that event, do not do it again
      return false;
    }
    
    // check if the event is recent
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float elapsedSecs = curTimeSecs - lastEventSecs;
    const bool eventIsOld = FLT_GE(elapsedSecs, _requiredRecentOnTreadsEvent_secs);
    if ( eventIsOld ) {
      // event is too old, even if we never fired do not start now
      return false;
    }
    
    // we should start, continue next checks
  }
  
  // not cooling down, delegate
  const bool ret = WantsToStartInternal(robot, lastTimeActivityRanSec);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IActivityStrategy::WantsToEnd(const Robot& robot, float lastTimeActivityStartedSec) const
{
  // if we started recently and we have a min duration, this activity doesn't wanna end yet
  const bool hasMinDuration = FLT_GT(_activityCanEndSecs, 0.0f);
  if ( hasMinDuration ) {
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float runAtLeastUntil = lastTimeActivityStartedSec + _activityCanEndSecs;
    const bool isWithinMinDuration = FLT_LT(curTimeSecs, runAtLeastUntil);
    if ( isWithinMinDuration ) {
      return false; // doesn't wanna end because it's hasn't run enough yet
    }
  }
  
  // check if we have been running for too long
  const bool hasMaxDuration = FLT_GT(_activityShouldEndSecs, 0.0f);
  if ( hasMaxDuration ) {
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float runAtMostUntil = lastTimeActivityStartedSec + _activityShouldEndSecs;
    const bool isBeyondMaxDuration = FLT_GT(curTimeSecs, runAtMostUntil);
    if ( isBeyondMaxDuration ) {
      return true; // wantsToEnd because it's beyond max
    }
  }

  // not in min duration anymore (or any set), delegate
  const bool ret = WantsToEndInternal(robot, lastTimeActivityStartedSec);
  return ret;
}

void IActivityStrategy::RandomizeCooldown(const Robot& robot) const
{
  _cooldownSecs = _baseCooldownSecs + robot.GetRNG().RandDbl(_cooldownVarianceSecs);
}
  
} // namespace
} // namespace
