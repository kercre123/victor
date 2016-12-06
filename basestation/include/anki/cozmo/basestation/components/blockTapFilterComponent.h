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
#include "util/global/globalDefinitions.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/common/basestation/objectIDs.h"
#include <list>

namespace Anki {
namespace Cozmo {

class Robot;

class BlockTapFilterComponent : private Util::noncopyable
{
public:

  explicit BlockTapFilterComponent(Robot& robot);
  
  void Update();
  
  bool ShouldIgnoreMovementDueToDoubleTap(const ObjectID& objectID);

private:
  
  void HandleActiveObjectTapped(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleActiveObjectMoved(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleActiveObjectStopped(const AnkiEvent<RobotInterface::RobotToEngine>& message);

  void HandleEnableTapFilter(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  
  void CheckForDoubleTap(const ObjectID& objectID);
  
  Robot& _robot;

  std::vector<Signal::SmartHandle> _robotToEngineSignalHandle;
  Signal::SmartHandle _gameToEngineSignalHandle;
  bool _enabled;
  Anki::TimeStamp_t _waitToTime;
  
  struct DoubleTapInfo {
    // The time we should stop waiting for a double tap
    TimeStamp_t doubleTapTime = 0;
    
    // Whether or not the object is moving
    bool isMoving = false;
    
    // The time we should stop ignoring move messages for the objectID this DoubleTapInfo
    // maps to
    TimeStamp_t ignoreNextMoveTime = 0;
  };
  
  std::map<ObjectID, DoubleTapInfo> _doubleTapObjects;
  std::list<ObjectTapped> _tapInfo;
  
#if ANKI_DEV_CHEATS
  void HandleSendTapFilterStatus(const AnkiEvent<ExternalInterface::MessageGameToEngine>& message);
  Signal::SmartHandle _debugGameToEngineSignalHandle;
#endif

};

}
}

#endif
