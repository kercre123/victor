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
  
IBehavior* SimpleBehaviorChooser::ChooseNextBehavior(float currentTime_sec) const
{
  for (auto behavior : _behaviorList)
  {
    if (behavior->IsRunnable(currentTime_sec))
    {
      return behavior;
    }
  }
  return nullptr;
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

  
} // namespace Cozmo
} // namespace Anki