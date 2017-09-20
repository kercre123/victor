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

#include "engine/aiComponent/behaviorSystem/behaviorSystemManager.h"

#include "engine/aiComponent/behaviorSystem/behaviorAudioClient.h"
#include "engine/actions/actionContainers.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorSystem/activities/activities/activityFactory.h"
#include "engine/aiComponent/behaviorSystem/behaviorContainer.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorSystem/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorSystem/iBSRunnable.h"
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
BehaviorSystemManager::BehaviorSystemManager()
: _isInitialized(false)
, _runningInfo(new BehaviorRunningInfo())
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSystemManager::~BehaviorSystemManager()
{
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::InitConfiguration(BehaviorExternalInterface& behaviorExternalInterface,
                                                const Json::Value& behaviorSystemConfig)
{
  // do not support multiple initialization. A) we don't need it, B) it's easy to forget to clean up everything properly
  // when adding new stuff. During my refactoring I found several variables that were not properly reset, so
  // potentially double Init was never supported
  DEV_ASSERT(!_isInitialized, "BehaviorSystemManager.InitConfiguration.AlreadyInitialized");
  
  // Init behavior audio client after parsing config because it relies on parsed values
  _audioClient = std::make_unique<Audio::BehaviorAudioClient>(behaviorExternalInterface,
                                                              behaviorExternalInterface.GetRobot().GetRobotAudioClient());
  _audioClient->Init(behaviorExternalInterface);
  // Assumes there's only one instance of the behavior external Intarfec
  _behaviorExternalInterface = &behaviorExternalInterface;
  
  
  _isInitialized = true;
  

  ActivityType type = IActivity::ExtractActivityTypeFromConfig(behaviorSystemConfig);
  
  _baseActivity.reset(ActivityFactory::CreateActivity(behaviorExternalInterface,
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
bool BehaviorSystemManager::SwitchToBehaviorBase(BehaviorExternalInterface& behaviorExternalInterface,
                                                 BehaviorRunningInfo& nextBehaviorInfo)
{
  BehaviorRunningInfo oldInfo = GetRunningInfo();
  IBehaviorPtr nextBehavior = nextBehaviorInfo.GetCurrentBehavior();

  StopAndNullifyCurrentBehavior(behaviorExternalInterface);
  bool initSuccess = true;
  if( nullptr != nextBehavior ) {
    ANKI_VERIFY(nextBehavior->IsRunnable(behaviorExternalInterface),
                "BehaviorSystemManager.SwitchToBehaviorBase.BehaviorNotRunnable",
                "Behavior %s returned but it's not runnable",
                BehaviorIDToString(nextBehavior->GetID()));
    const Result initRet = nextBehavior->OnActivated(behaviorExternalInterface);
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
  
  SetRunningInfo(nextBehaviorInfo);
  auto externalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if(externalInterface != nullptr){
    SendDasTransitionMessage(oldInfo, GetRunningInfo());
  }
  
  return initSuccess;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::SendDasTransitionMessage(const BehaviorRunningInfo& oldBehaviorInfo,
                                                     const BehaviorRunningInfo& newBehaviorInfo)
{
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
void BehaviorSystemManager::UpdateActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  // the current behavior has to be running. Otherwise current should be nullptr
  DEV_ASSERT(GetRunningInfo().GetCurrentBehavior() == nullptr ||
             GetRunningInfo().GetCurrentBehavior()->IsRunning(),
             "BehaviorSystemManager.GetDesiredActiveBehavior.CurrentBehaviorIsNotRunning");
 
  DEV_ASSERT(_baseActivity != nullptr, "BehaviorSystem.GetDesiredActiveBehavior.MissingBaseActivity");
  // ask the current activity for the next behavior
  IBehaviorPtr nextBehavior = _baseActivity->
        GetDesiredActiveBehavior(behaviorExternalInterface, GetRunningInfo().GetCurrentBehavior());
  if(nextBehavior != GetRunningInfo().GetCurrentBehavior()){
    BehaviorRunningInfo scoredInfo;
    scoredInfo.SetCurrentBehavior(nextBehavior);
    SwitchToBehaviorBase(behaviorExternalInterface, scoredInfo);
  }
  
  if (nextBehavior != nullptr) {
    // We have a current behavior, update it.
    const IBehavior::Status status = nextBehavior->Update(behaviorExternalInterface);
    
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
        FinishCurrentBehavior(behaviorExternalInterface, nextBehavior);
        break;
        
      case IBehavior::Status::Failure:
        PRINT_NAMED_ERROR("BehaviorSystemManager.Update.FailedUpdate",
                          "Behavior '%s' failed to Update().",
                          BehaviorIDToString(nextBehavior->GetID()));
        // same as the Complete case
        FinishCurrentBehavior(behaviorExternalInterface, nextBehavior);
        break;
    } // switch(status)
  }

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::SetRunningInfo(const BehaviorRunningInfo& newInfo)
{
  *_runningInfo = newInfo;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorSystemManager::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  ANKI_CPU_PROFILE("BehaviorSystemManager::Update");
  
  Result lastResult = RESULT_OK;
    
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorSystemManager.Update.NotInitialized", "");
    return RESULT_FAIL;
  }
  
  // Update the currently running activity
  _baseActivity->Update(behaviorExternalInterface);
  
  // Get the active behavior from the activity stack and update it
  UpdateActiveBehavior(behaviorExternalInterface);

    
  return lastResult;
} // Update()



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::StopAndNullifyCurrentBehavior(BehaviorExternalInterface& behaviorExternalInterface)
{
  IBehaviorPtr currentBehavior = GetRunningInfo().GetCurrentBehavior();
  
  if ( nullptr != currentBehavior && currentBehavior->IsRunning() ) {
    currentBehavior->OnDeactivated(behaviorExternalInterface);
  }
  
  GetRunningInfo().SetCurrentBehavior(nullptr);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::FinishCurrentBehavior(BehaviorExternalInterface& behaviorExternalInterface, IBehaviorPtr currentBehavior)
{
  BehaviorRunningInfo nullInfo;
  SwitchToBehaviorBase(behaviorExternalInterface, nullInfo);
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByID(BehaviorID behaviorID) const
{
  if(_behaviorExternalInterface != nullptr){
    return _behaviorExternalInterface->GetBehaviorContainer().FindBehaviorByID(behaviorID);
  }else{
    IBehaviorPtr empty;
    return empty;
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr BehaviorSystemManager::FindBehaviorByExecutableType(ExecutableBehaviorType type) const
{
  if(_behaviorExternalInterface != nullptr){
    return _behaviorExternalInterface->GetBehaviorContainer().FindBehaviorByExecutableType(type);
  }else{
    IBehaviorPtr empty;
    return empty;
  }
}


} // namespace Cozmo
} // namespace Anki
