/**
 * File: iAIGoalStrategy
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Interface for goal strategies (when goals become active/inactive)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "iAIGoalStrategy.h"

#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

namespace Anki {
namespace Cozmo {

namespace {
CONSOLE_VAR(float, kDefaultGoalMaxDurationSecs, "IAIGoalStrategy", 60.0f);
static const char* kStartMoodScorerConfigKey = "startMoodScorer";
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAIGoalStrategy::IAIGoalStrategy(const Json::Value& config)
: _minDurationSecs(-1.0f)
, _maxDurationSecs(-1.0f)
, _cooldownSecs(-1.0f)
, _startMoodScorer(nullptr)
, _requiredMinStartMoodScore(-1.0f)
{
  using namespace JsonTools;
  
  // set a common default for max duration instead of forcing every goal to define its own
  _maxDurationSecs = kDefaultGoalMaxDurationSecs;
  
  // timers
  GetValueOptional(config, "minDurationSecs", _minDurationSecs);
  GetValueOptional(config, "maxDurationSecs", _maxDurationSecs);
  GetValueOptional(config, "cooldownSecs", _cooldownSecs);
  
  const bool hasRequiredMinStartMoodScore =
    GetValueOptional(config, "requiredMinStartMoodScore", _requiredMinStartMoodScore);
  
  // read mood scorer data depending on requiredMinStart
  if ( hasRequiredMinStartMoodScore )
  {
    // if requiredMinStart is set, create and load a mood scorer
    _startMoodScorer.reset( new MoodScorer() );
    const Json::Value& moodScorerConfig = config[kStartMoodScorerConfigKey];
    const bool moodScoreJsonOk = _startMoodScorer->ReadFromJson(moodScorerConfig);
    ASSERT_NAMED_EVENT(moodScoreJsonOk,
      "IAIGoalStrategy.MoodScorerConfig",
      "Json config for %s in this strategy is not valid.",
      kStartMoodScorerConfigKey);
  }
  else
  {
    // value is not set, make sure there's no mood scorer data set, this would mean they forgot to set the required score
    ASSERT_NAMED_EVENT(config[kStartMoodScorerConfigKey].isNull(),
      "IAIGoalStrategy.MoodScorerConfig",
      "Strategy does not specify requiredMinStartMoodScore, but does have %s (which would be ignored)",
      kStartMoodScorerConfigKey);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IAIGoalStrategy::~IAIGoalStrategy()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAIGoalStrategy::WantsToStart(const Robot& robot, float lastTimeGoalRanSec) const
{
  // check cooldown if set, and if the goal ever ran
  if ( FLT_GT(_cooldownSecs, 0.0f) && FLT_GT(lastTimeGoalRanSec, 0.0f) )
  {
    const float inCooldownUntilSecs = lastTimeGoalRanSec + _cooldownSecs;
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const bool isCoolingDown = curTimeSecs < inCooldownUntilSecs;
    if ( isCoolingDown ) {
      return false;
    }
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
  
  // not cooling down, delegate
  const bool ret = WantsToStartInternal(robot, lastTimeGoalRanSec);
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool IAIGoalStrategy::WantsToEnd(const Robot& robot, float lastTimeGoalStartedSec) const
{
  // if we started recently and we have a min duration, this goal doesn't wanna end yet
  const bool hasMinDuration = FLT_GT(_minDurationSecs, 0.0f);
  if ( hasMinDuration ) {
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float runAtLeastUntil = lastTimeGoalStartedSec + _minDurationSecs;
    const bool isWithinMinDuration = FLT_LT(curTimeSecs, runAtLeastUntil);
    if ( isWithinMinDuration ) {
      return false; // doesn't wanna end because it's hasn't run enough yet
    }
  }
  
  // check if we have been running for too long
  const bool hasMaxDuration = FLT_GT(_maxDurationSecs, 0.0f);
  if ( hasMaxDuration ) {
    const float curTimeSecs = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const float runAtMostUntil = lastTimeGoalStartedSec + _maxDurationSecs;
    const bool isBeyondMaxDuration = FLT_GT(curTimeSecs, runAtMostUntil);
    if ( isBeyondMaxDuration ) {
      return true; // wantsToEnd because it's beyond max
    }
  }

  // not in min duration anymore (or any set), delegate
  const bool ret = WantsToEndInternal(robot, lastTimeGoalStartedSec);
  return ret;
}
  
} // namespace
} // namespace
