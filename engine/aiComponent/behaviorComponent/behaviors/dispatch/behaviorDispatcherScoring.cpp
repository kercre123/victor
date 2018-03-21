/**
 * File: behaviorDispatcherScoring.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2017-11-2
 *
 * Description: Simple behavior which takes a json-defined list of other behaviors
 * and dispatches to the behavior in the list that has the highest evaluated score
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherScoring.h"

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/helpers/behaviorScoringWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kBehaviorsInChooserConfigKey     = "behaviors";  
const char* kScoringConfigKey                = "scoring";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherScoring::InstanceConfig::InstanceConfig()
{

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherScoring::DynamicVariables::DynamicVariables()
{
  currentScoringTracker = nullptr;  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherScoring::BehaviorDispatcherScoring(const Json::Value& config)
: IBehaviorDispatcher(config)
{

  const Json::Value& behaviorsConfig = config[kBehaviorsInChooserConfigKey];
  DEV_ASSERT_MSG(!behaviorsConfig.isNull(),
                 "ScoringBehaviorChooser.ReloadFromConfig.BehaviorsNotSpecified",
                 "No Behaviors key found");

  if(!behaviorsConfig.isNull()){
    for(const auto& behavior: behaviorsConfig)
    {
      BehaviorID behaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behavior);
      // TODO:(bn) support anonymous behaviors here?
      IBehaviorDispatcher::AddPossibleDispatch(BehaviorTypesWrapper::BehaviorIDToString(behaviorID));
      
      const Json::Value& scoringConfig = (behavior)[kScoringConfigKey];
      DEV_ASSERT_MSG(!scoringConfig.isNull(),
                     "ScoringBehaviorChooser.ReloadFromConfig.ScoringNotSpecified",
                     "Scoring Information Not Provided For %s",
                     BehaviorTypesWrapper::BehaviorIDToString(behaviorID));

      _iConfig.scoringTracker.push_back(BehaviorScoringWrapper(scoringConfig));
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorDispatcherScoring::~BehaviorDispatcherScoring()
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherScoring::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  expectedKeys.insert( kBehaviorsInChooserConfigKey );
  IBehaviorDispatcher::GetBehaviorJsonKeys(expectedKeys);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherScoring::InitDispatcher()
{
  for(auto& entry: _iConfig.scoringTracker){
    entry.Init(GetBEI());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorDispatcherScoring::BehaviorDispatcher_OnDeactivated() 
{
  if(_dVars.currentScoringTracker != nullptr){
    _dVars.currentScoringTracker->BehaviorDeactivated();
    _dVars.currentScoringTracker = nullptr;
  }
  _dVars.currentDispatch.reset();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr BehaviorDispatcherScoring::GetDesiredBehavior()
{
  ICozmoBehaviorPtr desiredDispatch;  
  if(ANKI_VERIFY(GetAllPossibleDispatches().size() == _iConfig.scoringTracker.size(),
                 "BehaviorDispatcherScoring.GetDesiredBehavior.InvalidSizes",
                 "there are %zu dispatches, but %zu scoring trackers",
                 GetAllPossibleDispatches().size(),
                 _iConfig.scoringTracker.size())){
    const int invalidIdx = -1;
    int highestScoreIdx = invalidIdx;                  
    float highestScore = 0.f;

    // Iterate through available behaviors, find the highest scored behavior that
    // wants to be activated, and activate it
    auto& allDispatches = GetAllPossibleDispatches();
    for(int i = 0; i < allDispatches.size(); i++) {
      auto& entry = allDispatches[i];
      if(entry->IsActivated() || entry->WantsToBeActivated()) {
        const float newScore = _iConfig.scoringTracker[i].EvaluateScore(GetBEI());
        if(FLT_GT(newScore, highestScore)){
          highestScore = newScore;
          highestScoreIdx = i;
        }
      }
    }

    // Activate/deactivate score tracking if highest score has changed
    if(highestScoreIdx != invalidIdx){
      desiredDispatch = allDispatches[highestScoreIdx];
      if(desiredDispatch != _dVars.currentDispatch){
        if(_dVars.currentScoringTracker != nullptr){
          _dVars.currentScoringTracker->BehaviorDeactivated();
        }
        _dVars.currentScoringTracker = &_iConfig.scoringTracker[highestScoreIdx];
        _dVars.currentScoringTracker->BehaviorWillBeActivated();
        _dVars.currentDispatch = desiredDispatch;
      }
    }

  }

  return desiredDispatch;
}

} // namespace Cozmo
} // namespace Anki
