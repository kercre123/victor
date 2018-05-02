/**
 * File: cubeBleClient_vicos.cpp
 *
 * Author: Matt Michini
 * Created: 12/1/2017
 *
 * Description:
 *               Defines interface to BLE central process which communicates with cubes
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cubeBleClient.h"

#include "anki-ble/common/anki_ble_uuids.h"
#include "bleClient/bleClient.h"

#include "clad/externalInterface/messageCubeToEngine.h"
#include "clad/externalInterface/messageEngineToCube.h"

#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/string/stringUtils.h"
#include "util/fileUtils/fileUtils.h"

#include <queue>
#include <thread>

#ifdef SIMULATOR
#error SIMULATOR should NOT be defined by any target using cubeBleClient_vicos.cpp
#endif

namespace Anki {
namespace Cozmo {

namespace {
  // The one and only cube we are connecting/connected to
  std::string _cubeAddress;

  // file location on disk where the cube firmware binary is located
  std::string _cubeFirmwarePath;

  // Flag indicating whether we've already flashed one cube on connection
  bool _checkedCubeFirmwareVersion = false;
  
  // Are we currently connected to the cube?
  bool _connected = false;
  
  struct ev_loop* _loop = nullptr;
  
  std::unique_ptr<BleClient> _bleClient = nullptr;
  
  // shared queue for buffering cube messages received on the client thread
  using CubeMsgRecvBuffer = std::queue<std::vector<uint8_t>>;
  CubeMsgRecvBuffer _cubeMsgRecvBuffer;
  std::mutex _cubeMsgRecvBuffer_mutex;
  
  // shared queue for buffering advertisement messages received on the client thread
  struct CubeAdvertisementInfo {
    CubeAdvertisementInfo(const std::string& addr, const int rssi) : addr(addr), rssi(rssi) { }
    std::string addr;
    int rssi;
  };
  using CubeAdvertisementBuffer = std::queue<CubeAdvertisementInfo>;
  CubeAdvertisementBuffer _cubeAdvertisementBuffer;
  std::mutex _cubeAdvertisementBuffer_mutex;

  // Flag indicating when scanning for cubes has completed
  std::atomic<bool> _scanningFinished{false};

  // Flag indicating whether the connected cube's firmware version is correct
  std::atomic<bool> _cubeFirmwareVersionMatch{true};
}

  
CubeBleClient::CubeBleClient()
{
  _loop = ev_default_loop(EVBACKEND_SELECT);
  _bleClient = std::make_unique<BleClient>(_loop);
  
  _bleClient->RegisterAdvertisementCallback([](const std::string& addr, const int rssi) {
    std::lock_guard<std::mutex> lock(_cubeAdvertisementBuffer_mutex);
    _cubeAdvertisementBuffer.emplace(addr, rssi);
  });
  
  _bleClient->RegisterReceiveDataCallback([](const std::string& addr, const std::vector<uint8_t>& data){
    std::lock_guard<std::mutex> lock(_cubeMsgRecvBuffer_mutex);
    _cubeMsgRecvBuffer.push(data);
  });
  
  _bleClient->RegisterScanFinishedCallback([](){
    _scanningFinished = true;
  });

  _bleClient->RegisterReceiveFirmwareVersionCallback([](const std::string& addr, const std::string& connectedCubeFirmwareVersion) {
    std::string versionOnDisk = "";
    std::vector<uint8_t> firmware = Util::FileUtils::ReadFileAsBinary(_cubeFirmwarePath);
    size_t offset = 0x10; // The first 16 bytes of the firmware data are the version string
    if(firmware.size() > offset) {
      versionOnDisk = std::string(firmware.begin(), firmware.begin() + offset);
    }
    _cubeFirmwareVersionMatch = connectedCubeFirmwareVersion.compare(versionOnDisk)==0;
  });
}


CubeBleClient::~CubeBleClient()
{
  _bleClient->Stop();
  _bleClient.reset();
  ev_loop_destroy(_loop);
  _loop = nullptr;
}

bool CubeBleClient::Init()
{
  DEV_ASSERT(!_inited, "CubeBleClient.Init.AlreadyInitialized");

  _bleClient->Start();
  
  _inited = true;
  return true;
}

bool CubeBleClient::Update()
{
  if (!_inited) {
    DEV_ASSERT(false, "CubeBleClient.Update.NotInited");
    return false;
  }
  
  // Check for connection state changes
  if (!_connected && _bleClient->IsConnectedToCube()) {
    PRINT_NAMED_INFO("CubeBleClient.Update.ConnectedToCube",
                     "Connected to cube %s",
                     _cubeAddress.c_str());
    _connected = true;
    for (const auto& callback : _cubeConnectionCallbacks) {
      callback(_cubeAddress, true);
    }
  } else if (_connected && !_bleClient->IsConnectedToCube()) {
    PRINT_NAMED_INFO("CubeBleClient.Update.DisconnectedFromCube",
                     "Disconnected from cube %s",
                     _cubeAddress.c_str());
    _connected = false;
    for (const auto& callback : _cubeConnectionCallbacks) {
      callback(_cubeAddress, false);
    }
    _cubeAddress.clear();
  }
  
  // Pull advertisement messages from queue into a temp queue,
  // to avoid holding onto the mutex for too long.
  CubeAdvertisementBuffer swapCubeAdvertisementBuffer;
  {
    std::lock_guard<std::mutex> lock(_cubeAdvertisementBuffer_mutex);
    swapCubeAdvertisementBuffer.swap(_cubeAdvertisementBuffer);
  }
  
  while (!swapCubeAdvertisementBuffer.empty()) {
    const auto& data = swapCubeAdvertisementBuffer.front();
    ExternalInterface::ObjectAvailable msg;
    msg.factory_id = data.addr;
    msg.objectType = ObjectType::Block_LIGHTCUBE1; // TODO - update this with the Victor cube type once it's defined
    msg.rssi = Util::numeric_cast_clamped<decltype(msg.rssi)>(data.rssi);
    for (const auto& callback : _objectAvailableCallbacks) {
      callback(msg);
    }
    swapCubeAdvertisementBuffer.pop();
  }

  // check firmware versions -- if no match, prepare to flash the cube
  // note: only do this once after connecting to a cube
  if(!_cubeFirmwareVersionMatch && !_checkedCubeFirmwareVersion) {
    std::vector<uint8_t> firmware = Util::FileUtils::ReadFileAsBinary(_cubeFirmwarePath);
    _bleClient->FlashCube(std::move(firmware));
    _checkedCubeFirmwareVersion = true;
    _cubeFirmwareVersionMatch = true;
  }
  
  // Pull cube messages from queue into a temp queue,
  // to avoid holding onto the mutex for too long.
  CubeMsgRecvBuffer swapCubeMsgRecvBuffer;
  {
    std::lock_guard<std::mutex> lock(_cubeMsgRecvBuffer_mutex);
    swapCubeMsgRecvBuffer.swap(_cubeMsgRecvBuffer);
  }
  
  while (!swapCubeMsgRecvBuffer.empty()) {
    const auto& data = swapCubeMsgRecvBuffer.front();
    for (const auto& callback : _cubeMessageCallbacks) {
      MessageCubeToEngine cubeMessage(data.data(), data.size());
      callback(_cubeAddress, cubeMessage);
    }
    swapCubeMsgRecvBuffer.pop();
  }
  
  // Check to see if scanning for cubes has finished
  if (_scanningFinished) {
    for (const auto& callback : _scanFinishedCallbacks) {
      callback();
    }
    _scanningFinished = false;
  }
  
  return true;
}


void CubeBleClient::StartScanUponConnection()
{
  DEV_ASSERT(!_inited, "CubeBleClient.StartScanningUponConnection.AlreadyInited");
  _bleClient->SetStartScanUponConnection();
}


void CubeBleClient::SetScanDuration(const float duration_sec)
{
  _bleClient->SetScanDuration(duration_sec);
}

void CubeBleClient::SetCubeFirmwareFilepath(const std::string& cubeFirmwarePath)
{
  _cubeFirmwarePath = cubeFirmwarePath;
}


void CubeBleClient::StartScanning()
{
  PRINT_NAMED_INFO("CubeBleClient.StartScanning",
                   "Starting to scan for available cubes");
  
  // Sending from this thread for now. May need to queue this and
  // send it on the client thread if ipc client is not thread safe.
  DEV_ASSERT(_bleClient != nullptr, "CubeBleClient.StartScanning.NullBleClient");
  _bleClient->StartScanForCubes();
}


void CubeBleClient::StopScanning()
{
  PRINT_NAMED_INFO("CubeBleClient.StopScanning",
                   "Stopping scan for available cubes");
  
  // Sending from this thread for now. May need to queue this and
  // send it on the client thread if ipc client is not thread safe.
  DEV_ASSERT(_bleClient != nullptr, "CubeBleClient.StopScanning.NullBleClient");
  _bleClient->StopScanForCubes();
}


bool CubeBleClient::SendMessageToLightCube(const BleFactoryId& factoryId, const MessageEngineToCube& msg)
{
  DEV_ASSERT_MSG(factoryId == _cubeAddress,
                 "CubeBleClient.SendMessageToLightCube.WrongAddress",
                 "Can only send a cube message to cube with address %s (requested address %s)",
                 _cubeAddress.c_str(),
                 factoryId.c_str());
  
  u8 buff[msg.Size()];
  msg.Pack(buff, msg.Size());
  const auto& msgVec = std::vector<u8>(buff, buff + msg.Size());

  // Sending from this thread for now. May need to queue this and
  // send it on the client thread if ipc client is not thread safe.
  DEV_ASSERT(_bleClient != nullptr, "CubeBleClient.SendMessageToLightCube.NullBleClient");
  return _bleClient->Send(msgVec);
}


bool CubeBleClient::ConnectToCube(const BleFactoryId& factoryId)
{
  if (_connected) {
    PRINT_NAMED_WARNING("CubeBleClient.ConnectToCube.AlreadyConnected",
                        "We are already connected to a cube (address %s)!",
                        _cubeAddress.c_str());
    return false;
  }
  
  DEV_ASSERT(_cubeAddress.empty(), "CubeBleClient.ConnectToCube.CubeAddressNotEmpty");
  
  _cubeAddress = factoryId;
  
  PRINT_NAMED_INFO("CubeBleClient.ConnectToCube.AttemptingToConnect",
                   "Attempting to connect to cube %s",
                   _cubeAddress.c_str());
  
  // Sending from this thread for now. May need to queue this and
  // send it on the client thread if ipc client is not thread safe.
  DEV_ASSERT(_bleClient != nullptr, "CubeBleClient.ConnectToCube.NullBleClient");
  _bleClient->ConnectToCube(_cubeAddress);
  return true;
}


bool CubeBleClient::DisconnectFromCube(const BleFactoryId& factoryId)
{
  if (!_connected) {
    PRINT_NAMED_WARNING("CubeBleClient.DisconnectFromCube.NotConnected",
                        "Cannot disconnect - we are not connected to any cubes!");
    return false;
  }
  
  DEV_ASSERT_MSG(factoryId == _cubeAddress,
                 "CubeBleClient.DisconnectFromCube.WrongAddress",
                 "We are not connected to cube with ID %s (current connected cube ID %s)",
                 factoryId.c_str(),
                 _cubeAddress.c_str());
  
  // Sending from this thread for now. May need to queue this and
  // send it on the client thread if ipc client is not thread safe.
  DEV_ASSERT(_bleClient != nullptr, "CubeBleClient.DisconnectFromCube.NullBleClient");
  _bleClient->DisconnectFromCube();
  return true;
}

} // namespace Cozmo
} // namespace Anki
