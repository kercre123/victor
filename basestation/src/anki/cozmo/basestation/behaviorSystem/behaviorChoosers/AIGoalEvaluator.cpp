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
#include "AIGoalStrategies/iAIGoalStrategy.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"

#include "util/console/consoleInterface.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
#include "util/random/randomGenerator.h"
#include "json/json.h"

#include <algorithm>
#include <sstream>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helpers and debug utilities
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{
// static const char* kSelfConfigKey = "evaluator";
static const char* kGoalsConfigKey = "goals";

AIGoalEvaluator* defaultGoalEvaluator = nullptr;
void AIGoalEvaluatorSetDebugGoal(const std::string& name)
{
  if ( nullptr != defaultGoalEvaluator ) {
    defaultGoalEvaluator->SetConsoleRequestedGoalName(name);
  } else {
    PRINT_NAMED_WARNING("AIGoalEvaluatorCycleGoal", "No default goal evaluator. Can't cycle goals.");
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalSetFPNothingToDo( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("FP_NothingToDo");
}
CONSOLE_FUNC( AIGoalSetFPNothingToDo, "AIGoalEvaluator" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalSetFPHiking( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("FP_Hiking");
}
CONSOLE_FUNC( AIGoalSetFPHiking, "AIGoalEvaluator" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalSetFPPlayWithHumans( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("FP_PlayWithHumans");
}
CONSOLE_FUNC( AIGoalSetFPPlayWithHumans, "AIGoalEvaluator" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalSetFPPlayAlone( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("FP_PlayAlone");
}
CONSOLE_FUNC( AIGoalSetFPPlayAlone, "AIGoalEvaluator" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalSetFPSocialize( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("FP_Socialize");
}
CONSOLE_FUNC( AIGoalSetFPSocialize, "AIGoalEvaluator" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalClearSetting( ConsoleFunctionContextRef context ) {
  AIGoalEvaluatorSetDebugGoal("");
}
CONSOLE_FUNC( AIGoalClearSetting, "AIGoalEvaluator" );

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::AIGoalEvaluator(Robot& robot, const Json::Value& config)
: IBehaviorChooser(robot, config)
, _name("GoalEvaluator")
, _currentGoalPtr(nullptr)
{
  CreateFromConfig(robot, config);
  
  #if ( ANKI_DEV_CHEATS )
  {
    if ( !defaultGoalEvaluator ) {
      defaultGoalEvaluator = this;
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalEvaluator::~AIGoalEvaluator()
{
  #if ( ANKI_DEV_CHEATS )
  {
    if ( defaultGoalEvaluator == this ) {
      defaultGoalEvaluator = nullptr;
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalEvaluator::CreateFromConfig(Robot& robot, const Json::Value& config)
{
  _goals.clear();

  // read every goal and create them with their own config
  const Json::Value& goalsConfig = config[kGoalsConfigKey];
  if ( !goalsConfig.isNull() )
  {
    // reserve room for goals (note we reserve 1 per goal as upper bound, even if some goals fall in the same unlockId)
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
        const UnlockId goalSpark = goal->GetRequiredSpark();
        // add the goal to the vector of goals with the same spark
        _goals[goalSpark].emplace_back( goal );
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
  
  // sort goals by priority now so that later we can iterate from front to back
  // define lambda to sort. Note the container holds unique_ptrs
  auto sortByPriority = [](const std::unique_ptr<AIGoal>& goal1, const std::unique_ptr<AIGoal>& goal2) {
    ASSERT_NAMED(goal1->GetPriority() != goal2->GetPriority(), "AIGoalEvaluator.SamePriorityNotSupported"); // there's no tie-break
    const bool isBetterPriority = (goal1->GetPriority() < goal2->GetPriority());
    return isBetterPriority;
  };
  // iterate all goal vectors and sort
  for(auto& goalPair : _goals)
  {
    GoalVector& goalsPerUnlock = goalPair.second;
    std::sort(goalsPerUnlock.begin(), goalsPerUnlock.end(), sortByPriority);
  }
  
  // DebugPrintGoals();
  
  // clear current goal
  _currentGoalPtr = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalEvaluator::PickNewGoalForSpark(Robot& robot, UnlockId spark, bool isCurrentAllowed)
{
  // find goals that can run for the given spark
  const SparkToGoalsTable::iterator goalVectorIt = _goals.find(spark);
  if ( goalVectorIt != _goals.end() )
  {
    GoalVector& goalsForThisSpark = goalVectorIt->second;
  
    // can't have the spark registered but have no goals in the container, programmer error
    ASSERT_NAMED(!goalsForThisSpark.empty(), "AIGoalEvaluator.RegisteredSparkHasNoGoals");
    
    AIGoal* newGoal = nullptr;
    
    // goals are sorted by priority within this spark. Iterate from first to last until one wants to run
    for(auto& goal : goalsForThisSpark)
    {
      const IAIGoalStrategy& selectionStrategy = goal->GetStrategy();
      
      // if there's a debug console requested goal, force to pick that one
      if ( !_debugConsoleRequestedGoal.empty() )
      {
        // skip if name doesn't match
        if ( goal->GetName() != _debugConsoleRequestedGoal ) {
          continue;
        }
      }
      else
      {
        // check if this goal is the one that should be running
        // for current goal, check if it wants to end,
        // for non-current goals, check if they want to start
        const bool isCurrentRunningGoal = (goal.get() == _currentGoalPtr);
        if ( isCurrentRunningGoal )
        {
          // if the current goal is not allowed, skip it
          if ( !isCurrentAllowed ) {
            continue;
          }
          // it's currently running, check if we want to end
          const float lastTimeStarted = goal->GetLastTimeStartedSecs();
          const bool wantsToEnd = selectionStrategy.WantsToEnd(robot, lastTimeStarted);
          if ( wantsToEnd ) {
            continue;
          }
        }
        else
        {
          // it's not running, check if we want to start
          const float lastTimeFinished = goal->GetLastTimeStoppedSecs();
          const bool wantsToStart = selectionStrategy.WantsToStart(robot, lastTimeFinished);
          if ( !wantsToStart ) {
            continue;
          }
        }
      }
      
      // this is the best priority goal that wants to run, done searching
      newGoal = goal.get();
      break;
    }
    
    // check if we have selected something different than the current goal
    const bool selectedNew = (_currentGoalPtr != newGoal);
    if ( selectedNew )
    {
      // TODO consider a different cooldown for isCurrentAllowedToBePicked==false
      // Since the goal could not pick a valid behavior, but did want to run. Kicking it out now will trigger
      // regular cooldowns, while we might want to set a smaller cooldown due to failure
    
      // DAS
      PRINT_NAMED_EVENT("AIGoalEvaluator.NewGoalSelected",
        "Switched goal from '%s' to '%s' (spark '%s')",
        _currentGoalPtr ? _currentGoalPtr->GetName().c_str() : "no goal",
        newGoal         ? newGoal->GetName().c_str() : "no goal",
        EnumToString(spark) );
    
      // log
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.PickNewGoalForSpark",
        "Switched goal from '%s' to '%s'",
          _currentGoalPtr ? _currentGoalPtr->GetName().c_str() : "no goal",
          newGoal         ? newGoal->GetName().c_str() : "no goal" );
      // set new goal and timestamp
      if ( _currentGoalPtr ) {
        _currentGoalPtr->Exit(robot);
      }
      _currentGoalPtr = newGoal;
      _name = "Goal" + ( _currentGoalPtr ? _currentGoalPtr->GetName() : "None");
      if ( _currentGoalPtr ) {
        _currentGoalPtr->Enter(robot);
      }
    }
    else if ( _currentGoalPtr != nullptr )
    {
      // we picked the same goal
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.PickNewGoalForSpark",
        "Current goal '%s' is still the best one to run for spark '%s', %s",
          _currentGoalPtr ? _currentGoalPtr->GetName().c_str() : "no goal",
          EnumToString(spark),
          ( _debugConsoleRequestedGoal == _currentGoalPtr->GetName() ) ? "forced from debug" : "(not debug forced)");
    }
    else
    {
        // we did not select any goals. This means we are not going to run behaviors. It should not happen if at
        // least a fallback goal was defined, which can be done through proper strategy selection in data, so we
        // shouldn't correct it here, other than warn if it ever happens
        PRINT_NAMED_WARNING("AIGoalEvaluator.PickNewGoalForSpark",
          "None of the goals available for spark '%s' wants to run.",
          EnumToString(spark));
    }
    
    return selectedNew;
  }
  else
  {
    PRINT_NAMED_WARNING("AIGoalEvaluator.PickNewGoalForSpark", "No goals available for spark '%s'", EnumToString(spark));
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* AIGoalEvaluator::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // returned variable
  IBehavior* chosenBehavior = nullptr;
  
  // see if anything causes a goal change
  bool getNewGoal = false;
  bool isCurrentAllowedToBePicked = true;
  bool hasChosenBehavior = false;
  
  
  // check if we have a debugConsole goal
  if ( !_debugConsoleRequestedGoal.empty() )
  {
    // if we are not running the debug requested goal, ask to change. It should be picked if name is available
    if ( (!_currentGoalPtr) || _currentGoalPtr->GetName() != _debugConsoleRequestedGoal ) {
      getNewGoal = true;
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.DebugForcedChange",
        "Picking new goal because debug is forcing '%s'", _debugConsoleRequestedGoal.c_str());
    }
  }
  else
  {
    // no debug requests
    // get a new goal if the current behavior finished succesfully or sparks changed
    
    const bool changeDueToSpark = robot.GetBehaviorManager().ShouldSwitchToSpark();
    if ( (nullptr == _currentGoalPtr) || changeDueToSpark )
    {
      // the active spark does not match the current goal, pick the first goal available for the spark
      // this covers both going into and out of sparks
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.NullOrChangeDueToSpark",
                    "Picking new goal to match spark '%s'", EnumToString(robot.GetBehaviorManager().GetRequestedSpark()));
      getNewGoal = true;
    }
    else if ( currentRunningBehavior == nullptr )
    {
      // check with the current goal if it wants to end
      // note that we will also ask the current goal this when picking new goals. This additional check is not
      // superflous: since lower priority goals are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the GoalEvaluator.
      const float goalStartTime = _currentGoalPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentGoalPtr->GetStrategy().WantsToEnd(robot, goalStartTime);
      if ( currentWantsToEnd )
      {
        getNewGoal = true;
        PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.BehaviorAndGoalEnded",
          "Picking new goal because '%s' wants to end, and behavior finished",
          _currentGoalPtr->GetName().c_str());
      }
    }
  }
  
  // if we don't want to pick a new goal, ask for a new behavior now. The reason is that if we pick a different
  // behavior than the current one, we can also cause a goal change by timeout or if the current goal didn't select
  // a good behavior
  if ( !getNewGoal )
  {
    // pick behavior
    ASSERT_NAMED(nullptr!=_currentGoalPtr, "AIGoalEvaluator.CurrentGoalCantBeNull");
    chosenBehavior = _currentGoalPtr->ChooseNextBehavior(robot, currentRunningBehavior);
    hasChosenBehavior = true;

    // if the picked behavior is not good, we want a new goal too
    if ( (chosenBehavior == nullptr) || (chosenBehavior->GetType() == BehaviorType::NoneBehavior))
    {
      getNewGoal = true;
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.NoBehaviorChosenWhileRunning",
        "Picking new goal because '%s' chose behavior '%s'. This goal is not allowed to be repicked.",
        _currentGoalPtr->GetName().c_str(),
        chosenBehavior ? chosenBehavior->GetName().c_str() : "(null)" );
    }
    else if ( !chosenBehavior->IsRunning() )
    {
      // check with the current goal if it wants to end
      // note that we will also ask the current goal this when picking new goals. This additional check is not
      // superflous: since lower priority goals are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the GoalEvaluator.
      const float goalStartTime = _currentGoalPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentGoalPtr->GetStrategy().WantsToEnd(robot, goalStartTime);
      if ( currentWantsToEnd )
      {
        getNewGoal = true;
        PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.NewBehaviorChosenWhileRunning",
          "Picking new goal because '%s' wants to end, and behavior finished",
          _currentGoalPtr->GetName().c_str());
      }
    }
  }
  
  // if we have to pick new goal after having chosen a behavior, do it now
  if ( getNewGoal )
  {
    // Ensure that if a user cancels and re-trigger the same spark, the behavior exits and re-enters
    if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
       && robot.GetBehaviorManager().DidGameRequestSparkEnd()){
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors","AIGoalEvaluator.ChooseNextBehavior.SparkReselected", "Spark re-selected: none behavior will be selected");
    }
    
    // select new goal
    const UnlockId desiredSpark = robot.GetBehaviorManager().GetRequestedSpark();
    const bool changedGoal = PickNewGoalForSpark(robot, desiredSpark, isCurrentAllowedToBePicked);
    if ( changedGoal || !hasChosenBehavior )
    {
      // if we did change goals, flag as a change of spark too
      if ( changedGoal )
      {
        robot.GetBehaviorManager().SwitchToRequestedSpark();
        // warn if the goal selected does not match the desired spark
        if ( _currentGoalPtr && (_currentGoalPtr->GetRequiredSpark() != desiredSpark) ) {
          PRINT_NAMED_WARNING("AIGoalEvaluator.ChooseNextBehavior.GoalNotDesiredSpark",
            "The selected goal's required spark '%s' does not match the desired '%s'",
            EnumToString(_currentGoalPtr->GetRequiredSpark()),
            EnumToString(desiredSpark));
        }
      }
    
      // either we picked a new goal or we have the same but we didn't let it chose behavior before because
      // we wanted to check if there was a better one, so do it now
      if ( _currentGoalPtr )
      {
        chosenBehavior = _currentGoalPtr->ChooseNextBehavior(robot, currentRunningBehavior);
        
        // if the first behavior chosen is null/None, the goal conditions to start didn't handle the current situation.
        // The goal will be asked to leave next frame if it continues to pick null/None.
        // TODO in this case we might want to apply a different cooldown, rather than the default for
        // the goal.
        if ( changedGoal && ((!chosenBehavior)||(chosenBehavior->GetType() == BehaviorType::NoneBehavior)))
        {
          PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.ChooseNextBehavior.NewGoalDidNotChooseBehavior",
            "The new goal '%s' picked no behavior. It probably didn't cover the same conditions as the behaviors.",
            _currentGoalPtr->GetName().c_str());
        }
      } else {
        if ( isCurrentAllowedToBePicked ) {
          PRINT_NAMED_ERROR("AIGoalEvaluator.NoGoalAvailableError", "No current available goal");
        } else {
          PRINT_CH_INFO("Behaviors","AIGoalEvaluator.NoGoalAvailable.CurrentNotAllowed", "No current available goal");
        }
      }
    }
  }
  
  return chosenBehavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIGoalEvaluator::DebugPrintGoals() const
{
  PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.PrintGoals", "There are %zu unlockIds registered with goals:", _goals.size());
  // log a line per unlockId/sparkId
  for( const auto& goalTablePair : _goals )
  {
    // start the line with the sparkId
    std::stringstream stringForUnlockId;
    const UnlockId sparkId = goalTablePair.first;
    stringForUnlockId << "(" << EnumToString(sparkId) << ") : ";
    
    // now add every goal
    const GoalVector& goalsInSpark = goalTablePair.second;
    for( const auto& goal : goalsInSpark ) {
      stringForUnlockId << " --> [" << Util::numeric_cast<int>( goal->GetPriority() ) << "] " << goal->GetName();
    }
    
    // log
    PRINT_CH_INFO("Behaviors", "AIGoalEvaluator.PrintGoals", "%s", stringForUnlockId.str().c_str());
  }
}


} // namespace Cozmo
} // namespace Anki
