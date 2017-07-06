/**
* File: strategyNoPreDockPoses.cpp
*
* Author: Kevin M. Karol
* Created: 12/08/16
*
* Description: Strategy for responding to the fact that an object has no predock
* poses
*
* Copyright: Anki, Inc. 2016
*
*
**/

#include "anki/cozmo/basestation/behaviorSystem/wantsToRunStrategies/strategyNoPreDockPoses.h"

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
#include "anki/cozmo/basestation/aiComponent/aiComponent.h"
#include "anki/cozmo/basestation/aiComponent/AIWhiteboard.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqAcknowledgeObject.h"
#include "anki/cozmo/basestation/robot.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StrategyNoPreDockPoses::StrategyNoPreDockPoses(Robot& robot, const Json::Value& config)
: IWantsToRunStrategy(robot, config)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StrategyNoPreDockPoses::WantsToRunInternal(const Robot& robot) const
{
  const ObjectID& objID = robot.GetAIComponent().GetWhiteboard().GetNoPreDockPosesOnObject();
  if(objID.IsSet()){
    ObjectID unsetObj;
    robot.GetAIComponent().GetNonConstWhiteboard().SetNoPreDockPosesOnObject(unsetObj);
    return true;
  }
  
  return false;
}

  
} // namespace Cozmo
} // namespace Anki
