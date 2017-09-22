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

#include "engine/aiComponent/behaviorComponent/behaviorSystemManager.h"

#include "engine/actions/actionContainers.h"
#include "engine/aiComponent/behaviorComponent/activities/activities/iActivity.h"
#include "engine/aiComponent/behaviorComponent/activities/activities/activityFactory.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/iBSRunnable.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/viz/vizManager.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"

#include "clad/types/behaviorSystem/reactionTriggers.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/boundedWhile.h"

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
const IBehaviorPtr BehaviorSystemManager::GetCurrentBehavior() const{
  return GetRunningInfo().GetCurrentBehavior();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::UpdateRunnableStack(BehaviorExternalInterface& behaviorExternalInterface)
{
  // the current behavior has to be running. Otherwise current should be nullptr
  DEV_ASSERT(GetRunningInfo().GetCurrentBehavior() == nullptr ||
             GetRunningInfo().GetCurrentBehavior()->IsRunning(),
             "BehaviorSystemManager.GetDesiredActiveBehavior.CurrentBehaviorIsNotRunning");
  
  DEV_ASSERT(_baseActivity != nullptr, "BehaviorSystem.GetDesiredActiveBehavior.MissingBaseActivity");
  _baseActivity->Update(behaviorExternalInterface);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::SetRunningInfo(const BehaviorRunningInfo& newInfo)
{
  *_runningInfo = newInfo;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSystemManager::Update(BehaviorExternalInterface& behaviorExternalInterface)
{
  ANKI_CPU_PROFILE("BehaviorSystemManager::Update");
  
  
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("BehaviorSystemManager.Update.NotInitialized", "");
    //return RESULT_FAIL;
  }
  
  // Update the currently running activity
  _baseActivity->Update(behaviorExternalInterface);
  
  // Check to see what the current active behavior should be
  UpdateRunnableStack(behaviorExternalInterface);
  
} // Update()


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
