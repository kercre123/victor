/**
 * File: behaviorFindFaces.cpp
 *
 * Author: Lee Crippen / Brad Neuman / Matt Michini
 * Created: 2016-08-31
 *
 * Description: Delegates to the configured search behavior, terminating based on
 *              configurable stopping conditions.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/basicWorldInteractions/behaviorFindFaces.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/faceWorld.h"
#include "coretech/common/engine/jsonTools.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/console/consoleInterface.h"
#include "engine/actions/basicActions.h"

namespace Anki {
namespace Cozmo {

namespace {
const char* const kMaxFaceAgeToLookKey = "maxFaceAgeToLook_ms";
const char* const kStoppingConditionKey = "stoppingCondition";
const char* const kSearchBehaviorKey = "searchBehavior";
const char* const kTimeoutKey = "timeout_sec";
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::InstanceConfig::InstanceConfig()
{
  maxFaceAgeToLook_ms = 0;
  stoppingCondition = StoppingCondition::Invalid;
  timeout_sec = 0.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::DynamicVariables::DynamicVariables()
{
  searchingForFaces = false;
  imageTimestampWhenActivated = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::BehaviorFindFaces(const Json::Value& config)
  : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, kMaxFaceAgeToLookKey, _iConfig.maxFaceAgeToLook_ms);
  
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";

  const auto& conditionStr = JsonTools::ParseString(config, kStoppingConditionKey, debugName);
  _iConfig.stoppingCondition = StoppingConditionFromString(conditionStr);
  
  _iConfig.searchBehaviorStr = JsonTools::ParseString(config, kSearchBehaviorKey, debugName);
  
  JsonTools::GetValueOptional(config, kTimeoutKey, _iConfig.timeout_sec);
  
  // Make sure a nonzero timeout was provided if the stopping
  // condition is "timeout"
  if (_iConfig.stoppingCondition == StoppingCondition::Timeout) {
    DEV_ASSERT(Util::IsFltGTZero(_iConfig.timeout_sec), "BehaviorFindFaces.InvalidTimeout");
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kMaxFaceAgeToLookKey,
    kStoppingConditionKey,
    kSearchBehaviorKey,
    kTimeoutKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaces::WantsToBeActivatedBehavior() const
{
  // we can always search for faces
  return true;
}


void BehaviorFindFaces::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.searchBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.searchBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.searchBehaviorStr));
  DEV_ASSERT(_iConfig.searchBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullSearchBehavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::OnBehaviorActivated()
{
  DEV_ASSERT(_iConfig.stoppingCondition != StoppingCondition::Invalid,
             "BehaviorFindFaces.OnBehaviorActivated.InvalidStoppingCondition");
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  // Get known faces
  const TimeStamp_t latestImageTimeStamp = robotInfo.GetLastImageTimeStamp();
  _dVars.imageTimestampWhenActivated = latestImageTimeStamp;
  
  _dVars.startingFaces = GetBEI().GetFaceWorld().GetFaceIDs(latestImageTimeStamp > _iConfig.maxFaceAgeToLook_ms ?
                                                            latestImageTimeStamp - _iConfig.maxFaceAgeToLook_ms :
                                                            0);
  const bool hasRecentFaces = !_dVars.startingFaces.empty();
  if ((_iConfig.stoppingCondition == StoppingCondition::AnyFace) &&
      hasRecentFaces){
    return;
  }
  
  // Delegate to the search behavior
  if (_iConfig.searchBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_iConfig.searchBehavior.get());
    _dVars.searchingForFaces = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::OnBehaviorDeactivated()
{
  _dVars.startingFaces.clear();
  _dVars.imageTimestampWhenActivated = 0;
  _dVars.searchingForFaces = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::BehaviorUpdate()
{
  if (!IsActivated() || !IsControlDelegated()) {
    return;
  }
  
  if (_dVars.searchingForFaces) {
    // Check stopping conditions
    bool shouldStop = false;
    if (_iConfig.stoppingCondition == StoppingCondition::AnyFace) {
      shouldStop = GetBEI().GetFaceWorld().HasAnyFaces(_dVars.imageTimestampWhenActivated);
    } else if (_iConfig.stoppingCondition == StoppingCondition::NewFace) {
      const auto& newFaces = GetBEI().GetFaceWorld().GetFaceIDs(_dVars.imageTimestampWhenActivated);
      // Check if newFaces is a subset of startingFaces.
      // If not, then a new face ID must have been added at some point.
      const bool newFaceAdded = !std::includes(_dVars.startingFaces.begin(), _dVars.startingFaces.end(),
                                               newFaces.begin(), newFaces.end());
      shouldStop = newFaceAdded;
    } else if (_iConfig.stoppingCondition == StoppingCondition::Timeout) {
      shouldStop = (GetActivatedDuration() > _iConfig.timeout_sec);
    }
    
    if (shouldStop) {
      PRINT_CH_INFO("Behaviors",
                    "FindFaces.CancellingSearch",
                    "Cancelling face search due to stopping condition %s",
                    StoppingConditionToString(_iConfig.stoppingCondition));
      CancelDelegates();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::StoppingCondition BehaviorFindFaces::StoppingConditionFromString(const std::string& str) const
{
  if (str == "AnyFace")      { return StoppingCondition::AnyFace; }
  else if (str == "NewFace") { return StoppingCondition::NewFace; }
  else if (str == "Timeout") { return StoppingCondition::Timeout; }
  else if (str == "None")    { return StoppingCondition::None; }
  else {
    DEV_ASSERT_MSG(false,
                   "BehaviorFindFaces.StoppingConditionFromString.InvalidString",
                   "String %s is not a valid StoppingCondition",
                   str.c_str());
    return StoppingCondition::Invalid;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* BehaviorFindFaces::StoppingConditionToString(BehaviorFindFaces::StoppingCondition condition) const
{
  switch(condition) {
    case StoppingCondition::Invalid: return "Invalid";
    case StoppingCondition::None:    return "None";
    case StoppingCondition::AnyFace: return "AnyFace";
    case StoppingCondition::NewFace: return "NewFace";
    case StoppingCondition::Timeout: return "Timeout";
  }
}
  
} // namespace Cozmo
} // namespace Anki
