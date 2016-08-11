/**
 * File: AIGoalEvaluator
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: Evaluator/Selector of high level goals/objectives for the robot
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalEvaluator.h"

#include "AIGoal.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/random/randomGenerator.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

const char* AIGoalEvaluator::kSelfConfigKey = "evaluator";
const char* AIGoalEvaluator::kGoalsConfigKey = "goals";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::AIGoalEvaluator(Robot& robot, const Json::Value& config)
: IBehaviorChooser(robot, config)
, _curGoalIdx(0)
, _lastTimeGoalSwitched(0.0f)
{
  CreateFromConfig(robot, config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::~AIGoalEvaluator()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalEvaluator::CreateFromConfig(Robot& robot, const Json::Value& config)
{
  _goals.clear();

  // read every goal and create them with their own config
  const Json::Value& goalsConfig = config[kGoalsConfigKey];
  if ( !goalsConfig.isNull() )
  {
    // reserve room for goals
    _goals.reserve( goalsConfig.size() );
  
    // iterate all objects in the goals adding them to our container
    Json::Value::const_iterator it = goalsConfig.begin();
    const Json::Value::const_iterator end = goalsConfig.end();
    for(; it != end; ++it )
    {
      const Json::Value& goalConfig = *it;
    
      AIGoal* goal = new AIGoal();
      bool goalInitOk = goal->Init( robot, goalConfig );
      if ( goalInitOk )
      {
        // add the goal to our container
        _goals.emplace_back( goal );
      }
      else
      {
        // flag as failed and log
        PRINT_NAMED_ERROR("AIGoalEvaluator.Init", "Failed to initialize goal from config. Look for 'AIGoalEvaluator.Init.BadGoalConfig' event for information");
        JsonTools::PrintJsonError(goalConfig, "AIGoalEvaluator.Init.BadGoalConfig");
        
        // delete this goal, it did not initialize properly
        Anki::Util::SafeDelete( goal );
      }
      
    }
  }
  
  // set goal index to no-goal
  _curGoalIdx = _goals.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t AIGoalEvaluator::PickNewGoalForSpark(Robot& robot, UnlockId spark) const
{
  for( size_t idx = 0; idx < _goals.size(); ++idx )
  {
    if ( idx != _curGoalIdx )
    {
      const bool sparkMatches = (_goals[idx]->GetRequiredSpark() == spark);
      if ( sparkMatches ) {
        // this is a goal that is not the current one and matches the desired spark
        return idx;
      }
    }
  }

  PRINT_NAMED_WARNING("AIGoalEvaluator.PickNewGoalForSpark", "Could not pick new goal for spark '%s'", EnumToString(spark));
  return _curGoalIdx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoalEvaluator::ChooseNextBehavior(Robot& robot, bool didCurrentFinish) const
{
  // returned variable
  IBehavior* chosenBehavior = nullptr;
  
  // constants
  const float timeoutPerGoal_secs = 120.0f; // this is hardcoded here because it is going to be removed this week

  // current goal
  bool hasGoal = (_curGoalIdx < _goals.size());
  const AIGoal* currentGoal = hasGoal ? _goals[_curGoalIdx].get() : nullptr;
  
  // calculate timeout variables now because they are shared below, making that code easier to read
  const float currentTime = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );
  const float runThisGoalUntil = _lastTimeGoalSwitched+timeoutPerGoal_secs;
  const bool isGoalTimedOut = FLT_GE(currentTime, runThisGoalUntil);
  
  // see if sparks or behavior finished cause a goal change
  bool getNewGoal = false;

  // get a new goal if the current behavior finished succesfully or sparks changed
  const UnlockId activeSpark = robot.GetBehaviorManager().GetActiveSpark();
  const UnlockId goalSpark   = hasGoal ? currentGoal->GetRequiredSpark() : UnlockId::Count;
  const bool changeDueToSpark = (activeSpark != goalSpark);
  if ( !hasGoal || changeDueToSpark )
  {
    // the active spark does not match the current goal, pick the first goal available for the spark
    // this covers both going into and out of sparks
    getNewGoal = true;
    PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior",
      "Picking new goal to match spark '%s'", EnumToString(activeSpark));
  }
  else if ( didCurrentFinish )
  {
    // rsam: note on random chance of change. I implemented here a random change of changing, but it worked, it
    // changed goals in the most random moments, for example, after playing the intro for discovering a beacon,
    // it could switch to interacting with faces in other goal. Not useful at all and could make QA think that there
    // are bugs. Rely on timeouts to decide a goal change
    if ( isGoalTimedOut ) {
      getNewGoal = true;
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior",
        "Picking new goal because '%s' has run for more than %.2f seconds (ran for %.2f), and behavior finished",
        currentGoal->GetName().c_str(),
        timeoutPerGoal_secs, currentTime-_lastTimeGoalSwitched);
    }
  }
  
  // if we don't want to pick a new goal, ask for a new behavior now. The reason is that if we pick a different
  // behavior than the current one, we can also cause a goal change by timeout or if the current goal didn't select
  // a good behavior
  if ( !getNewGoal )
  {
    // pick behavior
    ASSERT_NAMED(nullptr!=currentGoal, "AIGoalEvaluator.CurrentGoalCantBeNull");
    chosenBehavior = currentGoal->ChooseNextBehavior(robot, didCurrentFinish);

    // if the picked behavior is not good, we want a new goal too
    if ( (chosenBehavior == nullptr) || (chosenBehavior->GetType() == BehaviorType::NoneBehavior))
    {
      getNewGoal = true;
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior",
        "Picking new goal because '%s' chose behavior '%s'",
        currentGoal->GetName().c_str(),
        chosenBehavior ? chosenBehavior->GetName().c_str() : "(null)" );
    }
    else if ( !chosenBehavior->IsRunning() )
    {
      // We also want to change goals after some amount of time. However, during testing, I realized that interrupting
      // behaviors while they where doing something (like picking up cubes, or carrying them somewhere), not only looked
      // bad, it had terrible gameplay consequences. So, for timeouts, we are letting the current goal pick behaviors.
      // If the current goal has picked a new one, but it has done so after the timeout period, we are gonna rely on
      // the fact that the goal decided to switch behaviors anyway to also switch goals at this point. Although
      // some of these changes would happen when behaviors change (see didCurrentFinish), this code here adds the extra
      //  layer of checking for timeouts when we switch behaviors because other one becomes higher score.
      if ( isGoalTimedOut )
      {
        getNewGoal = true;
        PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior",
          "Picking new goal because '%s' has run for more than %.2f seconds (ran for %.2f), and it picked a new behavior",
          currentGoal->GetName().c_str(),
          timeoutPerGoal_secs,
          currentTime-_lastTimeGoalSwitched);
      }
    }
  }
  
  // if we have to pick new goal after having chosen a behavior, do it now
  if ( getNewGoal )
  {
    const AIGoal* prevGoal = currentGoal;
    // select new goal
    _curGoalIdx = PickNewGoalForSpark(robot, activeSpark);
    // update current goal variables that change with current
    hasGoal = (_curGoalIdx < _goals.size());
    currentGoal = hasGoal ? _goals[_curGoalIdx].get() : nullptr;
    // update pick time
    _lastTimeGoalSwitched = Util::numeric_cast<float>( BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() );
    
    PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior",
      "Switched goal from '%s' to '%s'",
        prevGoal    ? prevGoal->GetName().c_str() : "no goal",
        currentGoal ? currentGoal->GetName().c_str() : "no goal");

    // since we have changed goal, ask again for a new behavior
    IBehavior* chosenBehavior = nullptr;
    if ( currentGoal ) {
      chosenBehavior = currentGoal->ChooseNextBehavior(robot, didCurrentFinish);
    } else {
      PRINT_NAMED_ERROR("AIGoalEvaluator.ChooseNextBehavior", "No current available goal");
    }
  }
  
  return chosenBehavior;
}


} // namespace Cozmo
} // namespace Anki
