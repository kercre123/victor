/**
* File: behaviorSystemManager.cpp
*
* Author: Kevin Karol
* Date:   8/17/2017
*
* Description: Manages and enforces the lifecycle and transitions
* of partso of the behavior system
*
* Copyright: Anki, Inc. 2017
**/

#include "engine/behaviorSystem/behaviorSystemManager.h"

#include "engine/audio/behaviorAudioClient.h"
#include "engine/actions/actionContainers.h"
#include "engine/behaviorSystem/activities/activities/iActivity.h"
#include "engine/behaviorSystem/activities/activities/activityFactory.h"
#include "engine/behaviorSystem/behaviors/iBehavior.h"
#include "engine/behaviorSystem/behaviorContainer.h"
#include "engine/behaviorSystem/iBSRunnable.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/viz/vizManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "clad/types/behaviorSystem/reactionTriggers.h"

#include "util/cpuProfiler/cpuProfiler.h"


namespace Anki {
namespace Cozmo {

// Forward declaration
class IReactionTriggerStrategy;

  
namespace{
  
}

/////////
// Running/Resume implementation
/////////
  
// struct which defines information about the currently running behavior
struct BehaviorRunningInfo{
  void SetCurrentBehavior(IBehaviorPtr newScoredBehavior){_currentBehavior = newScoredBehavior;}
  // return the active behavior based on the category set in the struct
  IBehaviorPtr GetCurrentBehavior() const{return _currentBehavior;}

private:
  // only one behavior should be active at a time
  // either a scored behavior or a reactionary behavior
  IBehaviorPtr _currentBehavior;
};

/////////
// BehaviorSystemManager implementation
/////////
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSystemManager::BehaviorSystemManager(Robot& robot)
: _isInitialized(false)
, _runningInfo(new BehaviorRunningInfo())
, _behaviorContainer(new BehaviorContainer(robot, robot.GetContext()->GetDataLoader()->GetBehaviorJsons()))
, _audioClient( new Audio::BehaviorAudioClient(robot) )
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSystemManager::~BehaviorSystemManager()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::InitConfiguration(Robot& robot, const Json::Value& behaviorSystemConfig)
{
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(!_isInitialized, "BehaviorSystemManager.InitConfiguration.AlreadyInitialized");
  
  if (robot.HasExternalInterface())
  {
    InitializeEventHandlers(robot);
  }
  
  // Init behavior audio client after parsing config because it relies on parsed values
  _audioClient->Init();
  
  _isInitialized = true;
  

  ActivityType type = IActivity::ExtractActivityTypeFromConfig(behaviorSystemConfig);
  
  _baseActivity.reset(ActivityFactory::CreateActivity(robot,
                                                      type,
                                                      behaviorSystemConfig));
  DEV_ASSERT(_baseActivity != nullptr, "BehaviorSystemManager.InitConfiguration.NoBaseActivity");
  
  return RESULT_OK;
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/** TODO - move this directly into behavior container
void BehaviorSystemManager::LoadBehaviorsIntoFactory()
{
  // Load Scored Behaviors
  const auto& behaviorData = _robot.GetContext()->GetDataLoader()->GetBehaviorJsons();
  for( const auto& behaviorIDJsonPair : behaviorData )
  {
    const auto& behaviorID = behaviorIDJsonPair.first;
    const auto& behaviorJson = behaviorIDJsonPair.second;
    if (!behaviorJson.empty())
    {
      // PRINT_NAMED_DEBUG("BehaviorSystemManager.LoadBehavior", "Loading '%s'", fullFileName.c_str());
      const Result ret = CreateBehaviorFromConfiguration(behaviorJson);
      if ( ret != RESULT_OK ) {
        PRINT_NAMED_ERROR("Robot.LoadBehavior.CreateFailed",
                          "Failed to create a behavior for behavior id '%s'",
                          BehaviorIDToString(behaviorID));
      }
    }
    else
    {
      PRINT_NAMED_WARNING("Robot.LoadBehavior",
                          "Failed to read behavior file for behavior id '%s'",
                          BehaviorIDToString(behaviorID));
    }
    // don't print anything if we read an empty json
  }
  
  // If we didn't load any behaviors from data, there's no reason to check to
  // see if all executable behaviors have a 1-to-1 matching
  if(behaviorData.size() > 0){
    _behaviorContainer->VerifyExecutableBehaviors();
  }
}**/
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::InitializeEventHandlers(Robot& robot)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/**Result BehaviorSystemManager::CreateBehaviorFromConfiguration(const Json::Value& behaviorJson)
{
  // try to create behavior, name should be unique here
  IBehaviorPtr newBehavior = _behaviorContainer->CreateBehavior(behaviorJson, _robot);
  const Result ret = (nullptr != newBehavior) ? RESULT_OK : RESULT_FAIL;
  return ret;
}**/
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const IBehaviorPtr BehaviorSystemManager::GetCurrentBehavior() const{
  return GetRunningInfo().GetCurrentBehavior();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSystemManager::SwitchToBehaviorBase(Robot& robot, BehaviorRunningInfo& nextBehaviorInfo)
{
  BehaviorRunningInfo oldInfo = GetRunningInfo();
  IBehaviorPtr nextBehavior = nextBehaviorInfo.GetCurrentBehavior();

  StopAndNullifyCurrentBehavior();
  bool initSuccess = true;
  if( nullptr != nextBehavior ) {
    ANKI_VERIFY(nextBehavior->IsRunnable(robot),
                "BehaviorSystemManager.SwitchToBehaviorBase.BehaviorNotRunnable",
                "Behavior %s returned but it's not runnable",
                BehaviorIDToString(nextBehavior->GetID()));
    const Result initRet = nextBehavior->Init();
    if ( initRet != RESULT_OK ) {
      // the previous behavior has been told to stop, but no new behavior has been started
      PRINT_NAMED_ERROR("BehaviorSystemManager.SetCurrentBehavior.InitFailed",
                        "Failed to initialize %s behavior.",
                        BehaviorIDToString(nextBehavior->GetID()));
      nextBehaviorInfo.SetCurrentBehavior(nullptr);
      initSuccess = false;
    }
  }
  
  /** Currently managed by Behavior Manager
   if( nullptr != nextBehavior ) {
    VIZ_BEHAVIOR_SELECTION_ONLY(
      robot.GetContext()->GetVizManager()->SendNewBehaviorSelected(
           VizInterface::NewBehaviorSelected( nextBehavior->GetID() )));
  }**/
  
  SetRunningInfo(robot, nextBehaviorInfo);
  SendDasTransitionMessage(robot, oldInfo, GetRunningInfo());
  
  return initSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::SendDasTransitionMessage(Robot& robot,
                                                     const BehaviorRunningInfo& oldBehaviorInfo,
                                                     const BehaviorRunningInfo& newBehaviorInfo)
{
  // If we don't have an external interface (Unit tests), bail early; we can't setup callbacks
  if (!robot.HasExternalInterface()) {
    return;
  }
  
  const IBehaviorPtr oldBehavior = oldBehaviorInfo.GetCurrentBehavior();
  const IBehaviorPtr newBehavior = newBehaviorInfo.GetCurrentBehavior();
  
  
  BehaviorID oldBehaviorID = nullptr != oldBehavior ? oldBehavior->GetID()  : BehaviorID::Wait;
  BehaviorID newBehaviorID = nullptr != newBehavior ? newBehavior->GetID()  : BehaviorID::Wait;
  
  Anki::Util::sEvent("robot.behavior_transition",
                     {{DDATA,
                       BehaviorIDToString(oldBehaviorID)}},
                       BehaviorIDToString(newBehaviorID));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::UpdateActiveBehavior(Robot& robot)
{
  // the current behavior has to be running. Otherwise current should be nullptr
  DEV_ASSERT(GetRunningInfo().GetCurrentBehavior() == nullptr ||
             GetRunningInfo().GetCurrentBehavior()->IsRunning(),
             "BehaviorSystemManager.GetDesiredActiveBehavior.CurrentBehaviorIsNotRunning");
 
  DEV_ASSERT(_baseActivity != nullptr, "BehaviorSystem.GetDesiredActiveBehavior.MissingBaseActivity");
  // ask the current activity for the next behavior
  IBehaviorPtr nextBehavior = _baseActivity->
        GetDesiredActiveBehavior(robot, GetRunningInfo().GetCurrentBehavior());
  if(nextBehavior != GetRunningInfo().GetCurrentBehavior()){
    BehaviorRunningInfo scoredInfo;
    scoredInfo.SetCurrentBehavior(nextBehavior);
    SwitchToBehaviorBase(robot, scoredInfo);
  }
  
  if (nextBehavior != nullptr) {
    // We have a current behavior, update it.
    const IBehavior::Status status = nextBehavior->Update();
    
    switch(status)
    {
      case IBehavior::Status::Running:
        // Nothing to do! Just keep on truckin'....
        break;
        
      case IBehavior::Status::Complete:
        // behavior is complete, switch to null (will also handle stopping current). If it was reactionary,
        // switch now to give the last behavior a chance to resume (if appropriate)
        PRINT_CH_DEBUG("Behaviors", "BehaviorSystemManager.Update.BehaviorComplete",
                       "Behavior '%s' returned  Status::Complete",
                       BehaviorIDToString(nextBehavior->GetID()));
        FinishCurrentBehavior(robot, nextBehavior);
        break;
        
      case IBehavior::Status::Failure:
        PRINT_NAMED_ERROR("BehaviorSystemManager.Update.FailedUpdate",
                          "Behavior '%s' failed to Update().",
                          BehaviorIDToString(nextBehavior->GetID()));
        // same as the Complete case
        FinishCurrentBehavior(robot, nextBehavior);
        break;
    } // switch(status)
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::SetRunningInfo(Robot& robot, const BehaviorRunningInfo& newInfo)
{
  *_runningInfo = newInfo;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::Update(Robot& robot)
{
  ANKI_CPU_PROFILE("BehaviorSystemManager::Update");
  
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorSystemManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
  
  // Update the currently running activity
  _baseActivity->Update(robot);
  
  // Get the active behavior from the activity stack and update it
  UpdateActiveBehavior(robot);

    
  return lastResult;
} // Update()



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::StopAndNullifyCurrentBehavior()
{
  IBehaviorPtr currentBehavior = GetRunningInfo().GetCurrentBehavior();
  
  if ( nullptr != currentBehavior && currentBehavior->IsRunning() ) {
    currentBehavior->Stop();
  }
  
  GetRunningInfo().SetCurrentBehavior(nullptr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::FinishCurrentBehavior(Robot& robot, IBehaviorPtr currentBehavior)
{
  BehaviorRunningInfo nullInfo;
  SwitchToBehaviorBase(robot, nullInfo);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorClass BehaviorSystemManager::GetBehaviorClass(IBehaviorPtr behavior) const
{
  return behavior->GetClass();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByID(BehaviorID behaviorID) const
{
  return _behaviorContainer->FindBehaviorByID(behaviorID);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  return _behaviorContainer->FindBehaviorByExecutableType(type);
}


} // namespace Cozmo
} // namespace Anki
