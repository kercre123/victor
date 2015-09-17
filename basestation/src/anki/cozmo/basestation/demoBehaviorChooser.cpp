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
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {
  
DemoBehaviorChooser::DemoBehaviorChooser(Robot& robot, const Json::Value& config)
  : super()
  , _robot(robot)
{
  SetupBehaviors(robot, config);
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
  Result updateResult = super::Update(currentTime_sec);
  if (Result::RESULT_OK != updateResult)
  {
    return updateResult;
  }
  
  static double initBlocksTime = currentTime_sec;
  
  switch (_demoState)
  {
    case DemoState::Faces:
    {
      if (_robot.GetEmotionManager().GetEmotion(EmotionManager::SCARED) == EmotionManager::MAX_VALUE)
      {
        _demoState = DemoState::Blocks;
        initBlocksTime = currentTime_sec;
      }
      break;
    }
    case DemoState::Blocks:
    {
      if ((currentTime_sec - initBlocksTime) > kBlocksBoredomTime
          && nullptr != _behaviorOCD
          && !_behaviorOCD->IsRunnable(currentTime_sec))
      {
        _demoState = DemoState::Rest;
      }
      break;
    }
    case DemoState::Rest:
    {
      // Once we're resting we stay resting
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("DemoBehaviorChooser.Update", "Chooser in unhandled state!");
    }
  }
  
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
    case DemoState::Blocks:
    {
      if (runnable(_behaviorOCD))
      {
        return _behaviorOCD;
      }
      break;
    }
    case DemoState::Faces:
    {
      if (runnable(_behaviorInteractWithFaces))
      {
        return _behaviorInteractWithFaces;
      }
      break;
    }
    case DemoState::Rest:
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
  PRINT_NAMED_ERROR("DemoBehaviorChooser.AddBehavior", "DemoBehaviorChooser has unique methods for adding behaviors. Use those instead.");
  return Result::RESULT_FAIL;
}

} // namespace Cozmo
} // namespace Anki