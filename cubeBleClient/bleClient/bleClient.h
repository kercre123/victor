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
#ifndef __CubeBleClient_BleClient_H__
#define  __CubeBleClient_BleClient_H__

#include "anki-ble/common/ipc-client.h"

#include <string>
#include <thread>

namespace Anki {
namespace Cozmo {
  class BleClient : public Anki::BluetoothDaemon::IPCClient {
  public:
    // Constructor
    BleClient(struct ev_loop* loop);
    
    ~BleClient() = default;
    
    // Attempts to connect to the server and starts the ev loop thread
    void Start();

    // Thread-safe way to terminate the client and ev loop.
    // BleClient can safely be destroyed once this returns.
    void Stop();
    
    // Send a message to the currently connected cube
    bool Send(const std::vector<uint8_t>& msg);
    
    // Attempt to connect to the cube with the given address
    void ConnectToCube(const std::string& address);
    
    void DisconnectFromCube();
    
    bool IsConnectedToCube() {
      return _connectionId >= 0;
    };
    
    // Are we connected to the bluetooth daemon?
    bool IsConnectedToServer() {
      return IsConnected();
    }
    
    // Start/stop scanning for cubes
    void StartScanForCubes();
    void StopScanForCubes();
    void SetScanDuration(const float duration_sec) {
      _scanDuration_sec = static_cast<ev_tstamp>(duration_sec);
    }

    void FlashCube(std::vector<uint8_t> cubeFirmware);
    
    using AdvertisementCallback = std::function<void(const std::string& addr, const int rssi)>;
    using ReceiveDataCallback = std::function<void(const std::string& addr, const std::vector<uint8_t>& data)>;
    using ScanFinishedCallback = std::function<void(void)>;
    using ReceiveFirmwareVersionCallback = std::function<void(const std::string& addr, const std::string& version)>; 
    
    // Advertisement callback gets called whenever we have new scanning results
    void RegisterAdvertisementCallback(const AdvertisementCallback& callback) {
      _advertisementCallback = callback;
    };
    
    // Receive data callback gets called whenever we receive a message from the connected cube.
    // Messages from other cubes are ignored (and should not happen normally).
    void RegisterReceiveDataCallback(const ReceiveDataCallback& callback) {
      _receiveDataCallback = callback;
    };
    
    // Gets called when scanning for cubes has completed
    void RegisterScanFinishedCallback(const ScanFinishedCallback& callback) {
      _scanFinishedCallback = callback;
    }

    void RegisterReceiveFirmwareVersionCallback(const ReceiveFirmwareVersionCallback& callback) {
      _receiveFirmwareVersionCallback = callback;
    }
    
  protected:
    virtual void OnScanResults(int error,
                               const std::vector<BluetoothDaemon::ScanResultRecord>& records) override;

    virtual void OnOutboundConnectionChange(const std::string& address,
                                            const int connected,
                                            const int connection_id,
                                            const std::vector<BluetoothDaemon::GattDbRecord>& records) override;

    virtual void OnReceiveMessage(const int connection_id,
                                  const std::string& characteristic_uuid,
                                  const std::vector<uint8_t>& value) override;

    // will flash the cube if the versions do not match
    virtual void OnCharacteristicReadResult(const int connection_id,
                                            const int error,
                                            const std::string& characteristic_uuid,
                                            const std::vector<uint8_t>& data) override;

  private:

    // Callback for the async event that terminates the ev loop
    void AsyncBreakCallback(ev::async& w, int revents);
    
    // Callback for the async event signalling the thread to begin
    // scanning for cubes (and start the scanning timer)
    void AsyncStartScanCallback(ev::async& w, int revents);
    
    // Callback for timer to occasionally check for connection to server
    void ServerConnectionCheckTimerCallback(ev::timer& w, int revents);
    
    // Callback for "scanning for cubes" timer
    void ScanningTimerCallback(ev::timer& w, int revents);

    // Callback for flashing the cube
    void AsyncFlashCubeCallback(ev::async& w, int revents);
    
    // Connection id for the single cube we are connected to.
    // Equal to -1 if we are not connected to a cube.
    int _connectionId = -1;
    
    // Address of the cube to connect to (can only connect to one at a time)
    std::string _cubeAddress;
    
    AdvertisementCallback          _advertisementCallback;
    ReceiveDataCallback            _receiveDataCallback;
    ScanFinishedCallback           _scanFinishedCallback;
    ReceiveFirmwareVersionCallback _receiveFirmwareVersionCallback;
    
    // The thread that runs the ev loop for server comms callbacks
    std::thread _loopThread;
    
    // Pointer to our ev loop
    struct ev_loop* _loop = nullptr;
    
    // Async signal used to terminate the ev_loop from any thread
    ev::async _asyncBreakSignal;
    
    // Async signal to begin scanning for cubes and start the scanning timer
    ev::async _asyncStartScanSignal;
    
    // Timer used to occasionally check connection to the server
    ev::timer _connectionCheckTimer;
    
    // Timer used to terminate scanning for cubes
    ev::timer _scanningTimer;

    // Async signal to begin flashing the cube
    ev::async _asyncFlashCubeSignal;
    
    // How long to scan for cubes for
    std::atomic<ev_tstamp> _scanDuration_sec{3.f};

    // path to cube firmware
    std::vector<uint8_t> _cubeFirmware;
  };

} // Cozmo
} // Anki

#endif // __CubeBleClient_BleClient_H__
