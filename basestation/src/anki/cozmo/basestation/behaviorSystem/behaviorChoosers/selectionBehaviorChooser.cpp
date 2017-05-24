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
  
namespace{
static const char* const kAIInformationAnalyzerLock = "SelectionBehaviorChooser";

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SelectionBehaviorChooser::SelectionBehaviorChooser(Robot& robot, const Json::Value& config)
: IBehaviorChooser(robot, config)
, _robot(robot)  
{
  if (_robot.HasExternalInterface())
  {
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID,
                               std::bind(&SelectionBehaviorChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
    
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType,
                               std::bind(&SelectionBehaviorChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
  }
  
  // Setup None Behavior now since it's always the fallback
  Json::Value noneConfig = IBehavior::CreateDefaultBehaviorConfig(BehaviorID::NoneBehavior);
  _behaviorNone = robot.GetBehaviorFactory().CreateBehavior(BehaviorClass::NoneBehavior, robot, noneConfig);
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehavior* SelectionBehaviorChooser::ChooseNextBehavior(Robot& robot, const IBehavior* currentRunningBehavior)
{
  auto runnable = [this, &robot](const IBehavior* behavior)
  {
    BehaviorPreReqRobot preReqData(robot);
    const bool behaviorIsRunning = nullptr != behavior && behavior->IsRunning();
    bool ret = (nullptr != behavior && (behaviorIsRunning || behavior->IsRunnable(preReqData)));
    
    // If this is the selected behavior
    if(behavior == _selectedBehavior)
    {
      // If there are no more runs to do then the selected behavior is not runnable
      if(_numRuns == 0)
      {
        return false;
      }
      
      // If there are runs left and the selected behavior was running (bool set from last tick)
      // but the behavior is currently not running (it just ended)
      if(_numRuns > 0 && _selectedBehaviorIsRunning && !behaviorIsRunning)
      {
        // Decrememnt numRuns since the behavior just finished running
        if(--_numRuns == 0)
        {
          ret = false;
        }
      }
      _selectedBehaviorIsRunning = behaviorIsRunning;
    }
    return ret;
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

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{

  IBehavior* selectedBehavior = nullptr;

  switch( event.GetData().GetTag() ) {
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID: {
      const ExternalInterface::ExecuteBehaviorByID& msg = event.GetData().Get_ExecuteBehaviorByID();
      selectedBehavior = _robot.GetBehaviorFactory().FindBehaviorByID( msg.behaviorID );
      _numRuns = msg.numRuns;

      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBehaviorChooser.HandleExecuteBehaviorByName.SelectBehavior",
                         "selecting behavior name '%s'", BehaviorIDToString(msg.behaviorID));
      } else {
        PRINT_NAMED_WARNING("SelectionBehaviorChooser.HandleExecuteBehaviorByName.UnknownBehavior",
                            "Unknown behavior %s",
                            BehaviorIDToString(msg.behaviorID));
      }

      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType:
    {
      const ExternalInterface::ExecuteBehaviorByExecutableType& msg = event.GetData().Get_ExecuteBehaviorByExecutableType();
      selectedBehavior = _robot.GetBehaviorFactory().FindBehaviorByExecutableType( msg.behaviorType );
      _numRuns = msg.numRuns;
      
      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBehaviorChooser.ExecuteBehaviorByExecutableType.SelectBehavior",
                         "selecting behavior '%s' exec type '%s'", selectedBehavior->GetIDStr().c_str(), EnumToString(msg.behaviorType) );
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
      auto& infoAnalyzer = _robot.GetAIComponent().GetAIInformationAnalyzer();
      if( newValue ) {
        infoAnalyzer.AddEnableRequest(process, kAIInformationAnalyzerLock);
      } else {
        infoAnalyzer.RemoveEnableRequest(process, kAIInformationAnalyzerLock);
      }
    }
  }
}

} // namespace Cozmo
} // namespace Anki
