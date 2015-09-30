/**
 * File: demoBehaviorChooser.cpp
 *
 * Author: Lee Crippen
 * Created: 09/04/15
 *
 * Description: Class for handling picking of behaviors in a somewhat scripted demo.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/demoBehaviorChooser.h"

#include "anki/cozmo/basestation/behaviors/behaviorLookAround.h"
#include "anki/cozmo/basestation/behaviors/behaviorInteractWithFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageGameToEngine.h"


namespace Anki {
namespace Cozmo {
  
DemoBehaviorChooser::DemoBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
  , _robot(robot)
  , _liveIdleEnabled(false)
{
  SetupBehaviors(robot, config);
  
  if (_robot.HasExternalInterface())
  {
    _eventHandlers.push_back(_robot.GetExternalInterface()->Subscribe(ExternalInterface::MessageGameToEngineTag::SetDemoState,
                                                                      std::bind(&DemoBehaviorChooser::HandleSetDemoState,
                                                                                this, std::placeholders::_1)));
  }
}
  
void DemoBehaviorChooser::SetupBehaviors(Robot& robot, const Json::Value& config)
{
  // Setup InteractWithFaces behavior
  _behaviorInteractWithFaces = new BehaviorInteractWithFaces(robot, config);
  Result addResult = super::AddBehavior(_behaviorInteractWithFaces);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorInteractWithFaces was not created properly.");
    return;
  }
  
  // Setup OCD behavior
  _behaviorOCD = new BehaviorOCD(robot, config);
  addResult = super::AddBehavior(_behaviorOCD);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorOCD was not created properly.");
    return;
  }
  
  // Setup LookAround behavior
  _behaviorLookAround = new BehaviorLookAround(robot, config);
  addResult = super::AddBehavior(_behaviorLookAround);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorLookAround was not created properly.");
    return;
  }
  
  // Setup Fidget behavior
  _behaviorFidget = new BehaviorFidget(robot, config);
  addResult = super::AddBehavior(_behaviorFidget);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorFidget was not created properly.");
    return;
  }
}
  
Result DemoBehaviorChooser::Update(double currentTime_sec)
{
  if(!_liveIdleEnabled) {
    _robot.SetIdleAnimation(AnimationStreamer::LiveAnimation);
    _liveIdleEnabled = true;
  }
  
  Result updateResult = super::Update(currentTime_sec);
  if (Result::RESULT_OK != updateResult)
  {
    return updateResult;
  }
  
  _demoState = _requestedState;
  
  return Result::RESULT_OK;
}
  
IBehavior* DemoBehaviorChooser::ChooseNextBehavior(double currentTime_sec) const
{
  auto runnable = [currentTime_sec](const IBehavior* behavior)
  {
    return (nullptr != behavior && behavior->IsRunnable(currentTime_sec));
  };
  
  switch (_demoState)
  {
    case DemoBehaviorState::BlocksOnly:
    {
      if (runnable(_behaviorOCD))
      {
        return _behaviorOCD;
      }
      break;
    }
    case DemoBehaviorState::FacesOnly:
    {
      if (runnable(_behaviorInteractWithFaces))
      {
        return _behaviorInteractWithFaces;
      }
      break;
    }
    case DemoBehaviorState::Default:
    {
      if (runnable(_behaviorOCD))
      {
        return _behaviorOCD;
      }
      else if (runnable(_behaviorInteractWithFaces))
      {
        return _behaviorInteractWithFaces;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.ChooseNextBehavior", "Chooser in unhandled state!");
    }
  }
  
  // Shared logic if desired behavior isn't possible
  if(runnable(_behaviorLookAround))
  {
    return _behaviorLookAround;
  }
  else if (runnable(_behaviorFidget))
  {
    return _behaviorFidget;
  }
  
  return nullptr;
}
  
Result DemoBehaviorChooser::AddBehavior(IBehavior* newBehavior)
{
  PRINT_NAMED_ERROR("DemoBehaviorChooser.AddBehavior", "DemoBehaviorChooser does not add behaviors externally.");
  return Result::RESULT_FAIL;
}
  
void DemoBehaviorChooser::HandleSetDemoState(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
{
  const ExternalInterface::SetDemoState& msg = event.GetData().Get_SetDemoState();
  _requestedState = msg.demoState;
}

} // namespace Cozmo
} // namespace Anki