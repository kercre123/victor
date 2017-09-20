/**
 * File: SelectionBSRunnableChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/aiComponent/behaviorSystem/bsRunnableChoosers/selectionBSRunnableChooser.h"

#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorSystem/behaviorContainer.h"
#include "engine/aiComponent/behaviorSystem/behaviorManager.h"
#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
namespace{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SelectionBSRunnableChooser::SelectionBSRunnableChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IBSRunnableChooser(behaviorExternalInterface, config)
, _behaviorExternalInterface(behaviorExternalInterface)
{
  auto robotExternalInterface = behaviorExternalInterface.GetRobotExternalInterface().lock();
  if (robotExternalInterface != nullptr)
  {
    _eventHandlers.push_back(robotExternalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID,
                               std::bind(&SelectionBSRunnableChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
    
    _eventHandlers.push_back(robotExternalInterface->Subscribe(
                               ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType,
                               std::bind(&SelectionBSRunnableChooser::HandleExecuteBehavior,
                                         this, std::placeholders::_1)));
  }
  
  // Setup Behavior wait now since it's always the fallback
  _behaviorWait = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(BehaviorID::Wait);
  DEV_ASSERT(_behaviorWait != nullptr &&
             _behaviorWait->GetClass() == BehaviorClass::Wait,
             "SelectionBSRunnableChooser.BehaviorWaitNotLoaded");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBehaviorPtr SelectionBSRunnableChooser::GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const IBehaviorPtr currentRunningBehavior)
{
  auto runnable = [this, &behaviorExternalInterface](const IBehaviorPtr behavior)
  {
    const bool behaviorIsRunning = nullptr != behavior && behavior->IsRunning();
    bool ret = (nullptr != behavior && (behaviorIsRunning || behavior->IsRunnable(behaviorExternalInterface)));
    
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
  else if (runnable(_behaviorWait))
  {
    return _behaviorWait;
  }
  
  return nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBSRunnableChooser::HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{

  IBehaviorPtr selectedBehavior;

  switch( event.GetData().GetTag() ) {
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID: {
      const ExternalInterface::ExecuteBehaviorByID& msg = event.GetData().Get_ExecuteBehaviorByID();
      selectedBehavior = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID( msg.behaviorID );
      _numRuns = msg.numRuns;

      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBSRunnableChooser.HandleExecuteBehaviorByName.SelectBehavior",
                         "selecting behavior name '%s'", BehaviorIDToString(msg.behaviorID));
      } else {
        PRINT_NAMED_WARNING("SelectionBSRunnableChooser.HandleExecuteBehaviorByName.UnknownBehavior",
                            "Unknown behavior %s",
                            BehaviorIDToString(msg.behaviorID));
      }

      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType:
    {
      const ExternalInterface::ExecuteBehaviorByExecutableType& msg = event.GetData().Get_ExecuteBehaviorByExecutableType();
      selectedBehavior = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByExecutableType( msg.behaviorType );
      _numRuns = msg.numRuns;
      
      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBSRunnableChooser.ExecuteBehaviorByExecutableType.SelectBehavior",
                         "selecting behavior '%s' exec type '%s'", selectedBehavior->GetIDStr().c_str(), EnumToString(msg.behaviorType) );
      } else {
        PRINT_NAMED_WARNING("SelectionBSRunnableChooser.ExecuteBehaviorByExecutableType.NoBehavior",
                            "No behavior for exec type %s",
                            EnumToString(msg.behaviorType));
      }
      
      break;
    }

    default:
      PRINT_NAMED_ERROR("SelectionBSRunnableChooser.HandleMessage.UnknownTag",
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
void SelectionBSRunnableChooser::OnSelected()
{
  // enable process for selected behavior
  SetProcessEnabled(_selectedBehavior, true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBSRunnableChooser::OnDeselected()
{
  // disable process for selected behavior
  SetProcessEnabled(_selectedBehavior, false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBSRunnableChooser::SetProcessEnabled(const IBehaviorPtr behavior, bool newValue)
{
  const char* const kAIInformationAnalyzerLock = "SelectionBSRunnableChooser";

  if ( nullptr != behavior )
  {
    // notify the information analyzer when our behavior has a required process
    AIInformationAnalysis::EProcess process = behavior->GetRequiredProcess();
    if ( process != AIInformationAnalysis::EProcess::Invalid )
    {
      auto& infoAnalyzer = _behaviorExternalInterface.GetAIComponent().GetAIInformationAnalyzer();
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
