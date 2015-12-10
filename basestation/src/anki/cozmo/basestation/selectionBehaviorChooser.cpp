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
#include "anki/cozmo/basestation/behaviors/behaviorFollowMotion.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
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
  _behaviorNone = robot.GetBehaviorFactory().CreateBehavior(BehaviorType::NoneBehavior, robot, config);
  Result addResult = AddBehavior(_behaviorNone);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("SelectionBehaviorChooser.SelectionBehaviorChooser", "BehaviorNone was not created properly.");
  }
}
  
IBehavior* SelectionBehaviorChooser::ChooseNextBehavior(const Robot& robot, double currentTime_sec) const
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
  BehaviorFactory& behaviorFactory = _robot.GetBehaviorFactory();
  IBehavior* newBehavior = behaviorFactory.CreateBehavior(newType, _robot, _config);
  
  if (newBehavior)
  {
    const Result addResult = AddBehavior(newBehavior);
    if (Result::RESULT_OK != addResult)
    {
      PRINT_NAMED_ERROR("SelectionBehaviorChooser.AddNewBehavior",
                        "Behavior '%s' of type %s was created but failed to add to chooser!", newBehavior->GetName().c_str(), EnumToString(newType));
      
      newBehavior = nullptr; // behavior will be retained by factory, but chooser cannot use it
    }
  }
  else
  {
    PRINT_NAMED_ERROR("SelectionBehaviorChooser.AddNewBehavior", "Behavior %s was not created properly.", EnumToString(newType));
  }
  
  return newBehavior;
}

} // namespace Cozmo
} // namespace Anki