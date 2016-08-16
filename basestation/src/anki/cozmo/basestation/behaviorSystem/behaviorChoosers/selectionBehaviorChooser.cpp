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

#include "selectionBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"


namespace Anki {
namespace Cozmo {
  
SelectionBehaviorChooser::SelectionBehaviorChooser(Robot& robot, const Json::Value& config)
  : SimpleBehaviorChooser(robot, config)
  , _robot(robot)  
{
  if (_robot.HasExternalInterface())
  {
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByName,
                               std::bind(&SelectionBehaviorChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
}
  
  // Setup None Behavior now since it's always the fallback
  _behaviorNone = robot.GetBehaviorFactory().CreateBehavior(BehaviorType::NoneBehavior, robot, config);
  Result addResult = TryAddBehavior(_behaviorNone);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("SelectionBehaviorChooser.SelectionBehaviorChooser", "BehaviorNone was not created properly.");
  }
}
  
IBehavior* SelectionBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  auto runnable = [&robot](const IBehavior* behavior)
  {
    return (nullptr != behavior && (behavior->IsRunning() || behavior->IsRunnable(robot)));
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

  IBehavior* selectedBehavior = nullptr;

  switch( event.GetData().GetTag() ) {
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByName: {
      const ExternalInterface::ExecuteBehaviorByName& msg = event.GetData().Get_ExecuteBehaviorByName();
      selectedBehavior = _robot.GetBehaviorFactory().FindBehaviorByName( msg.behaviorName );

      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBehaviorChooser.HandleExecuteBehavior.SelectBehavior",
                         "selecting behavior name '%s'", msg.behaviorName.c_str() );
      } else {
        PRINT_NAMED_WARNING("SelectionBehaviorChooser.HandleExecuteBehavior.UnknownBehavior",
                            "Unknown behavior %s",
                            msg.behaviorName.c_str());
      }

      break;
    }

    default:
      PRINT_NAMED_ERROR("SelectionBehaviorChooser.HandleMessage.UnknownTag",
                        "got a tag we didn't subscribe to");
      break;
  }
    
  _selectedBehavior = selectedBehavior;
}
  
IBehavior* SelectionBehaviorChooser::AddNewBehavior(BehaviorType newType)
{
  Json::Value nullConfig;
  BehaviorFactory& behaviorFactory = _robot.GetBehaviorFactory();
  IBehavior* newBehavior = behaviorFactory.CreateBehavior(newType, _robot, nullConfig);
  
  if (newBehavior)
  {
    TryAddBehavior(newBehavior);
  }
  else
  {
    PRINT_NAMED_ERROR("SelectionBehaviorChooser.AddNewBehavior", "Behavior %s was not created properly.", EnumToString(newType));
  }
  
  return newBehavior;
}

} // namespace Cozmo
} // namespace Anki
