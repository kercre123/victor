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
#include "anki/cozmo/basestation/behaviors/behaviorNone.h"
#include "anki/cozmo/basestation/behaviors/behaviorPlayAnim.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageGameToEngine.h"


// Disabled by default for now - it seems to work fine as far as I can tell but i don't want to risk breaking anyone
#define USE_MOOD_MANAGER_FOR_DEMO_CHOOSER   0


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
  
  /*
  // Setup Fidget behavior
  _behaviorFidget = new BehaviorFidget(robot, config);
  addResult = super::AddBehavior(_behaviorFidget);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorFidget was not created properly.");
    return;
  }
  */
  
  // Setup None Behavior
  _behaviorNone = new BehaviorNone(robot, config);
  addResult = super::AddBehavior(_behaviorNone);
  if (Result::RESULT_OK != addResult)
  {
    PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorNone was not created properly.");
    return;
  }
  
  // Setup Happy React
  {
    BehaviorPlayAnim* behaviorPlayAnim = new BehaviorPlayAnim(robot, config);
    addResult = super::AddBehavior(behaviorPlayAnim);
    
    if (Result::RESULT_OK == addResult)
    {
      behaviorPlayAnim->SetAnimationName("happy");
      behaviorPlayAnim->SetName("HappyReact");
      behaviorPlayAnim->SetMinTimeBetweenRuns(5.0);
      behaviorPlayAnim->AddEmotionScorer(EmotionScorer(EmotionType::Happiness,
                                                       Anki::Util::GraphEvaluator2d({{0.01f, 0.0f}, {0.01f, 1.0f}, {1.0f, 1.0f}}), true));
    }
    else
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorPlayAnim was not created properly.");
      return;
    }
  }
  
  // Setup Sad React
  {
    BehaviorPlayAnim* behaviorPlayAnim = new BehaviorPlayAnim(robot, config);
    addResult = super::AddBehavior(behaviorPlayAnim);
    
    if (Result::RESULT_OK == addResult)
    {
      behaviorPlayAnim->SetAnimationName("sad");
      behaviorPlayAnim->SetName("SadReact");
      behaviorPlayAnim->SetMinTimeBetweenRuns(5.0);
      behaviorPlayAnim->AddEmotionScorer(EmotionScorer(EmotionType::Happiness,
                                                       Anki::Util::GraphEvaluator2d({{-1.0f, 1.0f}, {-0.01f, 1.0f}, {-0.01f, 0.0f}}), true));
    }
    else
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.SetupBehaviors", "BehaviorPlayAnim was not created properly.");
      return;
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
  
  #if USE_MOOD_MANAGER_FOR_DEMO_CHOOSER
  {
    IBehavior* simpleChoice = SimpleBehaviorChooser::ChooseNextBehavior(robot, currentTime_sec);
    
    if (forcedChoice)
    {
      if (simpleChoice && (forcedChoice != simpleChoice) && simpleChoice->IsShortInterruption())
      {
        // maybe still force simpleChoice if sufficiently better scoring?
        // or if it's a short behavior?
        const float forcedChoiceScore = forcedChoice->EvaluateScore(robot, currentTime_sec);
        const float simpleChoiceScore = simpleChoice->EvaluateScore(robot, currentTime_sec);
        if (simpleChoiceScore > (forcedChoiceScore + 0.05f))
        {
          return simpleChoice;
        }
      }
      
      return forcedChoice;
    }
    
    return simpleChoice;
  }
  #else
  {
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
  }
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