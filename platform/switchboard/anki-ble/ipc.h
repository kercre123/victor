/**
 * File: ipc.h
 *
 * Author: seichert
 * Created: 2/5/2018
 *
 * Description: IPC for ankibluetoothd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include "taskExecutor.h"

#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

// Forward declarations for libev
struct ev_loop;
namespace ev {
struct io;
struct timer;
} // namespace ev

namespace Anki {
namespace BluetoothDaemon {
const std::string kSocketName("/data/misc/bluetooth/abtd.socket");
const char kIPCMessageMagic[4] = {'i', 'p', 'c', 'f'};
const uint32_t kIPCMessageVersion = 1;
const size_t k128BitUUIDSize = 37;
const size_t k16BitUUIDSize = 5;
const size_t kAddressSize = 18;
const size_t kLocalNameSize = 32;
const size_t kManufacturerDataMaxSize = 32;
const size_t kServiceDataMaxSize = 32;
const size_t kMaxAdvertisingLength = 62;
const size_t kIPCMessageMaxSize = 4096;
const size_t kIPCMessageMaxLength = kIPCMessageMaxSize - 12;

enum class IPCMessageType {
  Invalid = 0,
    Ping,
    OnPingResponse,
    SendMessage,
    OnInboundConnectionChange,
    OnReceiveMessage,
    Disconnect,
    StartAdvertising,
    StopAdvertising,
    OnPeripheralStateUpdate,
    StartScan,
    StopScan,
    OnScanResults,
    ConnectToPeripheral,
    OnOutboundConnectionChange,
    CharacteristicReadRequest,
    OnCharacteristicReadResult,
    DescriptorReadRequest,
    OnDescriptorReadResult,
    RequestConnectionParameterUpdate,
    OnRequestConnectionParameterUpdateResult
};

typedef struct __attribute__ ((__packed__)) IPCMessage {
  char magic[4];
  uint32_t version;
  IPCMessageType type;
  uint32_t length;
} IPCMessage;

typedef struct __attribute__ ((__packed__)) SendMessageArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
  bool reliable;
  uint32_t length;
  uint8_t value[];
} SendMessageArgs;

typedef struct __attribute__ ((__packed__)) CharacteristicReadRequestArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
} CharacteristicReadRequestArgs;

typedef struct __attribute__ ((__packed__)) DescriptorReadRequestArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
  char descriptor_uuid[k128BitUUIDSize];
} DescriptorReadRequestArgs;

typedef struct __attribute__ ((__packed__)) OnInboundConnectionChangeArgs {
  int connection_id;
  int connected;
} OnInboundConnectionChangeArgs;

typedef struct __attribute__ ((__packed__)) OnReceiveMessageArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
  uint32_t length;
  uint8_t value[];
} OnReceiveMessageArgs;

typedef struct __attribute__ ((__packed__)) OnCharacteristicReadResultArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
  int error;
  uint32_t length;
  uint8_t value[];
} OnCharacteristicReadResultArgs;

typedef struct __attribute__ ((__packed__)) OnDescriptorReadResultArgs {
  int connection_id;
  char characteristic_uuid[k128BitUUIDSize];
  char descriptor_uuid[k128BitUUIDSize];
  int error;
  uint32_t length;
  uint8_t value[];
} OnDescriptorReadResultArgs;

typedef struct __attribute__ ((__packed__)) DisconnectArgs {
  int connection_id;
} DisconnectArgs;

typedef struct __attribute__ ((__packed__)) AdvertisingData {
  bool include_device_name;
  bool include_tx_power_level;
  int manufacturer_data_len;
  uint8_t manufacturer_data[kManufacturerDataMaxSize];
  int service_data_len;
  uint8_t service_data[kServiceDataMaxSize];
  bool have_service_uuid;
  char service_uuid[k128BitUUIDSize];
  int min_interval;
  int max_interval;
} AdvertisingData;

typedef struct __attribute__ ((__packed__)) StartAdvertisingArgs {
  int appearance;
  AdvertisingData advertisement;
  AdvertisingData scan_response;
} StartAdvertisingArgs;

typedef struct __attribute__ ((__packed__)) OnPeripheralStateUpdateArgs {
  bool advertising;
  int connection_id;
  int connected;
  bool congested;
} OnPeripheralStateUpdateArgs;

typedef struct __attribute__ ((__packed__)) StartScanArgs {
  char service_uuid[k128BitUUIDSize];
} StartScanArgs;

typedef struct __attribute__ ((__packed__)) ScanResultRecord {
  char address[kAddressSize] = {0};
  int rssi = 0;
  int num_service_uuids = 0;
  char service_uuids[4][k128BitUUIDSize] = {{0}};
  char local_name[kLocalNameSize] = {0};
  int manufacturer_data_len = 0;
  uint8_t manufacturer_data[kManufacturerDataMaxSize] = {0};
  int advertisement_length = 0;
  uint8_t advertisement_data[kMaxAdvertisingLength] = {0};
  ScanResultRecord() {}
  ScanResultRecord(const std::string& address,
                   const int rssi,
                   const std::vector<uint8_t>& adv_data);
  bool HasServiceUUID(const std::string& uuid);
} ScanResultRecord;

typedef struct __attribute__ ((__packed__)) OnScanResultsArgs {
  int error;
  int record_count;
  ScanResultRecord records[];
} OnScanResultsArgs;

typedef struct __attribute__ ((__packed__)) ConnectToPeripheralArgs {
  char address[kAddressSize];
} ConnectToPeripheralArgs;


enum class GattDbRecordType {
  Service,
  Characteristic,
  Descriptor
};

typedef struct __attribute__ ((__packed__)) GattDbRecord {
  char uuid[k128BitUUIDSize];
  GattDbRecordType type;
  uint16_t handle;
  uint16_t start_handle;
  uint16_t end_handle;
  int properties;
} GattDbRecord;

typedef struct __attribute__ ((__packed__)) OnOutboundConnectionChangeArgs {
  char address[kAddressSize];
  int connected;
  int connection_id;
  int num_gatt_db_records;
  GattDbRecord records[];
} OnOutboundConnectionChangeArgs;

typedef struct __attribute__ ((__packed__)) RequestConnectionParameterUpdateArgs {
  char address[kAddressSize];
  int min_interval;
  int max_interval;
  int latency;
  int timeout;
} RequestConnectionParameterUpdateArgs;

typedef struct __attribute__ ((__packed__)) OnRequestConnectionParameterUpdateResultArgs {
  char address[kAddressSize];
  int status;
} OnRequestConnectionParameterUpdateResultArgs;

class IPCEndpoint {
 public:
  IPCEndpoint(struct ev_loop* loop);
  ~IPCEndpoint();
  bool IsSocketValid() const { return (sockfd_ != -1); }
  bool SendMessageToAllPeers(const IPCMessageType type,
                             uint32_t length,
                             uint8_t* val);
  bool SendMessageToPeer(const IPCMessageType type,
                         uint32_t length,
                         uint8_t* val)
  {
    return SendMessageToPeer(sockfd_, type, length, val);
  }
  bool SendMessageToPeer(const int fd,
                         const IPCMessageType type,
                         uint32_t length,
                         uint8_t* val);

 protected:
  class PeerState {
   public:
    PeerState(ev::io* read_write_watcher, TaskExecutor* task_executor)
        : read_write_watcher_(read_write_watcher)
        , task_executor_(task_executor) { }
    ~PeerState();
    int GetFD() const;
    bool IsQueueEmpty();
    void AddMessageToQueue(const std::vector<uint8_t>& message);
    std::vector<uint8_t> GetMessageAtFrontOfQueue();
    void EraseMessageFromFrontOfQueue();
    std::vector<uint8_t>& GetIncomingDataVector() { return incoming_data_; }
   private:
    std::mutex mutex_;
    ev::io* read_write_watcher_;
    std::deque<std::vector<uint8_t>> outgoing_queue_;
    std::vector<uint8_t> incoming_data_;
    TaskExecutor* task_executor_;
  };
  void AddPeerByFD(const int fd);
  void CreateSocket();
  void CloseSocket();
  void ReceiveMessage(PeerState* p);
  void SendQueuedMessagesToPeer(const int sockfd);
  virtual void OnReceiveError(const int sockfd);
  virtual void OnPeerClose(const int sockfd);
  virtual void OnReceiveIPCMessage(const int sockfd,
                                   const IPCMessageType type,
                                   const std::vector<uint8_t>& data) {}
  virtual void OnSendError(const int sockfd, const int error);
  void ReadWriteWatcherCallback(ev::io& w, int revents);

  struct ev_loop* loop_;
  TaskExecutor* task_executor_;
  int sockfd_;
  struct sockaddr_un addr_;
  std::map<int,PeerState*> peers_;
};

} // namespace BluetoothDaemon
} // namespace Anki