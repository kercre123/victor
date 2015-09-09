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
#include "anki/cozmo/basestation/behaviors/behaviorLookForFaces.h"
#include "anki/cozmo/basestation/behaviors/behaviorOCD.h"
#include "anki/cozmo/basestation/behaviors/behaviorFidget.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"


namespace Anki {
namespace Cozmo {
  
DemoBehaviorChooser::DemoBehaviorChooser(Robot& robot)
  : ReactionaryBehaviorChooser()
  , _robot(robot)
{
}
  
Result DemoBehaviorChooser::Update(double currentTime_sec)
{
  Result updateResult = ReactionaryBehaviorChooser::Update(currentTime_sec);
  static double initBlocksTime = currentTime_sec;
  
  switch (_demoState)
  {
    case DemoState::Faces:
    {
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
  return updateResult;
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
      if (runnable(_behaviorLookForFaces))
      {
        return _behaviorLookForFaces;
      }
      break;
    }
    case DemoState::Rest:
    {
      if (runnable(_behaviorLookForFaces))
      {
        return _behaviorLookForFaces;
      }
      else if (runnable(_behaviorOCD))
      {
        return _behaviorOCD;
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

Result DemoBehaviorChooser::AddBehaviorLookAround(BehaviorLookAround* behavior)
{
  Result addResult = AddBehavior(behavior);
  if (Result::RESULT_OK == addResult)
  {
    _behaviorLookAround = behavior;
  }
  return addResult;
}

Result DemoBehaviorChooser::AddBehaviorLookForFaces(BehaviorLookForFaces* behavior)
{
  Result addResult = AddBehavior(behavior);
  if (Result::RESULT_OK == addResult)
  {
    _behaviorLookForFaces = behavior;
  }
  return addResult;
}

Result DemoBehaviorChooser::AddBehaviorOCD(BehaviorOCD* behavior)
{
  Result addResult = AddBehavior(behavior);
  if (Result::RESULT_OK == addResult)
  {
    _behaviorOCD = behavior;
  }
  return addResult;
}

Result DemoBehaviorChooser::AddBehaviorFidget(BehaviorFidget* behavior)
{
  Result addResult = AddBehavior(behavior);
  if (Result::RESULT_OK == addResult)
  {
    _behaviorFidget = behavior;
  }
  return addResult;
}

} // namespace Cozmo
} // namespace Anki