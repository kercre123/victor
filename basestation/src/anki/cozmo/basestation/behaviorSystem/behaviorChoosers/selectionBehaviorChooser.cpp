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

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/selectionBehaviorChooser.h"

#include "anki/cozmo/basestation/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "anki/cozmo/basestation/behaviorSystem/aiComponent.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
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
    
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType,
                               std::bind(&SelectionBehaviorChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
  }
  
  // Setup None Behavior now since it's always the fallback
  _behaviorNone = robot.GetBehaviorFactory().CreateBehavior(BehaviorClass::NoneBehavior, robot, config);
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
    BehaviorPreReqRobot preReqData(robot);
    return (nullptr != behavior && (behavior->IsRunning() || behavior->IsRunnable(preReqData)));
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
        PRINT_NAMED_INFO("SelectionBehaviorChooser.HandleExecuteBehaviorByName.SelectBehavior",
                         "selecting behavior name '%s'", msg.behaviorName.c_str() );
      } else {
        PRINT_NAMED_WARNING("SelectionBehaviorChooser.HandleExecuteBehaviorByName.UnknownBehavior",
                            "Unknown behavior %s",
                            msg.behaviorName.c_str());
      }

      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType:
    {
      const ExternalInterface::ExecuteBehaviorByExecutableType& msg = event.GetData().Get_ExecuteBehaviorByExecutableType();
      selectedBehavior = _robot.GetBehaviorFactory().FindBehaviorByExecutableType( msg.behaviorType );
      
      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBehaviorChooser.ExecuteBehaviorByExecutableType.SelectBehavior",
                         "selecting behavior '%s' exec type '%s'", selectedBehavior->GetName().c_str(), EnumToString(msg.behaviorType) );
      } else {
        PRINT_NAMED_WARNING("SelectionBehaviorChooser.ExecuteBehaviorByExecutableType.NoBehavior",
                            "No behavior for exec type %s",
                            EnumToString(msg.behaviorType));
      }
      
      break;
    }

    default:
      PRINT_NAMED_ERROR("SelectionBehaviorChooser.HandleMessage.UnknownTag",
                        "got a tag we didn't subscribe to");
      break;
  }
  
  // if the behavior changes, check processes
  if ( _selectedBehavior != selectedBehavior ) {
    SetProcessEnabled(_selectedBehavior, false);  // disable previous process
    SetProcessEnabled(selectedBehavior, true);    // enable new process
  }
  
    
  _selectedBehavior = selectedBehavior;
}
  
IBehavior* SelectionBehaviorChooser::AddNewBehavior(BehaviorClass newType)
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::OnSelected()
{
  // enable process for selected behavior
  SetProcessEnabled(_selectedBehavior, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::OnDeselected()
{
  // disable process for selected behavior
  SetProcessEnabled(_selectedBehavior, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::SetProcessEnabled(const IBehavior* behavior, bool newValue)
{
  if ( nullptr != behavior )
  {
    // notify the information analyzer when our behavior has a required process
    AIInformationAnalysis::EProcess process = behavior->GetRequiredProcess();
    if ( process != AIInformationAnalysis::EProcess::Invalid )
    {
      if( newValue ) {
        _robot.GetAIComponent().GetAIInformationAnalyzer().AddEnableRequest(process, GetName());
      } else {
        _robot.GetAIComponent().GetAIInformationAnalyzer().RemoveEnableRequest(process, GetName());
      }
    }
  }
}

} // namespace Cozmo
} // namespace Anki
