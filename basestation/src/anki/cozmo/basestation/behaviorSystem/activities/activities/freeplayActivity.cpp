/**
 * File: freeplayActivity.cpp
 *
 * Author: Raul
 * Created: 05/02/16
 *
 * Description: High level activity which determines which freeplay subActivities
 * to run
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "anki/cozmo/basestation/behaviorSystem/activities/activities/freeplayActivity.h"

#include "anki/cozmo/basestation/behaviorSystem/activities/activities/iActivity.h"
#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/iActivityStrategy.h"

#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotDataLoader.h"

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
static const char* kDesiredActivityNamesConfigKey = "desiredActivityNames";
static const char* kActivitiesConfigKey = "activities";

// Keys to identify activities after put down dispatch
static const char* kFaceAndCubeConfigKey  = "faceAndCubeActivityName";
static const char* kFaceOnlyConfigKey     = "faceOnlyActivityName";
static const char* kCubeOnlyConfigKey     = "cubeOnlyActivityName";
static const char* kNoFaceNoCubeConfigKey = "noFaceNoCubeActivityName";
static const char* kObjectTapConfigKey    = "objectTapInteractionActivityName";


#if REMOTE_CONSOLE_ENABLED

FreeplayActivity* defaultFeeplayActivity = nullptr;
void FreeplayActivitySetDebugActivity(const std::string& name)
{
  if ( nullptr != defaultFeeplayActivity ) {
    defaultFeeplayActivity->SetConsoleRequestedActivityName(name);
  } else {
    PRINT_NAMED_WARNING("FreeplayActivityCycleActivity", "No default freeplayActivity. Can't cycle activities.");
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPNothingToDo( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("nothingToDo");
}
CONSOLE_FUNC( ActivitySetFPNothingToDo, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPHiking( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("hiking");
}
CONSOLE_FUNC( ActivitySetFPHiking, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPPlayWithHumans( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("playWithHumans");
}
CONSOLE_FUNC( ActivitySetFPPlayWithHumans, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPPlayAlone( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("playAlone");
}
CONSOLE_FUNC( ActivitySetFPPlayAlone, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPSocialize( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("socialize");
}
CONSOLE_FUNC( ActivitySetFPSocialize, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPBuildPyramid( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("buildPyramid");
}
CONSOLE_FUNC( ActivitySetFPBuildPyramid, "FreeplayActivity" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityClearSetting( ConsoleFunctionContextRef context ) {
  FreeplayActivitySetDebugActivity("");
}
CONSOLE_FUNC( ActivityClearSetting, "FreeplayActivity" );
  
#endif // REMOTE_CONSOLE_ENABLED

};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FreeplayActivity::FreeplayActivity(Robot& robot, const Json::Value& config)
: IBehaviorChooser(robot, config)
, _name("FreeplayActivity")
, _currentActivityPtr(nullptr)
{
  CreateFromConfig(robot, config);
  
  // register to events
  if ( _robot.HasExternalInterface() )
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*_robot.GetExternalInterface(), *this, _signalHandles);
    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotOffTreadsStateChanged>();
  }
  else {
    PRINT_NAMED_WARNING("FreeplayActivity::FreeplayActivity",
                        "Initialized FreeplayActivity with no external interface. Will miss events.");
  }
  
  #if ( ANKI_DEV_CHEATS )
  {
    if ( !defaultFeeplayActivity ) {
      defaultFeeplayActivity = this;
    }
  }
  #endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FreeplayActivity::~FreeplayActivity()
{
  #if ( ANKI_DEV_CHEATS )
  {
    if ( defaultFeeplayActivity == this ) {
      defaultFeeplayActivity = nullptr;
    }
  }
  #endif
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result FreeplayActivity::Update(Robot& robot)
{
  auto result = Result::RESULT_OK;
  if(_currentActivityPtr != nullptr){
    result = _currentActivityPtr->Update(robot);
  }
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FreeplayActivity::OnDeselected()
{
  if ( _currentActivityPtr ) {
    _currentActivityPtr->Exit(_robot);
    _currentActivityPtr = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void FreeplayActivity::HandleMessage(const ExternalInterface::RobotOffTreadsStateChanged& msg)
{
  // if we return to OnTreads and we are in freeplay without sparks, clear the activity to pick a new one as soon
  // as we regain control.
  const bool onTreads = msg.treadsState == OffTreadsState::OnTreads;
  if ( onTreads )
  {
    if ( _currentActivityPtr )
    {
      const UnlockId curActivitySpark = _currentActivityPtr->GetRequiredSpark();
      const bool isSparkless = (curActivitySpark == UnlockId::Count);
      const bool isRunningDebugActivity = (_currentActivityPtr->GetName() ==
                                                 _debugConsoleRequestedActivity);
      if ( isSparkless && !isRunningDebugActivity )
      {
        PRINT_CH_INFO("Behaviors", "FreeplayActivity.RobotOffTreadsStateChanged.KickingOutActivityOnPutDown",
          "Kicking out '%s' on put down so we pick up a new one", _currentActivityPtr->GetName().c_str() );
      
        // note: this will set the activity on cooldown, which may not be desired if it didn't run for some time
        
        // stop current activity
        _currentActivityPtr->Exit(_robot);
        _currentActivityPtr = nullptr;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FreeplayActivity::CreateFromConfig(Robot& robot, const Json::Value& config)
{
  // TODO: rename _activities
  _activities.clear();
  
  // read every activity and create them with their own config
  const Json::Value& activityPriorities = config[kActivitiesConfigKey];
  if ( !activityPriorities.isNull() )
  {
    // Get the activity jsons
    const auto& activityData = robot.GetContext()->GetDataLoader()->GetActivityJsons();
    DEV_ASSERT_MSG(activityPriorities.size() == activityData.size(),
                   "FreeplayActivity.CreateFromConfig.ActivitySizeMismatch",
                   "There are %d priorities and %d activities defined",
                   static_cast<int>(activityPriorities.size()),
                   static_cast<int>(activityData.size()));
    
    // Iterate through the activities
    uint debugLastActivityPriority = 0;
    
    // reserve room for activites (note we reserve 1 per activity as upper bound,
    //                             even if some activites fall in the same unlockId)
    _activities.reserve( activityPriorities.size() );
  
    // iterate all activity names in priority order, grab their data, and load them
    // into the container
    static const char* activityNameKey     = "name";
    static const char* activityPriorityKey = "priority";
    
    Json::Value::const_iterator it = activityPriorities.begin();
    const Json::Value::const_iterator end = activityPriorities.end();
    
    for(; it != end; ++it )
    {
      // values to be set from JSON
      std::string activityName = JsonTools::ParseString(*it, activityNameKey, "FreeplayActivity.ActivityName");
      uint activityPriority = JsonTools::ParseUint8(*it, activityPriorityKey, "FreeplayActivity.ActivityPriority");
      
      // Ensure that activities are listed in priority order
      DEV_ASSERT_MSG(activityPriority >= debugLastActivityPriority,
                     "FreeplayActivity.CreateFromConfig.ActivityPrioritiesOutOfOrder",
                     "Activity name %s", activityName.c_str());
      debugLastActivityPriority = activityPriority;
      
      
      const auto& dataIter = activityData.find(activityName);
      
      DEV_ASSERT_MSG(dataIter != activityData.end(),
                     "FreeplayActivity.CreateFromConfig.ActivityDataNotFound",
                     "No file found named %s", activityName.c_str());
      
      const Json::Value& activityConfig = dataIter->second;
    
      IActivity* activity = new IActivity();
      bool activityInitOk = activity->Init( robot, activityConfig );
      if ( activityInitOk )
      {
        const UnlockId activitySpark = activity->GetRequiredSpark();
        // add the activity to the vector of activities with the same spark
        _activities[activitySpark].emplace_back( activity );
      }
      else
      {
        // flag as failed and log
        PRINT_NAMED_ERROR("FreeplayActivity.Init",
                          "Failed to initialize activity from config. Look for 'FreeplayActivity.Init.BadActivityConfig' event for information");
        JsonTools::PrintJsonError(activityConfig, "FreeplayActivity.Init.BadActivityConfig");
        
        // delete this activity, it did not initialize properly
        Anki::Util::SafeDelete( activity );
      }
      
    }
  }
  
  // DebugPrintActivities();
  
  // clear current activity
  _currentActivityPtr = nullptr;
  
  // load desired activites
  {
    const Json::Value& desiredActivities = config[kDesiredActivityNamesConfigKey];
    using namespace JsonTools;
    const std::string& debugName = _name + ".FreeplayActivity.Constructor";
  
    // parse desired activities from objects
    _configParams.faceAndCubeActivityName =  ParseString(desiredActivities, kFaceAndCubeConfigKey,  debugName);
    _configParams.faceOnlyActivityName =     ParseString(desiredActivities, kFaceOnlyConfigKey,     debugName);
    _configParams.cubeOnlyActivityName =     ParseString(desiredActivities, kCubeOnlyConfigKey,     debugName);
    _configParams.noFaceNoCubeActivityName = ParseString(desiredActivities, kNoFaceNoCubeConfigKey, debugName);
    _configParams.objectTapInteractionActivityName =
                                             ParseString(desiredActivities, kObjectTapConfigKey,    debugName);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FreeplayActivity::PickNewActivityForSpark(Robot& robot, UnlockId spark, bool isCurrentAllowed)
{
  // find activities that can run for the given spark
  const SparkToActivitiesTable::iterator activityVectorIt = _activities.find(spark);
  if ( activityVectorIt != _activities.end() )
  {
    ActivityVector& activitiesForThisSpark = activityVectorIt->second;
  
    // can't have the spark registered but have no activities in the container, programmer error
    DEV_ASSERT(!activitiesForThisSpark.empty(), "FreeplayActivity.RegisteredSparkHasNoActivities");
    
    IActivity* newActivity = nullptr;
    
    // activities are sorted by priority within this spark.
    // Iterate from first to last until one wants to run
    for(auto& activity : activitiesForThisSpark)
    {
      const IActivityStrategy& selectionStrategy = activity->GetStrategy();
      
      // if there's a debug console requested activity, force to pick that one
      if ( !_debugConsoleRequestedActivity.empty() )
      {
        // skip if name doesn't match
        if ( activity->GetName() != _debugConsoleRequestedActivity ) {
          continue;
        }
      }
      else if ( !_requestedActivity.empty() )
      {
        // skip if name doesn't match
        if ( activity->GetName() != _requestedActivity ) {
          continue;
        }
      }
      else
      {
        // check if this activity is the one that should be running
        // for current activity, check if it wants to end,
        // for non-current activities, check if they want to start
        const bool isCurrentRunningActivity = (activity.get() == _currentActivityPtr);
        if ( isCurrentRunningActivity )
        {
          // if the current activity is not allowed, skip it
          if ( !isCurrentAllowed ) {
            continue;
          }
          // it's currently running, check if we want to end
          const float lastTimeStarted = activity->GetLastTimeStartedSecs();
          const bool wantsToEnd = selectionStrategy.WantsToEnd(robot, lastTimeStarted);
          if ( wantsToEnd ) {
            continue;
          }
        }
        else
        {
          // it's not running, check if we want to start
          const float lastTimeStarted = activity->GetLastTimeStartedSecs();
          const float lastTimeFinished = activity->GetLastTimeStoppedSecs();
          const bool wantsToStart = selectionStrategy.WantsToStart(robot, lastTimeFinished, lastTimeStarted);
          if ( !wantsToStart ) {
            continue;
          }
        }
      }
      
      // this is the best priority activity that wants to run, done searching
      newActivity = activity.get();
      break;
    }
    
    // check if we have selected something different than the current activity
    const bool selectedNew = (_currentActivityPtr != newActivity);
    if ( selectedNew )
    {
      // TODO consider a different cooldown for isCurrentAllowedToBePicked==false
      // Since the activity could not pick a valid behavior, but did want to run. Kicking it out now will trigger
      // regular cooldowns, while we might want to set a smaller cooldown due to failure

      // BN: NOTE: I implemented a version of this by checking the time the activity ran and using a different
      // cooldown if it ran for a very short period
    
      // DAS - Legacy DAS event from when activities were called goals
      // Do not change or analytics will break
      LOG_EVENT("AIGoalEvaluator.NewGoalSelected",
        "Switched activity from '%s' to '%s' (spark '%s')",
        _currentActivityPtr ? _currentActivityPtr->GetName().c_str() : "no activity",
        newActivity         ? newActivity->GetName().c_str() : "no activity",
        EnumToString(spark) );
    
      // log
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.PickNewActivityForSpark.Switched",
        "Switched activity from '%s' to '%s'",
          _currentActivityPtr ? _currentActivityPtr->GetName().c_str() : "no activity",
          newActivity         ? newActivity->GetName().c_str() : "no activity" );
      // set new activity and timestamp
      if ( _currentActivityPtr ) {
        _currentActivityPtr->Exit(robot);
      }
      _currentActivityPtr = newActivity;
      _name = "Goal" + ( _currentActivityPtr ? _currentActivityPtr->GetName() : "None");
      if ( _currentActivityPtr ) {
        _currentActivityPtr->Enter(robot);
      }
    }
    else if ( _currentActivityPtr != nullptr )
    {
      // we picked the same activity
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.PickNewActivityForSpark",
        "Current activity '%s' is still the best one to run for spark '%s', %s",
          _currentActivityPtr ? _currentActivityPtr->GetName().c_str() : "no activity",
          EnumToString(spark),
          ( _debugConsoleRequestedActivity == _currentActivityPtr->GetName() ) ? "forced from debug" : "(not debug forced)");
    }
    else
    {
        // we did not select any activity. This means we are not going to run behaviors. It should not happen if at
        // least a fallback activity was defined, which can be done through proper strategy selection in data, so we
        // shouldn't correct it here, other than warn if it ever happens
        PRINT_NAMED_WARNING("FreeplayActivity.PickNewActivityForSpark.NoActivitySelected",
          "None of the activities available for spark '%s' wants to run.",
          EnumToString(spark));
    }
    
    return selectedNew;
  }
  else
  {
    PRINT_NAMED_WARNING("FreeplayActivity.PickNewActivityForSpark.NoActivityAvailableForSpark",
                        "No activity available for spark '%s'", EnumToString(spark));
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FreeplayActivity::CalculateDesiredActivityFromObjects()
{
  // note that here we rely on "since we delocalized"
  
  // check if we have discovered new faces since we delocalized (last face's origin should be current)
  Pose3d lastFacePose;
  const TimeStamp_t lastFaceSeenTimestamp = _robot.GetFaceWorld().GetLastObservedFace(lastFacePose);
  const bool hasNewFace = (lastFaceSeenTimestamp>0) && ((&lastFacePose.FindOrigin()) == _robot.GetWorldOrigin());

  // check if we have discovered new cubes since we delocalized (any not unknown in current origin)
  BlockWorldFilter cubeFilter;
  cubeFilter.SetAllowedFamilies({{ ObjectFamily::Block, ObjectFamily::LightCube }});
  const bool hasNewCube = (_robot.GetBlockWorld().FindLocatedMatchingObject(cubeFilter) != nullptr);
  
  // depending on what we see, request the activity we want
  if ( hasNewFace && hasNewCube ) {
    _requestedActivity = _configParams.faceAndCubeActivityName;
  } else if ( hasNewFace ) {
    _requestedActivity = _configParams.faceOnlyActivityName;
  }  else if ( hasNewCube ) {
    _requestedActivity = _configParams.cubeOnlyActivityName;
  }  else {
    _requestedActivity = _configParams.noFaceNoCubeActivityName;
  }

  // log event to das - note robot.goal is a legacy name for Activities left
  // in place so that data is queriable - please do not change
  Util::sEventF("robot.goal_from_face_and_cube",
                {{DDATA, _requestedActivity.c_str()}},
                "%d:%d",
                hasNewFace ? 1 : 0,
                hasNewCube ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* FreeplayActivity::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  // returned variable
  IBehavior* chosenBehavior = nullptr;
  const bool hasActivityAtStart = (_currentActivityPtr != nullptr);
  
  // see if anything causes an activity change
  bool getNewActivity = false;
  bool isCurrentAllowedToBePicked = true;
  bool hasChosenBehavior = false;
  
  // check if we have a debugConsole activity
  if ( !_debugConsoleRequestedActivity.empty() )
  {
    // if we are not running the debug requested activity, ask to change. It should be picked if name is available
    if ( (!_currentActivityPtr) || _currentActivityPtr->GetName() != _debugConsoleRequestedActivity ) {
      getNewActivity = true;
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.DebugForcedChange",
        "Picking new activity because debug is forcing '%s'", _debugConsoleRequestedActivity.c_str());
    }
  }
  else if ( !_requestedActivity.empty() )
  {
    // if we are not running the requested activity, ask to change. It should be picked if name is available
    if ( (!_currentActivityPtr) || _currentActivityPtr->GetName() != _requestedActivity ) {
      getNewActivity = true;
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.RequestedActivity",
        "Picking new activity because '%s' was requested", _requestedActivity.c_str());
    }
  }
  else
  {
    // no debug requests
    // get a new activity if the current behavior finished succesfully or sparks changed
    
    const bool changeDueToSpark = robot.GetBehaviorManager().ShouldSwitchToSpark();
    if ( (nullptr == _currentActivityPtr) || changeDueToSpark )
    {
      // the active spark does not match the current activity, pick the first activity available for the spark
      // this covers both going into and out of sparks
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.NullOrChangeDueToSpark",
                    "Picking new activity to match spark '%s'", EnumToString(robot.GetBehaviorManager().GetRequestedSpark()));
      getNewActivity = true;
    }
    else if ( currentRunningBehavior == nullptr )
    {
      // check with the current activity if it wants to end
      // note that we will also ask the current activity this when picking new activities. This additional check is not
      // superflous: since lower priority activities are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the FreeplayActivity.
      const float activityStartTime = _currentActivityPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(robot, activityStartTime);
      if ( currentWantsToEnd )
      {
        getNewActivity = true;
        PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.BehaviorAndActivityEnded",
          "Picking new activity because '%s' wants to end, and behavior finished",
          _currentActivityPtr->GetName().c_str());
      }
    }
  }
  
  // if we don't want to pick a new activity, ask for a new behavior now. The reason is that if we pick a different
  // behavior than the current one, we can also cause an activity change by timeout or if the current activity didn't select
  // a good behavior
  if ( !getNewActivity )
  {
    // pick behavior
    DEV_ASSERT(nullptr!=_currentActivityPtr, "FreeplayActivity.CurrentActivityCantBeNull");
    chosenBehavior = _currentActivityPtr->ChooseNextBehavior(robot, currentRunningBehavior);
    hasChosenBehavior = true;

    // if the picked behavior is not good, we want a new activity too
    if ( (chosenBehavior == nullptr) || (chosenBehavior->GetClass() == BehaviorClass::NoneBehavior))
    {
      getNewActivity = true;
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.NoBehaviorChosenWhileRunning",
        "Picking new activity because '%s' chose behavior '%s'. This activity is not allowed to be repicked.",
        _currentActivityPtr->GetName().c_str(),
        chosenBehavior ? chosenBehavior->GetName().c_str() : "(null)" );
    }
    else if ( !chosenBehavior->IsRunning() )
    {
      // check with the current activity if it wants to end
      // note that we will also ask the current activity this when picking new activities. This additional check is not
      // superflous: since lower priority activities are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the FreeplayActivity.
      const float activityStartTime = _currentActivityPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(robot, activityStartTime);
      if ( currentWantsToEnd )
      {
        getNewActivity = true;
        PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.NewBehaviorChosenWhileRunning",
          "Picking new activity because '%s' wants to end, and behavior finished",
          _currentActivityPtr->GetName().c_str());
      }
    }
  }
  
  // if we have to pick new activity after having chosen a behavior, do it now
  if ( getNewActivity )
  {
    // Ensure that if a user cancels and re-trigger the same spark, the behavior exits and re-enters
    if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
       && robot.GetBehaviorManager().DidGameRequestSparkEnd()){
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors","FreeplayActivity.ChooseNextBehavior.SparkReselected", "Spark re-selected: none behavior will be selected");
    }
    
    // select new activity
    const UnlockId desiredSpark = robot.GetBehaviorManager().GetRequestedSpark();
    const bool changedActivity = PickNewActivityForSpark(robot, desiredSpark, isCurrentAllowedToBePicked);
    if ( changedActivity || !hasChosenBehavior )
    {
      // if we did change activities, flag as a change of spark too
      if ( changedActivity )
      {
        robot.GetBehaviorManager().SwitchToRequestedSpark();
        // warn if the activity selected does not match the desired spark
        if ( _currentActivityPtr && (_currentActivityPtr->GetRequiredSpark() != desiredSpark) ) {
          PRINT_NAMED_WARNING("FreeplayActivity.ChooseNextBehavior.ActivityNotDesiredSpark",
            "The selected activity's required spark '%s' does not match the desired '%s'",
            EnumToString(_currentActivityPtr->GetRequiredSpark()),
            EnumToString(desiredSpark));
        }
      }
    
      // either we picked a new activity or we have the same but we didn't let it chose behavior before because
      // we wanted to check if there was a better one, so do it now
      if ( _currentActivityPtr )
      {
        chosenBehavior = _currentActivityPtr->ChooseNextBehavior(robot, currentRunningBehavior);
        
        // if the first behavior chosen is null/None, the activity conditions to start didn't handle the current situation.
        // The activity will be asked to leave next frame if it continues to pick null/None.
        // TODO in this case we might want to apply a different cooldown, rather than the default for
        // the activity.
        if ( changedActivity && ((!chosenBehavior)||(chosenBehavior->GetClass() == BehaviorClass::NoneBehavior)))
        {
          PRINT_CH_INFO("Behaviors", "FreeplayActivity.ChooseNextBehavior.NewActivityDidNotChooseBehavior",
            "The new activity '%s' picked no behavior. It probably didn't cover the same conditions as the behaviors.",
            _currentActivityPtr->GetName().c_str());
        }
      } else {
        // selecting no activity is a valid situation. We need the lowest ordering activity to say it wants to end, so that
        // a higher ordering activity can take over. If none are avaiable, since the lowest ordering activity will give up
        // to give others a chance, there will be at least a frame in which there will be no activity selected.
        PRINT_CH_INFO("Behaviors","FreeplayActivity.NoActivitySelected", "Picked no activity");
      }
    }
    
    // clear the requested activity since we picked an activity. Even if it wasn't selected, we are only going to give it
    // once chance of running
    if ( !_requestedActivity.empty() ) {
      _requestedActivity.clear();
    }
  }
  
  // not selecting a activity at the end of a frame is a valid situation, as long as we had one this frame. We want to
  // check however that if we didn't have a activity at start, and we have not picked a new one, Cozmo is effectively
  // lost, not knowing what to do.
  const bool hasActivityAtEnd = (_currentActivityPtr != nullptr);
  if ( !hasActivityAtStart && !hasActivityAtEnd ) {
    PRINT_NAMED_ERROR("FreeplayActivity.NoActivityAvailableError", "There was no activity, and no activity was selected");
  }
  
  return chosenBehavior;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FreeplayActivity::DebugPrintActivities() const
{
  PRINT_CH_INFO("Behaviors", "FreeplayActivity.PrintActivities",
                "There are %zu unlockIds registered with activity:",
                _activities.size());
  
  // log a line per unlockId/sparkId
  for( const auto& activityTablePair : _activities )
  {
    // start the line with the sparkId
    std::stringstream stringForUnlockId;
    const UnlockId sparkId = activityTablePair.first;
    stringForUnlockId << "(" << EnumToString(sparkId) << ") : ";
    
    // now add every activity
    const ActivityVector& activitiesInSpark = activityTablePair.second;
    for( const auto& activity : activitiesInSpark ) {
      stringForUnlockId << " --> " << activity->GetName();
    }
    
    // log
    PRINT_CH_INFO("Behaviors", "FreeplayActivity.PrintActivities", "%s", stringForUnlockId.str().c_str());
  }
}


bool FreeplayActivity::IsCurrentActivityObjectTapInteraction() const
{
  return ((_currentActivityPtr != nullptr ) &&
          (_currentActivityPtr->GetName() == _configParams.objectTapInteractionActivityName));
}

void FreeplayActivity::ClearObjectTapInteractionRequestedActivity()
{
  if(_requestedActivity == _configParams.objectTapInteractionActivityName)
  {
    _requestedActivity.clear();
  }
}

} // namespace Cozmo
} // namespace Anki
