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

#include "anki-ble/ipc-client.h"
#include "anki-ble/log.h"
#include "anki-ble/memutils.h"
#include "libev/libev.h"

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
}

bool IPCClient::Connect()
{
  if (!connect_watcher_) {
    connect_watcher_ = new ev::io(loop_);
    connect_watcher_->set <IPCClient, &IPCClient::ConnectWatcherCallback> (this);
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
  strncpy(args->characteristic_uuid,
          characteristic_uuid.c_str(),
          sizeof(args->characteristic_uuid) - 1);
  args->reliable = reliable;
  args->length = (uint32_t) value.size();
  memcpy(args->value, value.data(), value.size());
  SendIPCMessageToServer(IPCMessageType::SendMessage,
                         args_length,
                         (uint8_t *) args);
  free(args);
}

void IPCClient::Disconnect(const int connection_id)
{
  DisconnectArgs args;
  args.connection_id = connection_id;
  SendIPCMessageToServer(IPCMessageType::Disconnect,
                         sizeof(args),
                         (uint8_t *) &args);
}

void IPCClient::StartAdvertising()
{
  SendIPCMessageToServer(IPCMessageType::StartAdvertising);
}

void IPCClient::StopAdvertising()
{
  SendIPCMessageToServer(IPCMessageType::StopAdvertising);
}

IPCClient::~IPCClient()
{
  delete connect_watcher_; connect_watcher_ = nullptr;
}

} // namespace BluetoothDaemon
} // namespace Anki