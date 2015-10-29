/**
 * File: selectionBehaviorChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/selectionBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"


namespace Anki {
namespace Cozmo {
  
SelectionBehaviorChooser::SelectionBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser()
  , _robot(robot)
  , _config(config)
{
  if (_robot.HasExternalInterface())
  {
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::ExecuteBehavior,
                                                                      std::bind(&SelectionBehaviorChooser::HandleExecuteBehavior,
                                                                                this, std::placeholders::_1)));
  }
  
  // Setup None Behavior now since it's always the fallback
  _behaviorNone = new BehaviorNone(robot, config);
  Result addResult = AddBehavior(_behaviorNone);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorNone was not created properly.");
    return;
  }
}
  
IBehavior* SelectionBehaviorChooser::ChooseNextBehavior(const Robot& robot, const MoodManager& moodManager, double currentTime_sec) const
{
  auto runnable = [&robot,currentTime_sec](const IBehavior* behavior)
  {
    return (nullptr != behavior && behavior->IsRunnable(robot,currentTime_sec));
  };
  
  if (runnable(_selectedBehavior))
  {
    return _selectedBehavior;
  }
  else if (runnable(_behaviorNone))
  {
    return _behaviorNone;
  }
  
  return nullptr;
}
  
void SelectionBehaviorChooser::HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::ExecuteBehavior& msg = event.GetData().Get_ExecuteBehavior();
  BehaviorType type = msg.behaviorType;
  IBehavior* selectedBehavior = GetBehaviorByName(EnumToString(type));
  if (nullptr == selectedBehavior)
  {
    selectedBehavior = AddNewBehavior(type);
    if (nullptr == selectedBehavior)
    {
      PRINT_NAMED_ERROR("SelectionBehaviorChooser.HandleExecuteBehavior",
                        "SelectionBehaviorChooser could not select the behavior %s.", EnumToString(type));
      return;
    }
  }
  
  _selectedBehavior = selectedBehavior;
}
  
IBehavior* SelectionBehaviorChooser::AddNewBehavior(BehaviorType newType)
{
  IBehavior* newBehavior = nullptr;
  
  switch (newType)
  {
    case BehaviorType::LookAround:
    {
      newBehavior = new BehaviorLookAround(_robot, _config);
      break;
    }
    case BehaviorType::OCD:
    {
      newBehavior = new BehaviorOCD(_robot, _config);
      break;
    }
    case BehaviorType::InteractWithFaces:
    {
      newBehavior = new BehaviorInteractWithFaces(_robot, _config);
      break;
    }
    default:
      // Do nothing
      break;
  }
  
  Result addResult = AddBehavior(newBehavior);
  if (Result::RESULT_OK != addResult)
  {
    Util::SafeDelete(newBehavior);
  }
  
  if (nullptr == newBehavior)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.AddNewBehavior", "Behavior %s was not created properly.", EnumToString(newType));
  }
  
  return newBehavior;
}

} // namespace Cozmo
} // namespace Anki