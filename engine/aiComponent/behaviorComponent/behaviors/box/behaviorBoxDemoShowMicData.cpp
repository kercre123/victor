/**
 * File: BehaviorBoxDemoShowMicData.cpp
 *
 * Author: Brad
 * Created: 2019-01-14
 *
 * Description: Displays microphone data on robot (through face info screen currently)
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoShowMicData.h"

#include "engine/actions/basicActions.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"

namespace Anki {
namespace Vector {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoShowMicData::BehaviorBoxDemoShowMicData(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoShowMicData::~BehaviorBoxDemoShowMicData()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoShowMicData::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowMicData::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowMicData::OnBehaviorActivated() 
{
  _messageSent = false;

  WaitForImagesAction* waitAction = new WaitForImagesAction(5);
  DelegateIfInControl(waitAction, [this]() {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
      Robot& robot = GetBEI().GetRobotInfo()._robot;
      const bool enable = true;
      robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableMicDirectionScreen( enable )));
      _messageSent = true;
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowMicData::OnBehaviorDeactivated()
{
  if( _messageSent ) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    const bool enable = false;
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableMicDirectionScreen( enable )));
  }
}


}
}
