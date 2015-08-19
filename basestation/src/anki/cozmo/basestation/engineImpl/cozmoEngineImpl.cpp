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

#include "anki/cozmo/basestation/engineImpl/cozmoEngineImpl.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  

  
CozmoEngineImpl::CozmoEngineImpl(IExternalInterface* externalInterface, Data::DataPlatform* dataPlatform)
: _isInitialized(false)
, _externalInterface(externalInterface)
, _dataPlatform(dataPlatform)
#if DEVICE_VISION_MODE != DEVICE_VISION_MODE_OFF
, _deviceVisionThread(dataPlatform)
#endif
{
  if (Anki::Util::gTickTimeProvider == nullptr) {
    Anki::Util::gTickTimeProvider = BaseStationTimer::getInstance();
  }

  ASSERT_NAMED(externalInterface != nullptr, "Cozmo.EngineImpl.ExternalInterface.nullptr");
}

CozmoEngineImpl::~CozmoEngineImpl()
{
  _robotChannel.Stop();

  if (Anki::Util::gTickTimeProvider == BaseStationTimer::getInstance()) {
    Anki::Util::gTickTimeProvider = nullptr;
  }

  BaseStationTimer::removeInstance();
}

Result CozmoEngineImpl::Init(const Json::Value& config)
{
  if(_isInitialized) {
    PRINT_NAMED_INFO("CozmoEngineImpl.Init.ReInit", "Reinitializing already-initialized CozmoEngineImpl with new config.");
  }

  _config = config;

  if(!_config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "No AdvertisingHostIP defined in Json config.");
    return RESULT_FAIL;
  }

  if(!_config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "No RobotAdvertisingPort defined in Json config.");
    return RESULT_FAIL;
  }

  if(!_config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "No UiAdvertisingPort defined in Json config.");
    return RESULT_FAIL;
  }

  Vision::CameraCalibration deviceCamCalib;
  if(!_config.isMember(AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION)) {
    PRINT_NAMED_WARNING("CozmoEngine.Init",
                        "No DeviceCameraCalibration defined in Json config. Using bogus settings.");
  } else {
    deviceCamCalib.Set(_config[AnkiUtil::kP_DEVICE_CAMERA_CALIBRATION]);
  }

  Result lastResult = RESULT_OK;
  const char *ipString = _config[AnkiUtil::kP_ADVERTISING_HOST_IP].asCString();
  int port =_config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT].asInt();
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize RobotComms; bad port.");
    return RESULT_FAIL;
  }
  Anki::Util::TransportAddress address(ipString, static_cast<uint16_t>(port));
  PRINT_STREAM_DEBUG("CozmoEngine.Init", "Initializing on address: " << address << ": " << address.GetIPAddress() << ":" << address.GetIPPort() << "; originals: " << ipString << ":" << port);

  _robotChannel.Start(address);
  if(!_robotChannel.IsStarted()) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "Failed to initialize RobotComms.");
    return RESULT_FAIL;
  }

  if(!_config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
    PRINT_NAMED_WARNING("CozmoEngineInit.NoVizHostIP",
                        "No VizHostIP member in JSON config file. Not initializing VizManager.");
  } else if(!_config[AnkiUtil::kP_VIZ_HOST_IP].asString().empty()){
    VizManager::getInstance()->Connect(_config[AnkiUtil::kP_VIZ_HOST_IP].asCString(), VIZ_SERVER_PORT);

    // Erase anything that's still being visualized in case there were leftovers from
    // a previous run?? (We should really be cleaning up after ourselves when
    // we tear down, but it seems like Webots restarts aren't always allowing
    // the cleanup to happen)
    VizManager::getInstance()->EraseAllVizObjects();

    // Only send images if the viz host is the same as the robot advertisement service
    // (so we don't waste bandwidth sending (uncompressed) viz data over the network
    //  to be displayed on another machine)
    if(_config[AnkiUtil::kP_VIZ_HOST_IP] == _config[AnkiUtil::kP_ADVERTISING_HOST_IP]) {
      VizManager::getInstance()->EnableImageSend(true);
    }
  }

  lastResult = InitInternal();
  if(lastResult != RESULT_OK) {
    PRINT_NAMED_ERROR("CozomEngine.Init", "Failed calling internal init.");
    return lastResult;
  }

# if DEVICE_VISION_MODE == DEVICE_VISION_MODE_ASYNC
  // TODO: Only start when needed?
  _deviceVisionThread.Start(deviceCamCalib);
# elif DEVICE_VISION_MODE == DEVICE_VISION_MODE_SYNC
  _deviceVisionThread.SetCameraCalibration(deviceCamCalib);
# endif

  _isInitialized = true;

  return RESULT_OK;
} // Init()

/*
void CozmoEngineImpl::GetAdvertisingRobots(std::vector<AdvertisingRobot>& advertisingRobots)
{
  _robotChannel.Update();
  _robotChannel.GetAdvertisingDeviceIDs(advertisingRobots);
}
 */


bool CozmoEngineImpl::ConnectToRobot(AdvertisingRobot whichRobot)
{
  Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
  bool success = _robotChannel.AcceptAdvertisingConnection(id);
  if (success) {
    //_connectedRobots[whichRobot];
    //_connectedRobots[whichRobot].visionThread.Start();
    //_connectedRobots[whichRobot].visionMsgHandler.Init(<#Comms::IComms *comms#>, <#Anki::Cozmo::RobotManager *robotMgr#>)
  }

  return success;
}

void CozmoEngineImpl::DisconnectFromRobot(RobotID_t whichRobot) {
  Anki::Comms::ConnectionId id = static_cast<Anki::Comms::ConnectionId>(whichRobot);
  _robotChannel.RemoveConnection(id);
  /*
  auto connectedRobotIter = _connectedRobots.find(whichRobot);
  if(connectedRobotIter != _connectedRobots.end()) {
    _connectedRobots.erase(connectedRobotIter);
  }
   */
}

Result CozmoEngineImpl::Update(const BaseStationTime_t currTime_ns)
{
  if(!_isInitialized) {
    PRINT_NAMED_ERROR("CozmoEngine.Init", "Cannot update CozmoEngine before it is initialized.\n");
    return RESULT_FAIL;
  }

  // Check if engine tic is exceeding expected time
  static TimeStamp_t prevTime = 0;
  TimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  if (prevTime != 0) {
    TimeStamp_t timeSinceLastTic = currTime - prevTime;
    // Print warning if this tic was executed more than the expected amount of time
    // (with some margin) after the last tic.
    if (timeSinceLastTic > BS_TIME_STEP + 10) {
      PRINT_NAMED_WARNING("CozmoEngine.Update.LateTic",
                          "currTime %dms, timeSinceLastTic %dms (should be ~%dms)\n",
                          currTime, timeSinceLastTic, BS_TIME_STEP);
    }
  }
  prevTime = currTime;


  // Notify any listeners that robots are advertising
  std::vector<Comms::ConnectionId> advertisingRobots;
  _robotChannel.GetAdvertisingConnections(advertisingRobots);
  for(auto robot : advertisingRobots) {
    _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::RobotAvailable(robot)));
  }

  // TODO: Handle images coming from connected robots
  /*
  for(auto & robotKeyPair : _connectedRobots) {
    robotKeyPair.second.visionMsgHandler.ProcessMessages();
  }
   */

  Result lastResult = UpdateInternal(currTime_ns);

  return lastResult;
} // Update()

void CozmoEngineImpl::ProcessDeviceImage(const Vision::Image &image)
{
  // Process image within the detection rectangle with vision processing thread:
  static const Cozmo::MessageRobotState bogusState; // req'd by API, but not really necessary for marker detection

# if DEVICE_VISION_MODE == DEVICE_VISION_MODE_ASYNC
  _deviceVisionThread.SetNextImage(image, bogusState);
# elif DEVICE_VISION_MODE == DEVICE_VISION_MODE_SYNC
  _deviceVisionThread.Update(image, bogusState);

  MessageVisionMarker msg;
  while(_deviceVisionThread.CheckMailbox(msg)) {
    // Pass marker detections along to UI/game for use
    _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::DeviceDetectedVisionMarker(
      msg.markerType,
      msg.x_imgUpperLeft,  msg.y_imgUpperLeft,
      msg.x_imgLowerLeft,  msg.y_imgLowerLeft,
      msg.x_imgUpperRight, msg.y_imgUpperRight,
      msg.x_imgLowerRight, msg.y_imgLowerRight
    )));
  }
# endif // DEVICE_VISION_MODE
  
}
  


} // namespace Cozmo
} // namespace Anki