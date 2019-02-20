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

#include "engine/actions/basicActions.h"
#include "engine/actions/sayTextAction.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "osState/osState.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {

CONSOLE_VAR(bool, kTheBox_TTSForIP, "TheBox.Audio", false);


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
  _messageSent = false;

  WaitForImagesAction* waitAction = new WaitForImagesAction(5);
  DelegateIfInControl(waitAction, [this]() {
      // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
      Robot& robot = GetBEI().GetRobotInfo()._robot;

      const bool enable = true;
      robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableNetworkScreen( enable )));
      _messageSent = true;

      if( kTheBox_TTSForIP ) {
        const bool updateIP = true;
        const std::string& ip = OSState::getInstance()->GetIPAddress(updateIP);
        if( !ip.empty() ) {
          DelegateIfInControl(new SayTextAction(ip, SayTextAction::AudioTtsProcessingStyle::Unprocessed));
        }
      }
      else {
        // instead, play a sound effect
        const auto dingEvent = AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Scan_Face_Success;
        GetBEI().GetRobotAudioClient().PostEvent( dingEvent, AudioMetaData::GameObjectType::Behavior );
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorBoxDemoShowNetworkInfo::OnBehaviorDeactivated() 
{
  if( _messageSent ) {
    // DEPRECATED - Grabbing robot to support current cozmo code, but this should be removed
    Robot& robot = GetBEI().GetRobotInfo()._robot;

    const bool enable = false;
    robot.SendMessage(RobotInterface::EngineToRobot(RobotInterface::EnableNetworkScreen( enable )));
  }
}


}
}
