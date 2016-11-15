/**
 * File: blockTapFilterComponent.h
 *
 * Author: Molly Jameson
 * Created: 2016-07-07
 *
 * Description: A component to manage time delays to only send the most intense taps
 *               from blocks sent close together, since the other taps were likely noise
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_BlockTapFilterComponent_H__
#define __Anki_Cozmo_Basestation_Components_BlockTapFilterComponent_H__


#include "anki/common/types.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include <list>

namespace Anki {
namespace Cozmo {

class Robot;

class BlockTapFilterComponent : private Util::noncopyable
{
public:

  explicit BlockTapFilterComponent(Robot& robot);
  
  void Update();

private:
  
  void HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message);

  void HandleEnableTapFilter(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  
  Robot& _robot;

  Signal::SmartHandle _robotToEngineSignalHandle;
  Signal::SmartHandle _gameToEngineSignalHandle;
  bool _enabled;
  Anki::TimeStamp_t _waitToTime;
  std::list<ObjectTapped> _tapInfo;
  
#if ANKI_DEV_CHEATS
  void HandleSendTapFilterStatus(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  Signal::SmartHandle _debugGameToEngineSignalHandle;
#endif

};

}
}

#endif
