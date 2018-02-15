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

}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorFindFaces::BehaviorFindFaces(const Json::Value& config)
  : ICozmoBehavior(config)
{
  JsonTools::GetValueOptional(config, "maxFaceAgeToLook_ms", _maxFaceAgeToLook_ms);
  
  const std::string& debugName = "Behavior" + GetDebugLabel() + ".LoadConfig";

  const auto& conditionStr = JsonTools::ParseString(config, "stoppingCondition", debugName);
  _stoppingCondition = StoppingConditionFromString(conditionStr);
  
  _searchBehaviorStr = JsonTools::ParseString(config, "searchBehavior", debugName);
  
  JsonTools::GetValueOptional(config, "timeout_sec", _timeout_sec);
  
  // Make sure a nonzero timeout was provided if the stopping
  // condition is "timeout"
  if (_stoppingCondition == StoppingCondition::Timeout) {
    DEV_ASSERT(Util::IsFltGTZero(_timeout_sec), "BehaviorFindFaces.InvalidTimeout");
  }
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorFindFaces::WantsToBeActivatedBehavior() const
{
  // we can always search for faces
  return true;
}


void BehaviorFindFaces::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_searchBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  _searchBehavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(_searchBehaviorStr));
  DEV_ASSERT(_searchBehavior != nullptr,
             "BehaviorFindFaces.InitBehavior.NullSearchBehavior");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::OnBehaviorActivated()
{
  DEV_ASSERT(_stoppingCondition != StoppingCondition::Invalid,
             "BehaviorFindFaces.OnBehaviorActivated.InvalidStoppingCondition");
  
  const auto& robotInfo = GetBEI().GetRobotInfo();
  
  // Get known faces
  const TimeStamp_t latestImageTimeStamp = robotInfo.GetLastImageTimeStamp();
  _imageTimestampWhenActivated = latestImageTimeStamp;
  
  _startingFaces = GetBEI().GetFaceWorld().GetFaceIDsObservedSince(latestImageTimeStamp > _maxFaceAgeToLook_ms ?
                                                                   latestImageTimeStamp - _maxFaceAgeToLook_ms :
                                                                   0);
  const bool hasRecentFaces = !_startingFaces.empty();
  if ((_stoppingCondition == StoppingCondition::AnyFace) &&
      hasRecentFaces){
    return;
  }
  
  // Delegate to the search behavior
  if (_searchBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_searchBehavior.get());
    _searchingForFaces = true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::OnBehaviorDeactivated()
{
  _startingFaces.clear();
  _imageTimestampWhenActivated = 0;
  _searchingForFaces = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFindFaces::BehaviorUpdate()
{
  if (!IsActivated() || !IsControlDelegated()) {
    return;
  }
  
  if (_searchingForFaces) {
    // Check stopping conditions
    bool shouldStop = false;
    if (_stoppingCondition == StoppingCondition::AnyFace) {
      shouldStop = GetBEI().GetFaceWorld().HasAnyFaces(_imageTimestampWhenActivated);
    } else if (_stoppingCondition == StoppingCondition::NewFace) {
      const auto& newFaces = GetBEI().GetFaceWorld().GetFaceIDsObservedSince(_imageTimestampWhenActivated);
      // Check if newFaces is a subset of startingFaces.
      // If not, then a new face ID must have been added at some point.
      const bool newFaceAdded = !std::includes(_startingFaces.begin(), _startingFaces.end(),
                                               newFaces.begin(), newFaces.end());
      shouldStop = newFaceAdded;
    } else if (_stoppingCondition == StoppingCondition::Timeout) {
      shouldStop = (GetActivatedDuration() > _timeout_sec);
    }
    
    if (shouldStop) {
      PRINT_CH_INFO("Behaviors",
                    "FindFaces.CancellingSearch",
                    "Cancelling face search due to stopping condition %s",
                    StoppingConditionToString(_stoppingCondition));
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
