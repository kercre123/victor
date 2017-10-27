/**
 * File: SelectionBehaviorChooser.h
 *
 * Author: Lee Crippen
 * Created: 10/15/15
 *
 * Description: Class for allowing specific behaviors to be selected.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviorChoosers/selectionBehaviorChooser.h"

#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorManager.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorEventComponent.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/helpers/templateHelpers.h"

namespace Anki {
namespace Cozmo {
  
namespace{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SelectionBehaviorChooser::SelectionBehaviorChooser(BehaviorExternalInterface& behaviorExternalInterface, const Json::Value& config)
: IBehaviorChooser(behaviorExternalInterface, config)
, _behaviorExternalInterface(behaviorExternalInterface)
{
  
  std::set<ExternalInterface::MessageGameToEngineTag> tags;
  tags.insert(ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID);
  tags.insert(ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType);

  behaviorExternalInterface.GetStateChangeComponent().SubscribeToTags(this, std::move(tags));

  
  // Setup Behavior wait now since it's always the fallback
  _behaviorWait = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(BehaviorID::Wait);
  DEV_ASSERT(_behaviorWait != nullptr &&
             _behaviorWait->GetClass() == BehaviorClass::Wait,
             "SelectionBehaviorChooser.BehaviorWaitNotLoaded");
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ICozmoBehaviorPtr SelectionBehaviorChooser::GetDesiredActiveBehavior(BehaviorExternalInterface& behaviorExternalInterface, const ICozmoBehaviorPtr currentRunningBehavior)
{
  auto behavior = [this, &behaviorExternalInterface](const ICozmoBehaviorPtr behavior)
  {
    const bool behaviorIsRunning = nullptr != behavior && behavior->IsActivated();
    bool ret = (nullptr != behavior && (behaviorIsRunning || behavior->WantsToBeActivated(behaviorExternalInterface)));
    
    // If this is the selected behavior
    if(behavior == _selectedBehavior)
    {
      // If there are no more runs to do then the selected behavior is not activatable
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
  
  if (behavior(_selectedBehavior))
  {
    return _selectedBehavior;
  }
  else if (behavior(_behaviorWait))
  {
    return _behaviorWait;
  }
  
  return nullptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::HandleExecuteBehavior(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{

  ICozmoBehaviorPtr selectedBehavior;

  switch( event.GetData().GetTag() ) {
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID: {
      const ExternalInterface::ExecuteBehaviorByID& msg = event.GetData().Get_ExecuteBehaviorByID();
      selectedBehavior = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByID(BehaviorIDFromString(msg.behaviorID) );
      _numRuns = msg.numRuns;

      if( selectedBehavior != nullptr ) {
        PRINT_NAMED_INFO("SelectionBehaviorChooser.HandleExecuteBehaviorByName.SelectBehavior",
                         "selecting behavior name '%s'", msg.behaviorID.c_str());
      } else {
        PRINT_NAMED_WARNING("SelectionBehaviorChooser.HandleExecuteBehaviorByName.UnknownBehavior",
                            "Unknown behavior %s",
                            msg.behaviorID.c_str());
      }

      break;
    }
      
    case ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType:
    {
      const ExternalInterface::ExecuteBehaviorByExecutableType& msg = event.GetData().Get_ExecuteBehaviorByExecutableType();
      selectedBehavior = _behaviorExternalInterface.GetBehaviorContainer().FindBehaviorByExecutableType( msg.behaviorType );
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
void SelectionBehaviorChooser::OnActivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // enable process for selected behavior
  SetProcessEnabled(_selectedBehavior, true);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::OnDeactivatedInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  // disable process for selected behavior
  SetProcessEnabled(_selectedBehavior, false);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::SetProcessEnabled(const ICozmoBehaviorPtr behavior, bool newValue)
{
  const char* const kAIInformationAnalyzerLock = "SelectionBehaviorChooser";

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


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SelectionBehaviorChooser::UpdateInternal(BehaviorExternalInterface& behaviorExternalInterface)
{
  const auto& stateChangeComp = behaviorExternalInterface.GetStateChangeComponent();
  for(const auto& event: stateChangeComp.GetGameToEngineEvents()){
    if(event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByID){
      HandleExecuteBehavior(event);
    }else if(event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::ExecuteBehaviorByExecutableType){
      HandleExecuteBehavior(event);
    }
  }
}


} // namespace Cozmo
} // namespace Anki
