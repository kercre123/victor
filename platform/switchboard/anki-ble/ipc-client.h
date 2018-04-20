/*
 * File: ipc-client.h
 *
 * Author: seichert
 * Created: 2/5/2018
 *
 * Description: IPC Client for ankibluetoothd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include "ipc.h"
#include "ble_advertise_settings.h"

#include <deque>
#include <mutex>
#include <vector>

namespace Anki {
namespace BluetoothDaemon {

class IPCClient : public IPCEndpoint {
 public:
  IPCClient(struct ev_loop* loop);
  ~IPCClient();
  bool Connect();

  bool SendIPCMessageToServer(const IPCMessageType type,
                              uint32_t length = 0,
                              uint8_t* val = nullptr) {
    return SendMessageToPeer(type, length, val);
  }
  bool IsConnected() { return !peers_.empty(); }
  void SendMessage(const int connection_id,
                   const std::string& characteristic_uuid,
                   const bool reliable,
                   const std::vector<uint8_t>& value);
  void ReadCharacteristic(const int connection_id,
                          const std::string& characteristic_uuid);
  void ReadDescriptor(const int connection_id,
                      const std::string& characteristic_uuid,
                      const std::string& descriptor_uuid);
  void Disconnect(const int connection_id);
  void StartAdvertising(const BLEAdvertiseSettings& settings);
  void StopAdvertising();
  void StartScan(const std::string& serviceUUID);
  void StopScan();
  void ConnectToPeripheral(const std::string& address);
  void RequestConnectionParameterUpdate(const std::string& address,
                                        int min_interval,
                                        int max_interval,
                                        int latency,
                                        int timeout);

 protected:
  virtual void OnReceiveIPCMessage(const int sockfd,
                                   const IPCMessageType type,
                                   const std::vector<uint8_t>& data);
  virtual void OnInboundConnectionChange(int connection_id, int connected) { }
  virtual void OnReceiveMessage(const int connection_id,
                                const std::string& characteristic_uuid,
                                const std::vector<uint8_t>& value) {}
  virtual void OnPeripheralStateUpdate(const bool advertising,
                                       const int connection_id,
                                       const int connected,
                                       const bool congested) {}
  virtual void OnScanResults(int error, const std::vector<ScanResultRecord>& records) {}
  virtual void OnOutboundConnectionChange(const std::string& address,
                                          const int connected,
                                          const int connection_id,
                                          const std::vector<GattDbRecord>& records) {}
  virtual void OnCharacteristicReadResult(const int connection_id,
                                          const int error,
                                          const std::string& characteristic_uuid,
                                          const std::vector<uint8_t>& data) {}
  virtual void OnDescriptorReadResult(const int connection_id,
                                      const int error,
                                      const std::string& characteristic_uuid,
                                      const std::string& descriptor_uuid,
                                      const std::vector<uint8_t>& data) {}
  virtual void OnRequestConnectionParameterUpdateResult(const std::string& address,
                                                        const int status) {}

 private:
  void ConnectWatcherCallback(ev::io& w, int revents);
  void OnConnect();

  ev::io* connect_watcher_;
};

} // namespace BluetoothDaemon
} // namespace Anki