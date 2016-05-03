/**
 * File: robotEventHandler.h
 *
 * Author: Lee
 * Created: 08/11/15
 *
 * Description: Class for subscribing to and handling events going to robots.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_RobotEventHandler_H__
#define __Cozmo_Basestation_RobotEventHandler_H__

#include "anki/types.h"
#include "util/signals/simpleSignal_fwd.h"
#include "util/helpers/noncopyable.h"

#include <vector>

namespace Anki {
namespace Cozmo {

// Forward declarations
class ActionList;
class IActionRunner;
class IExternalInterface;
class RobotManager;
class CozmoContext;
  
enum class QueueActionPosition : uint8_t;

template <typename Type>
class AnkiEvent;

namespace ExternalInterface {
  class MessageGameToEngine;
}


class RobotEventHandler : private Util::noncopyable
{
public:
  RobotEventHandler(const CozmoContext* context);
  
protected:
  const CozmoContext* _context;
  std::vector<Signal::SmartHandle> _signalHandles;
  
  using GameToEngineEvent = AnkiEvent<ExternalInterface::MessageGameToEngine>;
  
  void HandleActionEvents(const GameToEngineEvent& event);
  void HandleQueueSingleAction(const GameToEngineEvent& event);
  void HandleQueueCompoundAction(const GameToEngineEvent& event);
  void HandleSetLiftHeight(const GameToEngineEvent& event);
  void HandleEnableLiftPower(const GameToEngineEvent& event);
  void HandleEnableCliffSensor(const GameToEngineEvent& event);
  void HandleDisplayProceduralFace(const GameToEngineEvent& event);
  void HandleForceDelocalizeRobot(const GameToEngineEvent& event);
  void HandleBehaviorManagerEvent(const GameToEngineEvent& event);
  void HandleSendAvailableObjects(const GameToEngineEvent& event);
  void HandleSaveCalibrationImage(const GameToEngineEvent& event);
  void HandleClearCalibrationImages(const GameToEngineEvent& event);
  void HandleComputeCameraCalibration(const GameToEngineEvent& event);
  void HandleCameraCalibration(const GameToEngineEvent& event);
};

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_RobotEventHandler_H__
