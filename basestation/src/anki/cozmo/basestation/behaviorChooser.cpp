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

#include "anki/cozmo/basestation/behaviorChooser.h"
#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"

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
  
  // If a behavior already exists in the list with this name, replace it
  for (auto& behavior : _behaviorList)
  {
    if (behavior->GetName() == newBehavior->GetName())
    {
      PRINT_NAMED_WARNING("SimpleBehaviorChooser.AddBehavior.ReplaceExisting",
                               "Replacing existing '%s' behavior.", behavior->GetName().c_str());
      IBehavior* toDelete = behavior;
      behavior = newBehavior;
      Util::SafeDelete(toDelete);
      return Result::RESULT_OK;
    }
  }
  
  // Otherwise just push the new behavior onto the list
  _behaviorList.push_back(newBehavior);
  return Result::RESULT_OK;
}
  
IBehavior* SimpleBehaviorChooser::ChooseNextBehavior(const MoodManager& moodManager, double currentTime_sec) const
{
  const float kRandomFactor = 0.1f;
  
  Util::RandomGenerator rng; // [MarkW:TODO] We should share these (1 per robot or subsystem maybe?) for replay determinism
  
  IBehavior* bestBehavior = nullptr;
  float bestScore = 0.0f;
  for (IBehavior* behavior : _behaviorList)
  {
    float score = behavior->EvaluateScore(moodManager, currentTime_sec);
    if (score > 0.0f)
    {
      score += rng.RandDbl(kRandomFactor);
      
      if (score > bestScore)
      {
        bestBehavior = behavior;
        bestScore    = score;
      }
    }
  }
  return bestBehavior;
}
  
IBehavior* SimpleBehaviorChooser::GetBehaviorByName(const std::string& name) const
{
  for (auto behavior : _behaviorList)
  {
    if (behavior->GetName() == name)
    {
      return behavior;
    }
  }
  return nullptr;
}
  
#pragma mark --- SimpleBehaviorChooser Members ---
SimpleBehaviorChooser::~SimpleBehaviorChooser()
{
  for (auto& behavior : _behaviorList)
  {
    Util::SafeDelete(behavior);
  }
}
  
#pragma mark --- ReactionaryBehaviorChooser Members ---
  
// This helper function implements the logic to use when we want to find the behavior that's going to care about
// a particular input event. It goes through our list of reactionary Behaviors, and if the behavior's set of event
// tags includes the type associated with this event, we return it
template <typename EventType>
IReactionaryBehavior* ReactionaryBehaviorChooser::_GetReactionaryBehavior(const AnkiEvent<EventType>& event,
                                                                          std::function<const std::set<typename EventType::Tag>&(const IReactionaryBehavior&)> getTagSet
                                                                          ) const
{
  // Note this current implementation simply returns the first behavior in the list that cares about this event. There
  // could be others but simply the first is returned.
  for (auto& behavior : _reactionaryBehaviorList)
  {
    if (0 != getTagSet(*behavior).count(event.GetData().GetTag()))
    {
      return behavior;
    }
  }
  return nullptr;
}

IBehavior* ReactionaryBehaviorChooser::GetReactionaryBehavior(const AnkiEvent<ExternalInterface::MessageEngineToGame>& event) const
{
  auto getEngineToGameTags = [](const IReactionaryBehavior& behavior) -> const std::set<ExternalInterface::MessageEngineToGameTag>&
  {
    return behavior.GetEngineToGameTags();
  };
  return _GetReactionaryBehavior(event, getEngineToGameTags);
}

IBehavior* ReactionaryBehaviorChooser::GetReactionaryBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event) const
{
  auto getGameToEngineTags = [](const IReactionaryBehavior& behavior) -> const std::set<ExternalInterface::MessageGameToEngineTag>&
  {
    return behavior.GetGameToEngineTags();
  };
  return _GetReactionaryBehavior(event, getGameToEngineTags);
}
  
ReactionaryBehaviorChooser::~ReactionaryBehaviorChooser()
{
  for (auto& behavior : _reactionaryBehaviorList)
  {
    Util::SafeDelete(behavior);
  }
}

  
} // namespace Cozmo
} // namespace Anki