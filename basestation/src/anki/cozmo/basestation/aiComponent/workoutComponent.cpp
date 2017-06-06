/**
 * File: workoutComponent.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-11-07
 *
 * Description: AI component to handle cozmo's "workouts". holds definitions of various workouts and tracks
 *              which ones he should be doing
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/aiComponent/workoutComponent.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/events/animationTriggerHelpers.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
CONSOLE_VAR_RANGED(double, kEightiesWorkoutMusicProbability, "WorkoutComponent",  0.1, 0.0, 1.0); // probability that 80's music will be played during 'strong' workout

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WorkoutConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Result WorkoutConfig::InitConfiguration(const Json::Value& config)
{
  using namespace JsonTools;

  // default value will be "Count", which means "don't play the animation"
  GetValueOptional(config, "preLiftAnim", preLiftAnim);
  GetValueOptional(config, "postLiftAnim", postLiftAnim);
  GetValueOptional(config, "transitionAnim", transitionAnim);
  GetValueOptional(config, "putDownAnim", putDownAnim);

  // lift anims are required
  if( ! GetValueOptional(config, "strongLiftAnim", strongLiftAnim) ) {
    PRINT_NAMED_ERROR("WorkoutConfig.ParseJson.InvalidStrongLift",
                      "Could not parse strong lift from json");
    return RESULT_FAIL;
  }  
  if( ! GetValueOptional(config, "weakLiftAnim", weakLiftAnim) ) {
    PRINT_NAMED_ERROR("WorkoutConfig.ParseJson.InvalidWeakLift",
                      "Could not parse weak lift from json");
    return RESULT_FAIL;
  }

  const Json::Value& numStrongLiftsJson = config["numStrongLifts"];
  if (!numStrongLiftsJson.isNull())
  {
    _numStrongLiftScorer.ReadFromJson(numStrongLiftsJson);
  }
  else {
    PRINT_NAMED_ERROR("WorkoutConfig.ParseJson.NoNumStrongLifts",
                      "Must specify a mood scoring json for number of strong lifts to do");
    return RESULT_FAIL;
  }

  const Json::Value& numWeakLiftsJson = config["numWeakLifts"];
  if (!numWeakLiftsJson.isNull())
  {
    _numWeakLiftScorer.ReadFromJson(numWeakLiftsJson);
  }
  else {
    PRINT_NAMED_ERROR("WorkoutConfig.ParseJson.NoNumWeakLifts",
                      "Must specify a mood scoring json for number of weak lifts to do");
    return RESULT_FAIL;
  }

  GetValueOptional(config, "emotionEventOnComplete", _emotionEventOnComplete);
  
  _additionalBehaviorObjectiveOnComplete = BehaviorObjective::Count;
  std::string additionalbehaviorObjectiveOnCompleteStr;
  if (GetValueOptional(config, "additionalBehaviorObjectiveOnComplete", additionalbehaviorObjectiveOnCompleteStr)) {
    _additionalBehaviorObjectiveOnComplete = BehaviorObjectiveFromString(additionalbehaviorObjectiveOnCompleteStr.c_str());
  }
  
  
  return RESULT_OK;
}

unsigned int WorkoutConfig::GetNumStrongLifts(const Robot& robot) const
{
  return MoodScoreHelper(robot, _numStrongLiftScorer);
}

unsigned int WorkoutConfig::GetNumWeakLifts(const Robot& robot) const
{
  return MoodScoreHelper(robot, _numWeakLiftScorer);
}
  
unsigned int WorkoutConfig::MoodScoreHelper(const Robot& robot, const MoodScorer& moodScorer)
{
  if( moodScorer.IsEmpty() ) {
    return 0;
  }

  float moodScore = moodScorer.EvaluateEmotionScore(robot.GetMoodManager());

  const unsigned int num = Util::numeric_cast<unsigned int>(std::round(moodScore));
  return num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WorkoutComponent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WorkoutComponent::WorkoutComponent(Robot& robot)
  : _robot(robot)
{
  _currWorkout = _workouts.end();
}

Result WorkoutComponent::InitConfiguration(const Json::Value& config)
{  
  for( const auto& workoutJson : config["workouts"] ) {
    WorkoutConfig workout;
    Result res = workout.InitConfiguration( workoutJson );
    if( res != RESULT_OK ) {
      return res;
    }
    _workouts.emplace_back( std::move( workout ) );
  }

  PRINT_CH_INFO("Behaviors", "WorkoutComponent.Init",
                "Loaded %zu workouts", _workouts.size());
  
  if( _workouts.empty() ) {
    return RESULT_FAIL;
  }
  else {
    _currWorkout = _workouts.begin();
    return RESULT_OK;
  }
}

const WorkoutConfig& WorkoutComponent::GetCurrentWorkout() const
{
  // there should always be a workout
  DEV_ASSERT(_currWorkout != _workouts.end(), "WorkoutComponent.GetCurrentWorkout.NoWorkout");
  return *_currWorkout;
}

void WorkoutComponent::CompleteCurrentWorkout()
{
  if( _currWorkout != _workouts.end() ) {
    const bool hasEmotionEvent = !_currWorkout->_emotionEventOnComplete.empty();
    if( hasEmotionEvent ) {
      _robot.GetMoodManager().TriggerEmotionEvent(_currWorkout->_emotionEventOnComplete,
                                                  MoodManager::GetCurrentTimeInSeconds());
    }

    if( _currWorkout != --(_workouts.end()) ) {
      ++_currWorkout;
    }
    // else curr workout stays on the last element
  }
}

bool WorkoutComponent::ShouldPlayEightiesMusic()
{
  if (!_hasComputedIfEightiesMusicShouldPlay) {
    // 80's music should only be played if this is a strong workout, and it should be
    //   pseudo-random with the configured probability.
    _shouldPlayEightiesMusic = (_currWorkout->GetNumStrongLifts(_robot) > 0) &&
                               (_robot.GetRNG().RandDbl() < kEightiesWorkoutMusicProbability);
    
    _hasComputedIfEightiesMusicShouldPlay = true;
  }

  return _shouldPlayEightiesMusic;
}

}
}
