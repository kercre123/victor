/**
 * File: BehaviorBoxDemoShowNetworkInfo.cpp
 *
 * Author: Brad
 * Created: 2019-01-14
 *
 * Description: Show the network info (including IP address)
 *
 * Copyright: Anki, Inc. 2019
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/box/behaviorBoxDemoShowNetworkInfo.h"

#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "osState/osState.h"

namespace Anki {
namespace Vector {

CONSOLE_VAR_EXTERN(bool, kTheBox_TTSForDescription);


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoShowNetworkInfo::BehaviorBoxDemoShowNetworkInfo(const Json::Value& config)
 : ICozmoBehavior(config)
{
  // TODO: read config into _iConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorBoxDemoShowNetworkInfo::~BehaviorBoxDemoShowNetworkInfo()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorBoxDemoShowNetworkInfo::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowNetworkInfo::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.behaviorAlwaysDelegates = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowNetworkInfo::OnBehaviorActivated() 
{
  // TODO:(bn) use TTS to read IP?

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool enable = true;
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableNetworkScreen( enable )));

  if( kTheBox_TTSForDescription ) {
    const bool updateIP = true;
    const std::string& ip = OSState::getInstance()->GetIPAddress(updateIP);
    if( !ip.empty() ) {
      DelegateIfInControl(new SayTextAction(ip, SayTextAction::AudioTtsProcessingStyle::Unprocessed));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowNetworkInfo::OnBehaviorDeactivated() 
{
  // TODO:(bn) use TTS to read IP?

  // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
  Robot& robot = GetBEI().GetRobotInfo()._robot;

  const bool enable = false;
  robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableNetworkScreen( enable )));
}


}
}
