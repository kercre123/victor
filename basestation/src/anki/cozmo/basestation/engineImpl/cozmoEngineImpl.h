/*
 * File:          cozmoEngine.cpp
 * Date:          12/23/2014
 *
 * Description:   (See header file.)
 *
 * Author: Andrew Stein / Kevin Yoon
 *
 * Modifications:
 */

#ifndef __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineImpl_H__
#define __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineImpl_H__

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoEngine.h"
#include "anki/cozmo/basestation/multiClientChannel.h" // TODO: Remove?
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/visionProcessingThread.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/messaging/basestation/advertisementService.h"
#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/timer.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/signals/simpleSignal_fwd.h"
#include "anki/cozmo/basestation/robotMessageHandler.h"
#include "anki/cozmo/basestation/recording/playback.h"

#define DEVICE_VISION_MODE_OFF   0
#define DEVICE_VISION_MODE_SYNC  1
#define DEVICE_VISION_MODE_ASYNC 2

#define DEVICE_VISION_MOIDE DEVICE_VISION_MODE_OFF

namespace Anki {
namespace Cozmo {

class CozmoEngineImpl
{
public:

  CozmoEngineImpl(IExternalInterface* externalInterface, Util::Data::DataPlatform* dataPlatform);
  virtual ~CozmoEngineImpl();

  virtual Result Init(const Json::Value& config);

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(const BaseStationTime_t currTime_ns);

  // Provide an image from the device's camera for processing with the engine's
  // DeviceVisionProcessor
  void ProcessDeviceImage(const Vision::Image& image);

  using AdvertisingRobot    = CozmoEngine::AdvertisingRobot;
  //using AdvertisingUiDevice = CozmoEngine::AdvertisingUiDevice;

  //void GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots);

  virtual bool ConnectToRobot(AdvertisingRobot whichRobot);

  void DisconnectFromRobot(RobotID_t whichRobot);

  virtual void ReadAnimationsFromDisk() {};
protected:

  // Derived classes must implement any special initialization in this method,
  // which is called by Init().
  virtual Result InitInternal() = 0;

  // Derived classes must implement any per-tic updating they need done in this method.
  // Public Update() calls this automatically.
  virtual Result UpdateInternal(const BaseStationTime_t currTime_ns) = 0;

  bool                      _isInitialized;

  int                       _engineID;

  Json::Value               _config;

  MultiClientChannel        _robotChannel;

  IExternalInterface* _externalInterface;
  Util::Data::DataPlatform* _dataPlatform;
  
  /*
  // TODO: Merge this into RobotManager
  // Each engine can potetnailly talk to multiple physical robots.
  // Package up the stuff req'd to deal with one robot and store a map
  // of them keyed by robot ID.
  struct RobotContainer {
    VisionProcessingThread    visionThread;
    RobotMessageHandler       visionMsgHandler;
  };
  std::map<AdvertisingRobot, RobotContainer> _connectedRobots;
  */

# if DEVISION_VISION_MODE != DEVICE_VISION_MODE_OFF
  VisionProcessingThread    _deviceVisionThread;
# endif

  std::vector<Signal::SmartHandle> _signalHandles;

}; // class CozmoEngineImpl

} // namespace Cozmo
} // namespace Anki


#endif // __Anki_Cozmo_Basestation_EngineImpl_CozmoEngineImpl_H__
