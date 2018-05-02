/**
 * File: ipc-client.cpp
 *
 * Author: seichert
 * Created: 2/5/2018
 *
 * Description: IPC Client for ankibluetoothd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "ipc-client.h"
#include "libev/libev.h"
#include "log.h"
#include "memutils.h"
#include "strlcpy.h"

#include <algorithm>

#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace Anki {
namespace BluetoothDaemon {

IPCClient::IPCClient(struct ev_loop* loop)
    : IPCEndpoint(loop)
    , connect_watcher_(nullptr)
{
#ifdef NDEBUG
  setMinLogLevel(kLogLevelInfo);
#else
  setMinLogLevel(kLogLevelDebug);
#endif
}

bool IPCClient::Connect()
{
  if (!connect_watcher_) {
    connect_watcher_ = new ev::io(loop_);
    connect_watcher_->set <IPCClient, &IPCClient::ConnectWatcherCallback> (this);
  }
  if (sockfd_ == -1) {
    CreateSocket();
  }
  int rc = connect(sockfd_, (struct sockaddr *)&addr_, sizeof(addr_));
  if (rc == -1) {
    if (errno == EALREADY || errno == EINPROGRESS) {
      connect_watcher_->start(sockfd_, ev::WRITE);
      return true;
    }
    loge("ipc-client: connect errno = %d (%s)", errno, strerror(errno));
    return false;
  }
  OnConnect();
  return true;
}

void IPCClient::ConnectWatcherCallback(ev::io& w, int revents)
{
  if (revents & ev::WRITE) {
    logv("ipc-client: ConnectWatcherCallback - ev::WRITE");
    int error = 0;
    socklen_t optlen = sizeof(error);
    int rc = getsockopt(w.fd, SOL_SOCKET, SO_ERROR, &error, &optlen);
    if (rc == -1) {
      loge("ipc-client: getsockopt errno = %d (%s)", errno, strerror(errno));
      return;
    }
    if (error == 0) {
      OnConnect();
    } else {
      logv("ipc-client: connect SO_ERROR = %d (%s)", error, strerror(errno));
    }
  }
}

void IPCClient::OnConnect()
{
  logi("ipc-client: connected to IPC server");
  delete connect_watcher_; connect_watcher_ = nullptr;
  AddPeerByFD(sockfd_);
}

void IPCClient::OnReceiveIPCMessage(const int sockfd,
                                    const IPCMessageType type,
                                    const std::vector<uint8_t>& data)
{
  switch (type) {
    case IPCMessageType::Ping:
      logv("ipc-client: Ping received");
      (void) SendIPCMessageToServer(IPCMessageType::OnPingResponse, 0, nullptr);
      break;
    case IPCMessageType::OnPingResponse:
      logv("ipc-client: Ping response received");
      break;
    case IPCMessageType::OnPeripheralStateUpdate:
      {
        logv("ipc-client: OnPeripheralStateUpdate");
        OnPeripheralStateUpdateArgs* args = (OnPeripheralStateUpdateArgs *) data.data();
        OnPeripheralStateUpdate(args->advertising,
                                args->connection_id,
                                args->connected,
                                args->congested);
      }
      break;
    case IPCMessageType::OnInboundConnectionChange:
      {
        logv("ipc-client: OnInboundConnectionChange");
        OnInboundConnectionChangeArgs* args = (OnInboundConnectionChangeArgs *) data.data();
        OnInboundConnectionChange(args->connection_id, args->connected);
      }
      break;
    case IPCMessageType::OnReceiveMessage:
      {
        logv("ipc-client: OnReceiveMessage");
        OnReceiveMessageArgs* args = (OnReceiveMessageArgs *) data.data();
        std::string characteristic_uuid(args->characteristic_uuid);
        std::vector<uint8_t> value(args->value, args->value + args->length);
        OnReceiveMessage(args->connection_id,
                         characteristic_uuid,
                         value);
      }
      break;
    case IPCMessageType::OnCharacteristicReadResult:
      {
        logv("ipc-case: OnCharacteristicReadResult");
        OnCharacteristicReadResultArgs* args = (OnCharacteristicReadResultArgs *) data.data();
        std::string characteristic_uuid(args->characteristic_uuid);
        std::vector<uint8_t> value(args->value, args->value + args->length);
        OnCharacteristicReadResult(args->connection_id,
                                   args->error,
                                   characteristic_uuid,
                                   value);
      }
      break;
    case IPCMessageType::OnDescriptorReadResult:
      {
        logv("ipc-case: OnDescriptorReadResult");
        OnDescriptorReadResultArgs* args = (OnDescriptorReadResultArgs *) data.data();
        std::string characteristic_uuid(args->characteristic_uuid);
        std::string descriptor_uuid(args->descriptor_uuid);
        std::vector<uint8_t> value(args->value, args->value + args->length);
        OnDescriptorReadResult(args->connection_id,
                               args->error,
                               characteristic_uuid,
                               descriptor_uuid,
                               value);
      }
      break;
    case IPCMessageType::OnScanResults:
      {
        logv("ipc-client: OnScanResults");
        OnScanResultsArgs* args = (OnScanResultsArgs *) data.data();
        std::vector<ScanResultRecord> records;
        for (int i = 0 ; i < args->record_count; i++) {
          ScanResultRecord record;
          memcpy(&record, &(args->records[i]), sizeof(record));
          records.push_back(record);
        }
        OnScanResults(args->error, records);
      }
      break;
    case IPCMessageType::OnOutboundConnectionChange:
      {
        logv("ipc-client: OnOutboundConnectionChange");
        OnOutboundConnectionChangeArgs* args = (OnOutboundConnectionChangeArgs *) data.data();
        std::vector<GattDbRecord> records;
        for (int i = 0 ; i < args->num_gatt_db_records ; i++) {
          GattDbRecord record = {{0}};
          memcpy(&record, &(args->records[i]), sizeof(record));
          records.push_back(record);
        }
        OnOutboundConnectionChange(std::string(args->address),
                                   args->connected,
                                   args->connection_id,
                                   records);
      }
      break;
    case IPCMessageType::OnRequestConnectionParameterUpdateResult:
      {
        logv("ipc-client: OnRequestConnectionParameterUpdateResult");
        OnRequestConnectionParameterUpdateResultArgs* args =
            (OnRequestConnectionParameterUpdateResultArgs *) data.data();
        OnRequestConnectionParameterUpdateResult(std::string(args->address),
                                                 args->status);
      }
      break;
    default:
      loge("ipc-client: Unknown IPC message : %d", (int) type);
      break;
  }
}

void IPCClient::SendMessage(const int connection_id,
                            const std::string& characteristic_uuid,
                            const bool reliable,
                            const std::vector<uint8_t>& value)
{
  SendMessageArgs* args;
  uint32_t args_length = sizeof(*args) + value.size();
  args = (SendMessageArgs *) malloc_zero(args_length);
  args->connection_id = connection_id;
  (void) strlcpy(args->characteristic_uuid,
                 characteristic_uuid.c_str(),
                 sizeof(args->characteristic_uuid));
  args->reliable = reliable;
  args->length = (uint32_t) value.size();
  memcpy(args->value, value.data(), value.size());
  SendIPCMessageToServer(IPCMessageType::SendMessage,
                         args_length,
                         (uint8_t *) args);
  free(args);
}

void IPCClient::ReadCharacteristic(const int connection_id,
                                   const std::string& characteristic_uuid)
{
  CharacteristicReadRequestArgs args;
  args.connection_id = connection_id;
  (void) strlcpy(args.characteristic_uuid,
                 characteristic_uuid.c_str(),
                 sizeof(args.characteristic_uuid));
  SendIPCMessageToServer(IPCMessageType::CharacteristicReadRequest,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::ReadDescriptor(const int connection_id,
                               const std::string& characteristic_uuid,
                               const std::string& descriptor_uuid)
{
  DescriptorReadRequestArgs args;
  args.connection_id = connection_id;
  (void) strlcpy(args.characteristic_uuid,
                 characteristic_uuid.c_str(),
                 sizeof(args.characteristic_uuid));
  (void) strlcpy(args.descriptor_uuid,
                 descriptor_uuid.c_str(),
                 sizeof(args.descriptor_uuid));
  SendIPCMessageToServer(IPCMessageType::DescriptorReadRequest,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::Disconnect(const int connection_id)
{
  DisconnectArgs args;
  args.connection_id = connection_id;
  SendIPCMessageToServer(IPCMessageType::Disconnect,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::StartAdvertising(const BLEAdvertiseSettings& settings)
{
  StartAdvertisingArgs args = {0};
  args.appearance = settings.GetAppearance();

  const std::vector<const BLEAdvertiseData*>
      ble_ad_data = {&(settings.GetAdvertisement()), &(settings.GetScanResponse())};
  std::vector<AdvertisingData*> ad_data = {&(args.advertisement), &(args.scan_response)};

  for (int i = 0 ; i < ble_ad_data.size(); i++) {
    const BLEAdvertiseData* src = ble_ad_data[i];
    AdvertisingData* dst = ad_data[i];

    dst->include_device_name = src->GetIncludeDeviceName();
    dst->include_tx_power_level = src->GetIncludeTxPowerLevel();
    const std::vector<uint8_t>& manufacturer_data = src->GetManufacturerData();
    dst->manufacturer_data_len =
        std::min(manufacturer_data.size(), sizeof(dst->manufacturer_data));
    if (dst->manufacturer_data_len > 0) {
      if (manufacturer_data.size() > dst->manufacturer_data_len) {
        logw("Truncating manufacturer_data from %zu bytes to %d bytes",
             manufacturer_data.size(), dst->manufacturer_data_len);
      }
      (void) memcpy(dst->manufacturer_data,
                    manufacturer_data.data(),
                    dst->manufacturer_data_len);
    }
    const std::vector<uint8_t>& service_data = src->GetServiceData();
    dst->service_data_len =
        std::min(service_data.size(), sizeof(dst->service_data));
    if (dst->service_data_len > 0) {
      if (service_data.size() > dst->service_data_len) {
        logw("Truncating service_data from %zu bytes to %d bytes",
             service_data.size(), dst->service_data_len);
      }
      (void) memcpy(dst->service_data,
                    service_data.data(),
                    dst->service_data_len);
    }
    dst->have_service_uuid = !src->GetServiceUUID().empty();
    if (dst->have_service_uuid) {
      strlcpy(dst->service_uuid, src->GetServiceUUID().c_str(), sizeof(dst->service_uuid));
    }
    dst->min_interval = src->GetMinInterval();
    dst->max_interval = src->GetMaxInterval();
  }

  SendIPCMessageToServer(IPCMessageType::StartAdvertising,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::StopAdvertising()
{
  SendIPCMessageToServer(IPCMessageType::StopAdvertising);
}

void IPCClient::StartScan(const std::string& serviceUUID)
{
  StartScanArgs args = {{0}};
  (void) strlcpy(args.service_uuid, serviceUUID.c_str(), sizeof(args.service_uuid));
  SendIPCMessageToServer(IPCMessageType::StartScan,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::ConnectToPeripheral(const std::string& address)
{
  ConnectToPeripheralArgs args = {{0}};
  (void) strlcpy(args.address, address.c_str(), sizeof(args.address));
  SendIPCMessageToServer(IPCMessageType::ConnectToPeripheral,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::RequestConnectionParameterUpdate(const std::string& address,
                                                 int min_interval,
                                                 int max_interval,
                                                 int latency,
                                                 int timeout)
{
  RequestConnectionParameterUpdateArgs args = {{0}};
  (void) strlcpy(args.address, address.c_str(), sizeof(args.address));
  args.min_interval = min_interval;
  args.max_interval = max_interval;
  args.latency = latency;
  args.timeout = timeout;
  SendIPCMessageToServer(IPCMessageType::RequestConnectionParameterUpdate,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::SetAdapterName(const std::string& name)
{
  SetAdapterNameArgs args = {{0}};
  (void) strlcpy(args.name, name.c_str(), sizeof(args.name));
  SendIPCMessageToServer(IPCMessageType::SetAdapterName,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::StopScan()
{
  SendIPCMessageToServer(IPCMessageType::StopScan);
}

IPCClient::~IPCClient()
{
  delete connect_watcher_; connect_watcher_ = nullptr;
}

} // namespace BluetoothDaemon
} // namespace Anki

