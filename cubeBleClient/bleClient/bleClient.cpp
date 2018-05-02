/**
 * File: bleClient
 *
 * Author: Matt Michini (adapted from paluri)
 * Created: 3/28/2018
 *
 * Description: ble Client for ankibluetoothd. This is essentially a wrapper around
 *              IPCClient. Contains a thread which runs the ev loop for communicating
 *              with the server - keep this in mind when writing callbacks. Callbacks
 *              will occur on the ev loop thread.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
  
#include "bleClient.h"
#include "anki-ble/common/anki_ble_uuids.h"
#include "anki-ble/common/gatt_constants.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include <cmath>

namespace Anki {
namespace Cozmo {

namespace {
  // If we're not connected, keep trying to connect at this rate
  const ev_tstamp kConnectionCheckTime_sec = 0.5f;
  
  // If we're already connected, check for disconnection at this rate
  const ev_tstamp kDisconnectionCheckTime_sec = 5.f;

  // bytes per packet when performing OTA flash of the cubes
  const int kMaxBytesPerPacket = 20;
}


BleClient::BleClient(struct ev_loop* loop)
  : IPCClient(loop)
  , _loop(loop)
  , _asyncBreakSignal(loop)
  , _asyncStartScanSignal(loop)
  , _connectionCheckTimer(loop)
  , _scanningTimer(loop)
  , _asyncFlashCubeSignal(loop)
  , _cubeFirmware()
{
  // Set up watcher callbacks
  _asyncBreakSignal.set<BleClient, &BleClient::AsyncBreakCallback>(this);
  _asyncStartScanSignal.set<BleClient, &BleClient::AsyncStartScanCallback>(this);
  _connectionCheckTimer.set<BleClient, &BleClient::ConnectionCheckTimerCallback>(this);
  _scanningTimer.set<BleClient, &BleClient::ScanningTimerCallback>(this);
  _asyncFlashCubeSignal.set<BleClient, &BleClient::AsyncFlashCubeCallback>(this);
}

  
void BleClient::Start()
{
  // Create a thread which will run the ev_loop for server comms
  auto threadFunc = [this](){
    if (!Connect()) {
      PRINT_NAMED_WARNING("BleClient.LoopThread.ConnectFailed",
                          "Unable to connect to ble server - will retry");
    }
    
    // Start a connection check/retry timer to naively just always try to
    // reconnect if we become disconnected
    _connectionCheckTimer.start(kConnectionCheckTime_sec);
    
    // Start async watchers
    _asyncBreakSignal.start();
    _asyncStartScanSignal.start();
    _asyncFlashCubeSignal.start();
    
    // Start the loop (runs 'forever')
    ev_loop(_loop, 0);
  };
  
  // Kick off the thread
  _loopThread = std::thread(threadFunc);
  
}

void BleClient::Stop()
{
  if (_connectionId >= 0) {
    DisconnectFromCube();
  }
  
  // Signal the ev loop thread to break out of its loop
  _asyncBreakSignal.send();
  
  // Wait for the loop thread to complete
  if (_loopThread.joinable()) {
    _loopThread.join();
  }
}
  
bool BleClient::Send(const std::vector<uint8_t>& msg)
{
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.Send.NotConnectedToServer",
                        "Cannot send - not connected to the server");
    return false;
  }
  
  if(_connectionId < 0) {
    PRINT_NAMED_WARNING("BleClient.Send.NotConnectedToCube",
                        "Cannot send - not connected to any cube");
    return false;
  }

  const bool reliable = true;
  const std::string& uuid = kCubeAppWrite_128_BIT_UUID;
  SendMessage(_connectionId,
              uuid,
              reliable,
              msg);
  return true;
}


void BleClient::ConnectToCube(const std::string& address)
{
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.ConnectToCube.NotConnectedToServer",
                        "Cannot connect to cube - not connected to the server");
    return;
  }
  
  _cubeAddress = address;
  ConnectToPeripheral(_cubeAddress);
}


void BleClient::DisconnectFromCube()
{
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.DisconnectFromCube.NotConnectedToServer",
                        "Cannot disconnect from cube - not connected to the server");
    return;
  }
  
  if (_connectionId >= 0) {
    Disconnect(_connectionId);
  }
}


void BleClient::StartScanForCubes()
{
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.StartScanForCubes.NotConnectedToServer",
                        "Cannot start a scan - not connected to the server");
    return;
  }
  
  _asyncStartScanSignal.send();
}


void BleClient::StopScanForCubes()
{
  _scanningTimer.stop();
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.StopScanForCubes.NotConnectedToServer",
                        "Cannot stop scanning - not connected to the server");
    return;
  }
  
  StopScan();
  if (_scanFinishedCallback) {
    _scanFinishedCallback();
  }
}


void BleClient::FlashCube(std::vector<uint8_t> cubeFirmware)
{
  if (!IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.FlashCube.NotConnectedToServer",
                        "Cannot flash the cube - not connected to the server");
    return;
  }
  _cubeFirmware = std::move(cubeFirmware);
  _asyncFlashCubeSignal.send();
}


void BleClient::OnScanResults(int error, const std::vector<BluetoothDaemon::ScanResultRecord>& records)
{
  if (error != 0) {
    PRINT_NAMED_WARNING("BleClient.OnScanResults.Error",
                        "OnScanResults reporting error %d",
                        error);
    return;
  }
  
  if (_advertisementCallback) {
    for (const auto& r : records) {
      _advertisementCallback(r.address, r.rssi);
    }
  }
}

void BleClient::OnOutboundConnectionChange(const std::string& address,
                                           const int connected,
                                           const int connection_id,
                                           const std::vector<Anki::BluetoothDaemon::GattDbRecord>& records)
{
  PRINT_NAMED_INFO("BleClient.OnOutboundConnectionChange",
                   "addr %s, connected %d, connection_id %d, ",
                   address.c_str(), connected, connection_id);
  
  if (address != _cubeAddress) {
    return;
  }
  
  if (connected) {
    _connectionId = connection_id;
    ReadCharacteristic(_connectionId, Anki::kCubeAppVersion_128_BIT_UUID);
  } else if (_connectionId == connection_id) {
    _connectionId = -1;
    _cubeAddress.clear();
  }
}


void BleClient::OnCharacteristicReadResult(const int connection_id,
                                           const int error,
                                           const std::string& characteristic_uuid,
                                           const std::vector<uint8_t>& data) 
{
  if (connection_id == -1 || connection_id != _connectionId) {
    return;
  }

  if(error) {
    return;
  }

  if (Util::StringCaseInsensitiveEquals(characteristic_uuid, Anki::kCubeAppVersion_128_BIT_UUID)) {
    auto connectedFirmwareVersion = std::string(data.begin(), data.end());

    RequestConnectionParameterUpdate(_cubeAddress,
        kGattConnectionIntervalHighPriorityMinimum,
        kGattConnectionIntervalHighPriorityMaximum,
        kGattConnectionLatencyDefault,
        kGattConnectionTimeoutDefault);

    if(_receiveFirmwareVersionCallback) {
      _receiveFirmwareVersionCallback(_cubeAddress, connectedFirmwareVersion);
    }
  }
}
  
void BleClient::OnReceiveMessage(const int connection_id,
                                 const std::string& characteristic_uuid,
                                 const std::vector<uint8_t>& value)
{
  if (connection_id != _connectionId) {
    return;
  }
  
  if(Util::StringCaseInsensitiveEquals(characteristic_uuid, Anki::kCubeAppVersion_128_BIT_UUID)) {
    // this characteristic is returned after flashing a cube finishes
    std::string versionOnCube(value.begin(), value.end());
    size_t offset = 0x10; // the first 16 bytes of firmware are the version #
    std::string versionOnDisk(_cubeFirmware.begin(), _cubeFirmware.begin() + offset);
    if(versionOnCube.compare(versionOnDisk)==0) {
      PRINT_NAMED_INFO("BleClient.OnReceiveMessage.FlashingSuccess","%s",versionOnCube.c_str());
    } else {
      PRINT_NAMED_WARNING("BleClient.OnReceiveMessage.FlashingFailure","got = %s exp = %s",versionOnCube.c_str(), versionOnDisk.c_str());
    }
    _cubeFirmware.clear();
  } else if (Util::StringCaseInsensitiveEquals(characteristic_uuid, Anki::kCubeAppRead_128_BIT_UUID)) {
    if (_receiveDataCallback) {
      _receiveDataCallback(_cubeAddress, value);
    }
  }
}


void BleClient::AsyncBreakCallback(ev::async& w, int revents)
{
  // Break out of the loop
  ev_unloop(_loop, EVUNLOOP_ALL);
}


void BleClient::AsyncStartScanCallback(ev::async& w, int revents)
{
  // Commence scanning and start the timer
  StartScan(Anki::kCubeService_128_BIT_UUID);
  _scanningTimer.start(_scanDuration_sec);
}

void BleClient::AsyncFlashCubeCallback(ev::async& w, int revents)
{
  if(_cubeFirmware.empty()) {
    return;
  }
  size_t offset = 0x10; // first 16 bytes is firmware version


  while (offset < _cubeFirmware.size()) {
    std::vector<uint8_t> packet;
    size_t chunkLength = std::min(kMaxBytesPerPacket, (int) (_cubeFirmware.size() - offset));
    std::copy(_cubeFirmware.begin() + offset, 
              _cubeFirmware.begin() + offset + chunkLength, 
              std::back_inserter(packet));
    SendMessage(_connectionId, Anki::kCubeOTATarget_128_BIT_UUID, true, packet);
    offset += chunkLength;
  }
}

void BleClient::ConnectionCheckTimerCallback(ev::timer& timer, int revents)
{
  static bool wasConnected = false;
  if (wasConnected && !IsConnected()) {
    PRINT_NAMED_WARNING("BleClient.ConnectionCheckTimerCallback.DisconnectedFromServer",
                        "We've become disconnected from the BLE server - attempting to reconnect");
    // Server will kill our cube connection once we've become disconnected,
    // so reset connectionId
    _connectionId = -1;
    // Stop scanning for cubes timer
    _scanningTimer.stop();
    // Immediately attempt to re-connect
    if (!Connect()) {
      PRINT_NAMED_WARNING("BleClient.LoopThread.ConnectFailed",
                          "Unable to connect to ble server - will retry");
    }
  } else if (!wasConnected && IsConnected()) {
    PRINT_NAMED_INFO("BleClient.ConnectionCheckTimerCallback.ConnectedToServer",
                     "Connected to the BLE server!");
    if (_startScanUponConnection) {
      StartScanForCubes();
    }
  }
  
  // Fire up the timer again for the appropriate interval
  timer.start(IsConnected() ?
              kDisconnectionCheckTime_sec :
              kConnectionCheckTime_sec);
  
  wasConnected = IsConnected();
}


void BleClient::ScanningTimerCallback(ev::timer& w, int revents)
{
  StopScanForCubes();
}


} // Cozmo
} // Anki
