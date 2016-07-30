/**
 * File: behaviorReactAcknowledgeCubeMoved.cpp
 *
 * Author: Kevin M. Karol
 * Created: 7/26/16
 *
 * Description: Behavior to acknowledge when a localized cube has been moved
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactAcknowledgeCubeMoved.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

using namespace ExternalInterface;

BehaviorReactAcknowledgeCubeMoved::BehaviorReactAcknowledgeCubeMoved(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactAcknowledgeCubeMoved");
  
  SubscribeToTriggerTags(robot.GetID(), {{
    RobotInterface::RobotToEngineTag::activeObjectTapped,
    RobotInterface::RobotToEngineTag::activeObjectMoved
  }});
  
}

bool BehaviorReactAcknowledgeCubeMoved::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

Result BehaviorReactAcknowledgeCubeMoved::InitInternalReactionary(Robot& robot)
{
  //TODO: Merging seperately
  return Result::RESULT_OK;
}

IBehavior::Status BehaviorReactAcknowledgeCubeMoved::UpdateInternal(Robot& robot)
{
  //TODO: Merging seperately
  return Status::Running;
}

void BehaviorReactAcknowledgeCubeMoved::StopInternalReactionary(Robot& robot)
{
  
}


void BehaviorReactAcknowledgeCubeMoved::AlwaysHandleInternal(const RobotToEngineEvent& event, const Robot& robot)
{
  uint32_t objID = 0;
  switch(event.GetData().GetTag()){
    case RobotInterface::RobotToEngineTag::activeObjectMoved:
      objID = event.GetData().Get_activeObjectMoved().objectID;
      break;
      
    case RobotInterface::RobotToEngineTag::activeObjectTapped:
      objID = event.GetData().Get_activeObjectTapped().objectID;
      break;
      
    default:
      break;
  }
  
  //TODO: Merging seperately

}
  
} // namespace Cozmo
} // namespace Anki