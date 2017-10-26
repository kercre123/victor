/**
 * File: ActivityFreeplay.cpp
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
#include "engine/aiComponent/behaviorComponent/activities/activities/activityFreeplay.h"

#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/iActivityStrategy.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/iBehavior.h"
#include "engine/aiComponent/freeplayDataTracker.h"
#include "engine/ankiEventUtil.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/needsSystem/needsManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

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
static const char* kActivitiesConfigKey           = "subActivities";
static const char* activityIDKey                  = "behaviorID";
static const char* activityPriorityKey            = "activityPriority";
  
// Keys to identify activities after put down dispatch
static const char* kFaceAndCubeConfigKey  = "faceAndCubeActivityName";
static const char* kFaceOnlyConfigKey     = "faceOnlyActivityName";
static const char* kCubeOnlyConfigKey     = "cubeOnlyActivityName";
static const char* kNoFaceNoCubeConfigKey = "noFaceNoCubeActivityName";


#if REMOTE_CONSOLE_ENABLED

ActivityFreeplay* defaultFeeplayActivity = nullptr;
void ActivityFreeplaySetDebugActivity(BehaviorID activityID)
{
  if ( nullptr != defaultFeeplayActivity ) {
    const char* activityIDString = BehaviorIDToString(activityID);
    PRINT_CH_INFO("Behaviors", "ActivityFreeplay.DebugSetActivity", "Setting activity to '%s'",
                  activityIDString != nullptr ? activityIDString : "<INVALID>");

    defaultFeeplayActivity->SetConsoleRequestedActivity(activityID);
  } else {
    PRINT_NAMED_WARNING("ActivityFreeplayCycleActivity", "No default ActivityFreeplay. Can't cycle activities.");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO:(bn) We could remove the other functions... but QA may be using them
void ActivitySetDebug( ConsoleFunctionContextRef context ) {
  const char* activityIdString = ConsoleArg_Get_String(context, "behaviorID");
  ActivityFreeplaySetDebugActivity(BehaviorIDFromString(activityIdString));
}
CONSOLE_FUNC( ActivitySetDebug, "ActivityFreeplay", const char* activityID );

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPNothingToDo( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_NothingToDo);
}
CONSOLE_FUNC( ActivitySetFPNothingToDo, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPHiking( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_Hiking);
}
CONSOLE_FUNC( ActivitySetFPHiking, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPPlayWithHumans( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_PlayWithHumans);
}
CONSOLE_FUNC( ActivitySetFPPlayWithHumans, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPPlayAlone( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_PlayAlone);
}
CONSOLE_FUNC( ActivitySetFPPlayAlone, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPSocialize( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_Socialize);
}
CONSOLE_FUNC( ActivitySetFPSocialize, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetFPBuildPyramid( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_BuildPyramid);
}
CONSOLE_FUNC( ActivitySetFPBuildPyramid, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityClearSetting( ConsoleFunctionContextRef context ) {
  // CLEAR SETTING NO LONGER WORKS
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_Feeding);
}
CONSOLE_FUNC( ActivityClearSetting, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetSinging( ConsoleFunctionContextRef context ) {
  ActivityFreeplaySetDebugActivity(BehaviorID::Activity_Singing);
}
CONSOLE_FUNC( ActivitySetSinging, "ActivityFreeplay" );
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivitySetSingingCooldown(ConsoleFunctionContextRef context)
{
  if(nullptr != defaultFeeplayActivity)
  {
    const float cooldown = ConsoleArg_Get_Float(context, "cooldown_ms");
    defaultFeeplayActivity->SetActivityStrategyCooldown(UnlockId::Count, BehaviorID::Activity_Singing, cooldown);
  }
}
CONSOLE_FUNC( ActivitySetSingingCooldown, "ActivityFreeplay", float cooldown_ms);

#endif // REMOTE_CONSOLE_ENABLED

};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFreeplay::ActivityFreeplay(const Json::Value& config)
: IActivity(config)
, _currentActivityPtr(nullptr)
, _requestedActivity(BehaviorID::Wait)
, _debugConsoleRequestedActivity(BehaviorID::Wait)
, _subID(BehaviorID::Wait)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityFreeplay::~ActivityFreeplay()
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
void ActivityFreeplay::InitActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  _behaviorExternalInterface = &behaviorExternalInterface;
  CreateFromConfig(behaviorExternalInterface, _config);
  
  
  behaviorExternalInterface.GetStateChangeComponent().SubscribeToTags(this,
      {
        ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged
      });
  
  #if ( ANKI_DEV_CHEATS )
    {
      if ( !defaultFeeplayActivity ) {
        defaultFeeplayActivity = this;
      }
    }
  #endif
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::SetActivityIDFromSubActivity(BehaviorID activityID){
  _subID = activityID;
  _freeplayIDString = std::string(IActivity::GetIDStr()) + "_" + BehaviorIDToString(_subID);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ActivityFreeplay::Update_Legacy(BehaviorExternalInterface& behaviorExternalInterface)
{
  // Event Handling
  const auto& stateChangeComp = behaviorExternalInterface.GetStateChangeComponent();
  for(const auto& event: stateChangeComp.GetEngineToGameEvents()){
    if(event.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::RobotOffTreadsStateChanged){
      auto& msg = event.GetData().Get_RobotOffTreadsStateChanged();
      // if we return to OnTreads and we are in freeplay without sparks, clear the activity to pick a new one as soon
      // as we regain control.
      const bool onTreads = msg.treadsState == OffTreadsState::OnTreads;
      if ( onTreads )
      {
        if ( _currentActivityPtr != nullptr)
        {
          const UnlockId curActivitySpark = _currentActivityPtr->GetRequiredSpark();
          const bool isSparkless = (curActivitySpark == UnlockId::Count);
          const bool isRunningDebugActivity = (_currentActivityPtr->GetID() ==
                                               _debugConsoleRequestedActivity);
          const bool isSevereNeedsActivity =
          (_currentActivityPtr->GetID() == BehaviorID::Activity_NeedsSevereLowEnergy) ||
          (_currentActivityPtr->GetID() == BehaviorID::Activity_NeedsSevereLowRepair);
          
          if ( isSparkless && !isRunningDebugActivity && !isSevereNeedsActivity)
          {
            PRINT_CH_INFO("Behaviors", "ActivityFreeplay.RobotOffTreadsStateChanged.KickingOutActivityOnPutDown",
                          "Kicking out '%s' on put down so we pick up a new one",
                          EnumToString(_currentActivityPtr->GetID()));
            
            // note: this will set the activity on cooldown, which may not be desired if it didn't run for some time
            
            // stop current activity
            _currentActivityPtr->OnDeactivated(*_behaviorExternalInterface);
            _currentActivityPtr = nullptr;
          }
        }
      }
      
      
    }
  }
  
  
  
  auto result = Result::RESULT_OK;
  if(_currentActivityPtr != nullptr){
    result = _currentActivityPtr->Update_Legacy(behaviorExternalInterface);
  }
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::OnDeactivatedActivity(BehaviorExternalInterface& behaviorExternalInterface)
{
  if ( _currentActivityPtr ) {
    _currentActivityPtr->OnDeactivated(behaviorExternalInterface);
    _currentActivityPtr = nullptr;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::CreateFromConfig(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
{
  // TODO: rename _activities
  _activities.clear();
  
  // read every activity and create them with their own config
  const Json::Value& activityPriorities = config[kActivitiesConfigKey];
  if ( !activityPriorities.isNull() )
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    // Get the activity jsons
    const auto& activityData = robot.GetContext()->GetDataLoader()->GetBehaviorJsons();
    /**DEV_ASSERT_MSG(activityPriorities.size() == activityData.size(),
                   "ActivityFreeplay.CreateFromConfig.ActivitySizeMismatch",
                   "There are %d priorities and %d activities defined",
                   static_cast<int>(activityPriorities.size()),
                   static_cast<int>(activityData.size()));**/
    
    // Iterate through the activities
    uint debugLastActivityPriority = 0;
    
    
    // reserve room for activites (note we reserve 1 per activity as upper bound,
    //                             even if some activites fall in the same unlockId)
    _activities.reserve( activityPriorities.size() );
  
    // iterate all activity names in priority order, grab their data, and load them
    // into the container

    
    Json::Value::const_iterator it = activityPriorities.begin();
    const Json::Value::const_iterator end = activityPriorities.end();
    
    for(; it != end; ++it )
    {
      // Get the activityID and priority for the subActivity
      std::string activityID_str = JsonTools::ParseString(*it, activityIDKey,
                                                         "ActivityFreeplay.CreateFromConfig.ActivityID.KeyMissing");
      
      BehaviorID activityID = BehaviorIDFromString(activityID_str);

      const uint activityPriority = JsonTools::ParseUint8(*it, activityPriorityKey,
                                                          "ActivityFreeplay.CreateFromConfig.ActivityPriority");
      
      DEV_ASSERT_MSG(activityPriority >= debugLastActivityPriority,
                     "ActivityFreeplay.CreateFromConfig.ActivityPrioritiesOutOfOrder",
                     "Activity id %s", BehaviorIDToString(activityID));
      debugLastActivityPriority = activityPriority;
      
      
      
      // Locate the file data by its file name
      const auto& dataIter = activityData.find(activityID);
      
      DEV_ASSERT_MSG(dataIter != activityData.end(),
                     "ActivityFreeplay.CreateFromConfig.ActivityDataNotFound",
                     "No file found named %s", BehaviorIDToString(activityID));
      
      // Gather the appropriate data from the config to generate the activity
      const Json::Value& activityConfig = dataIter->second;
    
      BehaviorID activityType =  ICozmoBehavior::ExtractBehaviorIDFromConfig(activityConfig);
      
      
      ICozmoBehaviorPtr subActivity = behaviorExternalInterface.
                            GetBehaviorContainer().FindBehaviorByID(activityType);
      
      std::shared_ptr<IActivity> castActivity = std::static_pointer_cast<IActivity>(subActivity);
      
      // add the activity to the vector of activities with the same spark
      const UnlockId subActivitySpark = castActivity->GetRequiredSpark();
      _activities[subActivitySpark].emplace_back( castActivity );

      
    }
  }
  
  // DebugPrintActivities();
  
  // clear current activity
  _currentActivityPtr = nullptr;
  
  // load desired activites
  {
    const Json::Value& desiredActivities = config[kDesiredActivityNamesConfigKey];
    using namespace JsonTools;
    const std::string& debugName = "ActivityFreeplay.Constructor";
  
    // parse desired activities from objects
    _configParams.faceAndCubeActivity  = BehaviorIDFromString(
                      ParseString(desiredActivities, kFaceAndCubeConfigKey,  debugName));
    _configParams.faceOnlyActivity     = BehaviorIDFromString(
                      ParseString(desiredActivities, kFaceOnlyConfigKey,     debugName));
    _configParams.cubeOnlyActivity     = BehaviorIDFromString(
                      ParseString(desiredActivities, kCubeOnlyConfigKey,     debugName));
    _configParams.noFaceNoCubeActivity = BehaviorIDFromString(
                      ParseString(desiredActivities, kNoFaceNoCubeConfigKey, debugName));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityFreeplay::PickNewActivityForSpark(BehaviorExternalInterface& behaviorExternalInterface, UnlockId spark, bool isCurrentAllowed)
{
  // find activities that can run for the given spark
  const SparkToActivitiesTable::iterator activityVectorIt = _activities.find(spark);
  if ( activityVectorIt != _activities.end() )
  {
    ActivityVector& activitiesForThisSpark = activityVectorIt->second;
  
    // can't have the spark registered but have no activities in the container, programmer error
    DEV_ASSERT(!activitiesForThisSpark.empty(), "ActivityFreeplay.RegisteredSparkHasNoActivities");
    
    IActivity* newActivity = nullptr;
    
    // activities are sorted by priority within this spark.
    // Iterate from first to last until one wants to run
    for(auto& activity : activitiesForThisSpark)
    {
      const IActivityStrategy& selectionStrategy = activity->GetStrategy();
      
      // if there's a debug console requested activity, force to pick that one
      if ( _debugConsoleRequestedActivity != BehaviorID::Wait)
      {
        // skip if name doesn't match
        if ( activity->GetID() != _debugConsoleRequestedActivity ) {
          continue;
        }
      }
      else if (_requestedActivity != BehaviorID::Wait)
      {
        // skip if name doesn't match
        if ( activity->GetID() != _requestedActivity ) {
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
          const bool wantsToEnd = selectionStrategy.WantsToEnd(behaviorExternalInterface, lastTimeStarted);
          if ( wantsToEnd ) {
            continue;
          }
        }
        else
        {
          // it's not running, check if we want to start
          const float lastTimeStarted = activity->GetLastTimeStartedSecs();
          const float lastTimeFinished = activity->GetLastTimeStoppedSecs();
          const bool wantsToStart = selectionStrategy.WantsToStart(behaviorExternalInterface, lastTimeFinished, lastTimeStarted);
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
    
      // DAS - Legacy event from when activities were called goals
      // please don't change or queries will break
      LOG_EVENT("AIGoalEvaluator.NewGoalSelected",
        "Switched goal from '%s' to '%s' (spark '%s')",
        _currentActivityPtr ? EnumToString(_currentActivityPtr->GetID()) : "no activity",
        newActivity         ? EnumToString(newActivity->GetID()) : "no activity",
        EnumToString(spark) );
    
      // log
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.PickNewActivityForSpark.Switched",
        "Switched activity from '%s' to '%s'",
          _currentActivityPtr ? EnumToString(_currentActivityPtr->GetID()) : "no activity",
          newActivity         ? EnumToString(newActivity->GetID()) : "no activity" );
      // set new activity and timestamp
      if ( _currentActivityPtr ) {
        _currentActivityPtr->OnDeactivated(behaviorExternalInterface);
      }
      _currentActivityPtr = newActivity;
      if ( _currentActivityPtr ) {
        SetActivityIDFromSubActivity(_currentActivityPtr->GetID());
        _currentActivityPtr->OnActivated(behaviorExternalInterface);
      }
    }
    else if ( _currentActivityPtr != nullptr )
    {
      // we picked the same activity
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.PickNewActivityForSpark",
        "Current activity '%s' is still the best one to run for spark '%s', %s",
          _currentActivityPtr ? EnumToString(_currentActivityPtr->GetID()) : "no activity",
          EnumToString(spark),
          ( _debugConsoleRequestedActivity == _currentActivityPtr->GetID() ) ? "forced from debug" : "(not debug forced)");
    }
    else
    {
        // we did not select any activity. This means we are not going to run behaviors. It should not happen if at
        // least a fallback activity was defined, which can be done through proper strategy selection in data, so we
        // shouldn't correct it here, other than warn if it ever happens
        PRINT_NAMED_WARNING("ActivityFreeplay.PickNewActivityForSpark.NoActivitySelected",
          "None of the activities available for spark '%s' wants to run.",
          EnumToString(spark));
    }
    
    return selectedNew;
  }
  else
  {
    PRINT_NAMED_WARNING("ActivityFreeplay.PickNewActivityForSpark.NoActivityAvailableForSpark",
                        "No activity available for spark '%s'", EnumToString(spark));
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::CalculateDesiredActivityFromObjects(BehaviorExternalInterface& behaviorExternalInterface)
{
  // note that here we rely on "since we delocalized"
  
  // check if we have discovered new faces since we delocalized (last face's origin should be current)
  Pose3d lastFacePose;
  const TimeStamp_t lastFaceSeenTimestamp = behaviorExternalInterface.GetFaceWorld().GetLastObservedFace(lastFacePose);
  bool hasNewFace = false;
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    (lastFaceSeenTimestamp>0) && robot.IsPoseInWorldOrigin(lastFacePose);
  }

  // check if we have discovered new cubes since we delocalized (any not unknown in current origin)
  BlockWorldFilter cubeFilter;
  cubeFilter.SetAllowedFamilies({{ ObjectFamily::Block, ObjectFamily::LightCube }});
  const bool hasNewCube = (behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObject(cubeFilter) != nullptr);
  
  // depending on what we see, request the activity we want
  if ( hasNewFace && hasNewCube ) {
    _requestedActivity = _configParams.faceAndCubeActivity;
  } else if ( hasNewFace ) {
    _requestedActivity = _configParams.faceOnlyActivity;
  }  else if ( hasNewCube ) {
    _requestedActivity = _configParams.cubeOnlyActivity;
  }  else {
    _requestedActivity = _configParams.noFaceNoCubeActivity;
  }

  // log event to das - note robot.goal is a legacy name for Activities left
  // in place so that data is queriable - please do not change
  Util::sEventF("robot.goal_from_face_and_cube",
                {{DDATA, BehaviorIDToString(_requestedActivity)}},
                "%d:%d",
                hasNewFace ? 1 : 0,
                hasNewCube ? 1 : 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ActivityFreeplay::GetDesiredActiveBehaviorInternal(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior)
{
  // returned variable
  ICozmoBehaviorPtr chosenBehavior = nullptr;
  const bool hasActivityAtStart = (_currentActivityPtr != nullptr);
  
  // see if anything causes an activity change
  bool getNewActivity = false;
  bool isCurrentAllowedToBePicked = true;
  bool hasChosenBehavior = false;
  
  // check if we have a debugConsole activity
  if ( _debugConsoleRequestedActivity != BehaviorID::Wait)
  {
    // if we are not running the debug requested activity, ask to change. It should be picked if name is available
    if ( (!_currentActivityPtr) || _currentActivityPtr->GetID() != _debugConsoleRequestedActivity ) {
      getNewActivity = true;
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.DebugForcedChange",
        "Picking new activity because debug is forcing '%s'", BehaviorIDToString(_debugConsoleRequestedActivity));
    }
  }
  else if ( _requestedActivity != BehaviorID::Wait )
  {
    // if we are not running the requested activity, ask to change. It should be picked if name is available
    if ( (!_currentActivityPtr) || _currentActivityPtr->GetID() != _requestedActivity ) {
      getNewActivity = true;
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.RequestedActivity",
        "Picking new activity because '%s' was requested", BehaviorIDToString(_requestedActivity));
    }
  }
  else
  {
    // no debug requests
    // get a new activity if the current behavior finished succesfully or sparks changed
    
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    const bool changeDueToSpark = robot.GetBehaviorManager().ShouldSwitchToSpark();
    if ( (nullptr == _currentActivityPtr) || changeDueToSpark )
    {
      // the active spark does not match the current activity, pick the first activity available for the spark
      // this covers both going into and out of sparks
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.NullOrChangeDueToSpark",
                    "Picking new activity to match spark '%s'", EnumToString(robot.GetBehaviorManager().GetRequestedSpark()));
      getNewActivity = true;
    }
    else if ( currentRunningBehavior == nullptr &&
             behaviorExternalInterface.HasNeedsManager() &&
             behaviorExternalInterface.GetNeedsManager().IsPendingSparksRewardMsg())
    {
      // check with the current activity if it wants to end
      // note that we will also ask the current activity this when picking new activities. This additional check is not
      // superflous: since lower priority activities are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the ActivityFreeplay.
      const float activityStartTime = _currentActivityPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(behaviorExternalInterface, activityStartTime);
      if ( currentWantsToEnd )
      {
        getNewActivity = true;
        PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.BehaviorAndActivityEnded",
          "Picking new activity because '%s' wants to end, and behavior finished",
          BehaviorIDToString(_currentActivityPtr->GetID()));
      }
    }
  }
  
  // if we don't want to pick a new activity, ask for a new behavior now. The reason is that if we pick a different
  // behavior than the current one, we can also cause an activity change by timeout or if the current activity didn't select
  // a good behavior
  if ( !getNewActivity )
  {
    // pick behavior
    DEV_ASSERT(nullptr!=_currentActivityPtr, "ActivityFreeplay.CurrentActivityCantBeNull");
    chosenBehavior = _currentActivityPtr->GetDesiredActiveBehavior(behaviorExternalInterface, currentRunningBehavior);
    hasChosenBehavior = true;

    // if the picked behavior is not good, we want a new activity too
    if (chosenBehavior == nullptr)
    {
      getNewActivity = true;
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.NoBehaviorChosenWhileRunning",
        "Picking new activity because '%s' chose behavior '%s'. This activity is not allowed to be repicked.",
        BehaviorIDToString(_currentActivityPtr->GetID()),
        chosenBehavior ? BehaviorIDToString(chosenBehavior->GetID()) : "(null)" );
    }
    // The second check here is to prevent checking WantsToEnd while a sparks reward interlude behavior
    // is pending, because we don't want to end if it is pending
    else if (!chosenBehavior->IsActivated() &&
             behaviorExternalInterface.HasNeedsManager() &&
             behaviorExternalInterface.GetNeedsManager().IsPendingSparksRewardMsg())
    {
      // check with the current activity if it wants to end
      // note that we will also ask the current activity this when picking new activities. This additional check is not
      // superflous: since lower priority activities are not interrupted until they give up their slot or
      // they time out, this check allows them to give up the slot. This check is expected to be consistent throughout
      // the tick, so it should not have non-deterministic factors changing within a tick of the ActivityFreeplay.
      const float activityStartTime = _currentActivityPtr->GetLastTimeStartedSecs();
      const bool currentWantsToEnd = _currentActivityPtr->GetStrategy().WantsToEnd(behaviorExternalInterface, activityStartTime);
      if ( currentWantsToEnd )
      {
        getNewActivity = true;
        PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.NewBehaviorChosenWhileRunning",
          "Picking new activity because '%s' wants to end, and behavior finished",
          BehaviorIDToString(_currentActivityPtr->GetID()));
      }
    }
  }
  
  // if we have to pick new activity after having chosen a behavior, do it now
  if ( getNewActivity )
  {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    Robot& robot = behaviorExternalInterface.GetRobot();
    // Ensure that if a user cancels and re-trigger the same spark, the behavior exits and re-enters
    if(robot.GetBehaviorManager().GetActiveSpark() == robot.GetBehaviorManager().GetRequestedSpark()
       && robot.GetBehaviorManager().DidGameRequestSparkEnd()){
      isCurrentAllowedToBePicked = false;
      PRINT_CH_INFO("Behaviors","ActivityFreeplay.ChooseNextBehavior.SparkReselected", "Spark re-selected: none behavior will be selected");
    }
    
    // select new activity
    const UnlockId desiredSpark = robot.GetBehaviorManager().GetRequestedSpark();
    const bool changedActivity = PickNewActivityForSpark(behaviorExternalInterface,
                                                         desiredSpark,
                                                         isCurrentAllowedToBePicked);
    if ( changedActivity || !hasChosenBehavior )
    {
      // if we did change activities, flag as a change of spark too
      if ( changedActivity )
      {
        robot.GetBehaviorManager().SwitchToRequestedSpark();
        // warn if the activity selected does not match the desired spark
        if ( _currentActivityPtr && (_currentActivityPtr->GetRequiredSpark() != desiredSpark) ) {
          PRINT_NAMED_WARNING("ActivityFreeplay.ChooseNextBehavior.ActivityNotDesiredSpark",
            "The selected activity's required spark '%s' does not match the desired '%s'",
            EnumToString(_currentActivityPtr->GetRequiredSpark()),
            EnumToString(desiredSpark));
        }

        {
          // if this is a spark, then we are paused
          // NOTE: this may be invalid for voice commands, but not sure how we want to count those yet anyway
          const bool isPaused = (desiredSpark != UnlockId::Count);
          robot.GetAIComponent().GetFreeplayDataTracker().SetFreeplayPauseFlag(isPaused, FreeplayPauseFlag::Spark);
        }
      }
    
      // either we picked a new activity or we have the same but we didn't let it chose behavior before because
      // we wanted to check if there was a better one, so do it now
      if ( _currentActivityPtr )
      {
        chosenBehavior = _currentActivityPtr->GetDesiredActiveBehavior(behaviorExternalInterface, currentRunningBehavior);
        
        // if the first behavior chosen is null/None, the activity conditions to start didn't handle the current situation.
        // The activity will be asked to leave next frame if it continues to pick null/None.
        // TODO in this case we might want to apply a different cooldown, rather than the default for
        // the activity.
        if ( changedActivity && (chosenBehavior != nullptr))
        {
          PRINT_CH_INFO("Behaviors", "ActivityFreeplay.ChooseNextBehavior.NewActivityDidNotChooseBehavior",
            "The new activity '%s' picked no behavior. It probably didn't cover the same conditions as the behaviors.",
            BehaviorIDToString(_currentActivityPtr->GetID()));
        }
      } else {
        // selecting no activity is a valid situation. We need the lowest ordering activity to say it wants to end, so that
        // a higher ordering activity can take over. If none are avaiable, since the lowest ordering activity will give up
        // to give others a chance, there will be at least a frame in which there will be no activity selected.
        PRINT_CH_INFO("Behaviors","ActivityFreeplay.NoActivitySelected", "Picked no activity");
      }
    }
    
    // clear the requested activity since we picked an activity. Even if it wasn't selected, we are only going to give it
    // once chance of running
    if ( _requestedActivity != BehaviorID::Wait ) {
      _requestedActivity = BehaviorID::Wait;
    }
  }
  
  // not selecting a activity at the end of a frame is a valid situation, as long as we had one this frame. We want to
  // check however that if we didn't have a activity at start, and we have not picked a new one, Cozmo is effectively
  // lost, not knowing what to do.
  const bool hasActivityAtEnd = (_currentActivityPtr != nullptr);
  if ( !hasActivityAtStart && !hasActivityAtEnd ) {
    PRINT_NAMED_ERROR("ActivityFreeplay.NoActivityAvailableError", "There was no activity, and no activity was selected");
  }
  
  return chosenBehavior;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::DebugPrintActivities() const
{
  PRINT_CH_INFO("Behaviors", "ActivityFreeplay.PrintActivities",
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
      stringForUnlockId << " --> " << BehaviorIDToString(activity->GetID());
    }
    
    // log
    PRINT_CH_INFO("Behaviors", "ActivityFreeplay.PrintActivities", "%s", stringForUnlockId.str().c_str());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ActivityFreeplay::SetActivityStrategyCooldown(const UnlockId& unlockID,
                                           const BehaviorID& activityId,
                                           float cooldown_ms)
{
  const SparkToActivitiesTable::iterator iter = _activities.find(unlockID);
  if(iter != _activities.end())
  {
    for(auto& activity : iter->second)
    {
      if(activity->GetID() == activityId)
      {
        IActivityStrategy* strategy = activity->DevGetStrategy();
        if(strategy != nullptr)
        {
          strategy->SetCooldown(cooldown_ms);
        }
      }
    }
  }
}

} // namespace Cozmo
} // namespace Anki
