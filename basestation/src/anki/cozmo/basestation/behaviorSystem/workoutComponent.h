/**
 * File: workoutComponent.h
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

#ifndef __Cozmo_Basestation_BehaviorSystem_WorkoutComponent_H__
#define __Cozmo_Basestation_BehaviorSystem_WorkoutComponent_H__

#include "anki/common/types.h"
#include "anki/cozmo/basestation/moodSystem/moodScorer.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/behaviorObjectives.h"
#include "json/json.h"
#include <string>
#include <vector>

namespace Anki {
namespace Cozmo {

class WorkoutComponent;
class Robot;

// defines a given "workout" including which animations to use and how many lifts to do
class WorkoutConfig {
  friend class WorkoutComponent;
public:
  
  Result InitConfiguration(const Json::Value& config);
  
  // a "workout" for now is just a cube lifting exercise. The robot will play the pre lift animation, then
  // lift the cube, then play post lift. After that, it will play some number of strong lifts, then a
  // transition, followed by some number of weak lifts, and finally a "put down" animation to end the
  // experience. Any of these except the strong and weak lift loop can be set to Count, in which case they are
  // skipped
  
  AnimationTrigger preLiftAnim = AnimationTrigger::Count;
  AnimationTrigger postLiftAnim = AnimationTrigger::Count;
  AnimationTrigger strongLiftAnim = AnimationTrigger::Count;
  AnimationTrigger transitionAnim = AnimationTrigger::Count;
  AnimationTrigger weakLiftAnim = AnimationTrigger::Count;
  AnimationTrigger putDownAnim = AnimationTrigger::Count;

  unsigned int GetNumStrongLifts(const Robot& robot) const;
  unsigned int GetNumWeakLifts(const Robot& robot) const;
  
  BehaviorObjective GetAdditionalBehaviorObjectiveOnComplete() const { return _additionalBehaviorObjectiveOnComplete; }
  
private:
  
  // number of lifts are defined based on mood, rounded to the nearest int (negative will count as 0)
  MoodScorer _numStrongLiftScorer;
  MoodScorer _numWeakLiftScorer;

  static unsigned int MoodScoreHelper(const Robot& robot, const MoodScorer& moodScorer);


  // emotion event to trigger when this workout competed
  std::string _emotionEventOnComplete;
  
  // additional behavior objective that gets emitted upon completion
  BehaviorObjective _additionalBehaviorObjectiveOnComplete;
};


class WorkoutComponent
{
public:

  WorkoutComponent(Robot& robot);

  Result InitConfiguration(const Json::Value& config);

  // returns the current workout the robot should do
  const WorkoutConfig& GetCurrentWorkout() const;

  // marks the current workout as complete, may advance the current workout
  void CompleteCurrentWorkout();

  // returns true if 80's music should be played for the current workout
  bool ShouldPlayEightiesMusic();
  
  // re-determine if 80's music should be played for the current/next workout
  void ShouldRecomputeEightiesMusic() { _hasComputedIfEightiesMusicShouldPlay = false; };
  
private:

  using WorkoutList = std::vector< WorkoutConfig >;
  WorkoutList _workouts;

  // which workout we are doing, must be set after the constructor is complete
  WorkoutList::const_iterator _currWorkout;

  // Should we play the 80's music for the current workout?
  bool _shouldPlayEightiesMusic = false;
  
  // Have we already computed whether or not to play 80's music for the current workout?
  //   (should only compute it once per workout since it involves random numbers)
  bool _hasComputedIfEightiesMusicShouldPlay = false;
  
  Robot& _robot;

};

}
}

#endif
