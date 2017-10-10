/**
* File: ScoringBehaviorChooser.cpp
*
* Author: Lee
* Created: 08/20/15
*
* Description: Class for handling picking of behaviors strictly based on their
* current score
*
* Copyright: Anki, Inc. 2015
*
**/
#include "engine/aiComponent/behaviorComponent/behaviorChoosers/scoringBehaviorChooser.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/messageHelpers.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#if ANKI_DEV_CHEATS
  #define VIZ_BEHAVIOR_SELECTION  1
#else
  #define VIZ_BEHAVIOR_SELECTION  0
#endif // ANKI_DEV_CHEATS

#if VIZ_BEHAVIOR_SELECTION
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)  exp
#else
  #define VIZ_BEHAVIOR_SELECTION_ONLY(exp)
#endif // VIZ_BEHAVIOR_SELECTION

#define DEBUG_SHOW_ALL_SCORES 0

namespace Anki {
namespace Cozmo {

  
namespace{
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScoringBehaviorChooser::ScoringBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
:IBehaviorChooser(behaviorExternalInterface, config)
{
  ReloadFromConfig(behaviorExternalInterface, config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ScoringBehaviorChooser::~ScoringBehaviorChooser()
{
  ClearBehaviors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ScoringBehaviorChooser::ReloadFromConfig(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
{
  const char* kScoreBonusForCurrentBehaviorKey = "scoreBonusForCurrentBehavior";
  const char* kBehaviorsInChooserConfigKey     = "behaviors";
  const char* kScoringConfigKey                = "scoring";
  
  // clear previous
  ClearBehaviors();

  // add behaviors to this chooser
  const Json::Value& behaviorsConfig = config[kBehaviorsInChooserConfigKey];
  DEV_ASSERT_MSG(!behaviorsConfig.isNull(),
                 "ScoringBehaviorChooser.ReloadFromConfig.BehaviorsNotSpecified",
                 "No Behaviors key found");
  if(!behaviorsConfig.isNull()){
    for(const auto& behavior: behaviorsConfig)
    {
      BehaviorID behaviorID = ICozmoBehavior::ExtractBehaviorIDFromConfig(behavior);

      const Json::Value& scoringConfig = (behavior)[kScoringConfigKey];
      DEV_ASSERT_MSG(!scoringConfig.isNull(),
                     "ScoringBehaviorChooser.ReloadFromConfig.ScoringNotSpecified",
                     "Scoring Information Not Provided For %s",
                     BehaviorIDToString(behaviorID));
      // Find the behavior name in the factory
      ICozmoBehaviorPtr scoredBehavior =  behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(behaviorID);
      DEV_ASSERT_MSG(scoredBehavior != nullptr,
                     "ScoringBehaviorChooser.ReloadFromConfig.FailedToFindBehavior",
                     "Behavior not found: %s",
                     BehaviorIDToString(behaviorID));
      if(scoredBehavior != nullptr){
        // Load the scoring information into the behavior
        // note that if there are multiple choosers using the same behaviorName
        // their scores must match
        scoredBehavior->ReadFromScoredJson(scoringConfig);
        TryAddBehavior( scoredBehavior );
        }
    }
  }

  // - score bonus for current behavior [optional]
  _scoreBonusForCurrentBehavior.Clear();
  const Json::Value& scoreBonusJson = config[kScoreBonusForCurrentBehaviorKey];
  if (!scoreBonusJson.isNull())
  {
    const bool scoreBonusLoadedOk = _scoreBonusForCurrentBehavior.ReadFromJson(scoreBonusJson);
    if ( !scoreBonusLoadedOk )
    {
      PRINT_NAMED_WARNING("ScoringBehaviorChooser.ReadFromJson.BadScoreBonus",
        "'%s' failed to read (%s)", kScoreBonusForCurrentBehaviorKey, scoreBonusJson.isNull() ? "Missing" : "Bad");
    }
  
    if (_scoreBonusForCurrentBehavior.GetNumNodes() == 0)
    {
      PRINT_NAMED_WARNING("ScoringBehaviorChooser.ReadFromJson.EmptyScoreBonus", "Forcing to default (no bonuses)");
      _scoreBonusForCurrentBehavior.AddNode(0.0f, 0.0f); // no bonus for any X
    }
  }
  else {
    _scoreBonusForCurrentBehavior.AddNode(0.0f, 0.0f); // no bonus for any X
  }

  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float ScoringBehaviorChooser::ScoreBonusForCurrentBehavior(float runningDuration) const
{
  const float minMargin = _scoreBonusForCurrentBehavior.EvaluateY(runningDuration);
  return minMargin;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ScoringBehaviorChooser::GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior)
{
  static const float kRandomFactor = 0.1f;
  
  VIZ_BEHAVIOR_SELECTION_ONLY( VizInterface::RobotBehaviorSelectData robotBehaviorSelectData );

  ICozmoBehaviorPtr runningBehavior;
  float runningBehaviorScore = 0.0f;
  
  ICozmoBehaviorPtr bestBehavior;
  float bestScore = 0.0f;
  for (const auto& kv : _idToScoredBehaviorMap)
  {
    ICozmoBehaviorPtr behavior = kv.second;
    
    VizInterface::BehaviorScoreData scoreData;

    scoreData.behaviorScore = behavior->EvaluateScore(behaviorExternalInterface);
    scoreData.totalScore    = scoreData.behaviorScore;
    VIZ_BEHAVIOR_SELECTION_ONLY( scoreData.behaviorID = behavior->GetID() );
    
    if (scoreData.totalScore > 0.0f)
    {
      if (behavior->IsRunning())
      {
        const float runningDuration = behavior->GetRunningDuration();
        const float runningBonus = ScoreBonusForCurrentBehavior(runningDuration);
        
        scoreData.totalScore += runningBonus;

        // running behavior gets max possible random score
        scoreData.totalScore += kRandomFactor;

        // don't allow margin and rand to push score out of >0 range
        scoreData.totalScore = Util::Max(scoreData.totalScore, 0.01f);

        if( DEBUG_SHOW_ALL_SCORES ) {
          PRINT_NAMED_DEBUG("BehaviorChooser.Score.Running",
                            "behavior '%s' total=%f (raw=%f + running=%f + random=%f)",
                            behavior->GetIDStr().c_str(),
                            scoreData.totalScore,
                            scoreData.behaviorScore,
                            runningBonus,
                            kRandomFactor);
        }        
      }
      else
      {
        // randomization only for non-running behaviors
        scoreData.totalScore += behaviorExternalInterface.GetRNG().RandDbl(kRandomFactor);

        if( DEBUG_SHOW_ALL_SCORES ) {
          PRINT_NAMED_DEBUG("BehaviorChooser.Score.NotRunning",
                            "behavior '%s' total=%f (raw=%f + random)",
                            behavior->GetIDStr().c_str(),
                            scoreData.totalScore,
                            scoreData.behaviorScore);
        }
      }

      // allow subclasses to modify this score
      ModifyScore(behavior, scoreData.totalScore);
      
      if (scoreData.totalScore > bestScore)
      {
        bestBehavior = behavior;
        bestScore    = scoreData.totalScore;
      }

      if( behavior->IsRunning() ) {
        if( nullptr != runningBehavior ) {
          PRINT_NAMED_WARNING("BehaviorChooser.MultipleRunningBehaviors",
                              "Looks like more than one behavior returned IsRunning(). One of them is '%s'",
                              behavior->GetIDStr().c_str());
        }
        runningBehavior = behavior;
        runningBehaviorScore = scoreData.totalScore;
      }
        
    }
    else if( DEBUG_SHOW_ALL_SCORES ) {
      PRINT_NAMED_DEBUG("BehaviorChooser.Score.Zero",
                        "behavior '%s' choosable but has 0 score",
                        behavior->GetIDStr().c_str());
    }
    
    VIZ_BEHAVIOR_SELECTION_ONLY( robotBehaviorSelectData.scoreData.push_back(scoreData) );
  }
  
  #if ANKI_DEV_CHEATS
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should
    // be removed
    const Robot& robot = behaviorExternalInterface.GetRobot();
    VIZ_BEHAVIOR_SELECTION_ONLY( robot.GetContext()->GetVizManager()->SendRobotBehaviorSelectData(std::move(robotBehaviorSelectData)) );
  #endif
  
  if( runningBehavior != nullptr && bestBehavior != runningBehavior ) {
    PRINT_NAMED_INFO("BehaviorChooser.SwitchBehaviors",
                      "behavior '%s' has score of %f, so is interrupting running behavior '%s' which scored %f",
                      bestBehavior != nullptr ? bestBehavior->GetIDStr().c_str() : "null",
                      bestScore,
                      runningBehavior->GetIDStr().c_str(),
                      runningBehaviorScore);
  }
  
  return bestBehavior;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScoringBehaviorChooser::ClearBehaviors()
{
  _idToScoredBehaviorMap.clear();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ScoringBehaviorChooser::TryAddBehavior(ICozmoBehaviorPtr behavior)
{
  // try to add by behavior name
  BehaviorID behaviorID = behavior->GetID();
  
  const auto insertResult = _idToScoredBehaviorMap.insert(std::make_pair(behaviorID, behavior));
  const bool addedNewEntry = insertResult.second;
  if (!addedNewEntry)
  {
    // if we have an entry in our map under this name, it has to match the pointer in the factory, otherwise
    // who the hell are we pointing to?
    DEV_ASSERT(insertResult.first->second == behavior,
      "ScoringBehaviorChooser.TryAddBehavior.DuplicateNameDifferentPointer" );
  }
  else
  {
    // added to the map as expected
    PRINT_NAMED_DEBUG("ScoringBehaviorChooser.TryAddBehavior.Addition",
      "Added behavior '%s' from factory", BehaviorIDToString(behaviorID));
  }
  
  // return code
  const Result ret = addedNewEntry ? RESULT_OK : RESULT_FAIL;
  return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr ScoringBehaviorChooser::FindBehaviorInTableByID(BehaviorID behaviorID)
{
  ICozmoBehaviorPtr ret = nullptr;
  const auto& matchIt = _idToScoredBehaviorMap.find(behaviorID);
  if ( matchIt != _idToScoredBehaviorMap.end() ) {
    ret = matchIt->second;
  }
  return ret;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ScoringBehaviorChooser::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  for (const auto& kv : _idToScoredBehaviorMap){
    delegates.insert(kv.second.get());
  }
}


} // namespace Cozmo
} // namespace Anki
