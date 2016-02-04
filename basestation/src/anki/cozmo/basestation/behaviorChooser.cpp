/**
 * File: behaviorChooser.cpp
 *
 * Author: Lee
 * Created: 08/20/15
 *
 * Description: Class for handling picking of behaviors.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/viz/vizManager.h"
#include "util/global/globalDefinitions.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"


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


namespace Anki {
namespace Cozmo {

#pragma mark --- SimpleBehaviorChooser IBehaviorChooser Members ---
Result SimpleBehaviorChooser::AddBehavior(IBehavior* newBehavior)
{
  if (nullptr == newBehavior)
  {
    PRINT_NAMED_ERROR("SimpleBehaviorChooser.AddBehavior", "Refusing to add a null behavior.");
    return Result::RESULT_FAIL;
  }
  
  assert(newBehavior->IsOwnedByFactory()); // we assume all behaviors are created and owned by factory now
  
  auto newEntry = _nameToBehaviorMap.insert( NameToBehaviorMap::value_type(newBehavior->GetName(), newBehavior) );
  const bool addedNewEntry = newEntry.second;
  if (!addedNewEntry)
  {
    // a behavior already exists in the list with this name, replace it
    
    IBehavior* oldBehavior = newEntry.first->second;
    
    PRINT_NAMED_WARNING("SimpleBehaviorChooser.AddBehavior.ReplaceExisting",
                             "Replacing existing '%s' behavior.", oldBehavior->GetName().c_str());

    assert(oldBehavior->IsOwnedByFactory()); // otherwise we'd be leaking the old behavior
    newEntry.first->second = newBehavior;

    return Result::RESULT_OK;
  }
  
  return Result::RESULT_OK;
}

  
void SimpleBehaviorChooser::EnableAllBehaviors(bool newVal)
{
  for (const auto& kv : _nameToBehaviorMap)
  {
    IBehavior* behavior = kv.second;
    behavior->SetIsChoosable(newVal);
  }
}
  
  
void SimpleBehaviorChooser::EnableBehaviorGroup(BehaviorGroup behaviorGroup, bool newVal)
{
  for (const auto& kv : _nameToBehaviorMap)
  {
    IBehavior* behavior = kv.second;
    if (behavior->IsBehaviorGroup(behaviorGroup))
    {
      behavior->SetIsChoosable(newVal);
    }
  }
}
  
  
void SimpleBehaviorChooser::EnableBehavior(const std::string& behaviorName, bool newVal)
{
  const auto& it = _nameToBehaviorMap.find(behaviorName);
  if (it != _nameToBehaviorMap.end())
  {
    IBehavior* behavior = it->second;
    behavior->SetIsChoosable(newVal);
  }
  else
  {
    PRINT_NAMED_WARNING("EnableBehavior.NotFound", "No Behavior named '%s' (newVal = %d)", behaviorName.c_str(), (int)newVal);
  }
}
  

IBehavior* SimpleBehaviorChooser::ChooseNextBehavior(const Robot& robot, double currentTime_sec) const
{
  const float kRandomFactor = 0.1f;
  
  Util::RandomGenerator rng; // [MarkW:TODO] We should share these (1 per robot or subsystem maybe?) for replay determinism
  
  VIZ_BEHAVIOR_SELECTION_ONLY( VizInterface::RobotBehaviorSelectData robotBehaviorSelectData );
  
  IBehavior* bestBehavior = nullptr;
  float bestScore = 0.0f;
  for (const auto& kv : _nameToBehaviorMap)
  {
    IBehavior* behavior = kv.second;
    
    if (!behavior->IsChoosable())
    {
      continue;
    }
        
    VizInterface::BehaviorScoreData scoreData;
    
    scoreData.behaviorScore = behavior->EvaluateScore(robot, currentTime_sec);
    scoreData.totalScore    = scoreData.behaviorScore;
    VIZ_BEHAVIOR_SELECTION_ONLY( scoreData.name = behavior->GetName() );
    
    if (scoreData.totalScore > 0.0f)
    {
      scoreData.totalScore += rng.RandDbl(kRandomFactor);
      
      if (scoreData.totalScore > bestScore)
      {
        bestBehavior = behavior;
        bestScore    = scoreData.totalScore;
      }
    }
    
    VIZ_BEHAVIOR_SELECTION_ONLY( robotBehaviorSelectData.scoreData.push_back(scoreData) );
  }
  
  VIZ_BEHAVIOR_SELECTION_ONLY( VizManager::getInstance()->SendRobotBehaviorSelectData(std::move(robotBehaviorSelectData)) );
  
  return bestBehavior;
}
  
IBehavior* SimpleBehaviorChooser::GetBehaviorByName(const std::string& name) const
{
  const auto& it = _nameToBehaviorMap.find(name);
  if (it != _nameToBehaviorMap.end())
  {
    IBehavior* behavior = it->second;
    return behavior;
  }
  
  return nullptr;
}
  
#pragma mark --- SimpleBehaviorChooser Members ---
SimpleBehaviorChooser::~SimpleBehaviorChooser()
{
  #if ANKI_DEVELOPER_CODE
  for (const auto& kv : _nameToBehaviorMap)
  {
    const IBehavior* behavior = kv.second;
    ASSERT_NAMED(behavior->IsOwnedByFactory(), "Behavior not owned by factory - shouldn't be possible!");
  }
  #endif //ANKI_DEVELOPER_CODE
}
  
#pragma mark --- ReactionaryBehaviorChooser Members ---
  
// This helper function implements the logic to use when we want to find the behavior that's going to care about
// a particular input event. It goes through our list of reactionary Behaviors, and if the behavior's set of event
// tags includes the type associated with this event, we return it
template <typename EventType>
IReactionaryBehavior* ReactionaryBehaviorChooser::_GetReactionaryBehavior(
  const Robot& robot,
  const AnkiEvent<EventType>& event,
  std::function<const std::set<typename EventType::Tag>&(const IReactionaryBehavior&)> getTagSet) const
{
  // Note this current implementation simply returns the first behavior in the list that cares about this
  // event, and is runnable. There could be others but simply the first is returned.
  for (auto& behavior : _reactionaryBehaviorList)
  {
    if (0 != getTagSet(*behavior).count(event.GetData().GetTag()))
    {
      if ( behavior->IsRunnable(robot, BaseStationTimer::getInstance()->GetCurrentTimeInSeconds()) ) {
        return behavior;
      }
    }
  }
  return nullptr;
}

IBehavior* ReactionaryBehaviorChooser::GetReactionaryBehavior(
  const Robot& robot,
  const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) const
{
  auto getEngineToGameTags = [](const IReactionaryBehavior& behavior) -> const std::set<ExternalInterface::MessageEngineToGameTag>&
  {
    return behavior.GetEngineToGameTags();
  };
  return _GetReactionaryBehavior(robot, event, getEngineToGameTags);
}

IBehavior* ReactionaryBehaviorChooser::GetReactionaryBehavior(
  const Robot& robot,
  const AnkiEvent<ExternalInterface::MessageGameToEngine>& event) const
{
  auto getGameToEngineTags = [](const IReactionaryBehavior& behavior) -> const std::set<ExternalInterface::MessageGameToEngineTag>&
  {
    return behavior.GetGameToEngineTags();
  };
  return _GetReactionaryBehavior(robot, event, getGameToEngineTags);
}
  
ReactionaryBehaviorChooser::~ReactionaryBehaviorChooser()
{
  #if ANKI_DEVELOPER_CODE
  for (auto& behavior : _reactionaryBehaviorList)
  {
    ASSERT_NAMED(behavior->IsOwnedByFactory(), "Behavior not owned by factory - shouldn't be possible!");
  }
  #endif //ANKI_DEVELOPER_CODE
}

  
} // namespace Cozmo
} // namespace Anki
