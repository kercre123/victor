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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorFactory.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageGameToEngine.h"


// MoodManager for demo chooser is enabled by default now
#define USE_MOOD_MANAGER_FOR_DEMO_CHOOSER   1


namespace Anki {
namespace Cozmo {
  
DemoBehaviorChooser::DemoBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
  , _robot(robot)
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
  BehaviorFactory& behaviorFactory = robot.GetBehaviorFactory();

  _behaviorInteractWithFaces = behaviorFactory.CreateBehavior(BehaviorType::InteractWithFaces, robot, config);
  _behaviorOCD               = behaviorFactory.CreateBehavior(BehaviorType::OCD, robot, config);
  _behaviorLookAround        = behaviorFactory.CreateBehavior(BehaviorType::LookAround, robot, config);
  _behaviorNone              = behaviorFactory.CreateBehavior(BehaviorType::NoneBehavior, robot, config);
  //_behaviorFidget            = behaviorFactory.CreateBehavior(BehaviorType::Fidget, robot, config);
  
  for (const auto& it : behaviorFactory.GetBehaviorMap())
  {
    IBehavior* behaviorToAdd = it.second;
    const Result addResult = super::AddBehavior(behaviorToAdd);
    if (Result::RESULT_OK != addResult)
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors.FailedToAdd",
                        "BehaviorFactory behavior '%s' failed to add to chooser!", behaviorToAdd->GetName().c_str());
    }
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
  
IBehavior* DemoBehaviorChooser::ChooseNextBehavior(const Robot& robot, double currentTime_sec) const
{
#if USE_MOOD_MANAGER_FOR_DEMO_CHOOSER
  return SimpleBehaviorChooser::ChooseNextBehavior(robot, currentTime_sec);
#else
  auto runnable = [&robot,currentTime_sec](const IBehavior* behavior)
  {
    return (nullptr != behavior && behavior->IsRunnable(robot, currentTime_sec));
  };
  
  IBehavior* forcedChoice = nullptr;
  
  switch (_demoState)
  {
    case DemoBehaviorState::BlocksOnly:
    {
      if (runnable(_behaviorOCD))
      {
        forcedChoice = _behaviorOCD;
      }
      break;
    }
    case DemoBehaviorState::FacesOnly:
    {
      if (runnable(_behaviorInteractWithFaces))
      {
        forcedChoice = _behaviorInteractWithFaces;
      }
      break;
    }
    case DemoBehaviorState::Default:
    {
      if (runnable(_behaviorOCD))
      {
        forcedChoice = _behaviorOCD;
      }
      else if (runnable(_behaviorInteractWithFaces))
      {
        forcedChoice = _behaviorInteractWithFaces;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.ChooseNextBehavior", "Chooser in unhandled state!");
    }
  }

  if (forcedChoice)
  {
    return forcedChoice;
  }
  else if(runnable(_behaviorLookAround))
  {
    return _behaviorLookAround;
  }
  else if (runnable(_behaviorFidget))
  {
    return _behaviorFidget;
  }
  else if (runnable(_behaviorNone))
  {
    return _behaviorNone;
  }
  
  return nullptr;
  #endif // USE_MOOD_MANAGER_FOR_DEMO_CHOOSER
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