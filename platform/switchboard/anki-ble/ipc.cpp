/**
 * File: ipc.cpp
 *
 * Author: seichert
 * Created: 2/6/2018
 *
 * Description: Common IPC code for ankibluetoothd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "ipc.h"
#include "anki_ble_uuids.h"
#include "gatt_constants.h"
#include "libev/libev.h"
#include "log.h"
#include "stringutils.h"
#include "strlcpy.h"

#include <unistd.h>

#include <algorithm>
#include <cctype>
namespace Anki {
namespace BluetoothDaemon {

IPCEndpoint::IPCEndpoint(struct ev_loop* loop)
  : loop_(loop)
{
  sockfd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
  addr_ = (const struct sockaddr_un) { .sun_family = AF_UNIX };
  (void) strlcpy(addr_.sun_path, kSocketName.c_str(), sizeof(addr_.sun_path));
  task_executor_ = new TaskExecutor(loop);
}

IPCEndpoint::~IPCEndpoint()
{
  delete task_executor_; task_executor_ = nullptr;
  for (auto& pair : peers_) {
    delete pair.second;
  }
  CloseSocket();
}

void IPCEndpoint::CloseSocket()
{
  peers_.erase(sockfd_);
  (void) close(sockfd_); sockfd_ = -1;
}

void IPCEndpoint::ReceiveMessage(PeerState* p)
{
  ssize_t bytesRead;
  do {
    uint8_t buf[kIPCMessageMaxSize];
    bytesRead = recv(p->GetFD(), buf, sizeof(buf), 0);
    if (bytesRead > 0) {
      std::vector<uint8_t>& incoming_data = p->GetIncomingDataVector();
      std::copy(buf, buf + bytesRead, std::back_inserter(incoming_data));
      IPCMessage* message;

      while (incoming_data.size() >= sizeof(*message)) {
        message = (IPCMessage *) incoming_data.data();

        if (0 != memcmp(message->magic, kIPCMessageMagic, sizeof(message->magic))) {
          loge("ipc-endpoint: recv'd bad magic '%c%c%c%c' (%02X %02X %02X %02X)"
               ", expected '%c%c%c%c'",
               std::isprint(message->magic[0]) ? message->magic[0] : '?',
               std::isprint(message->magic[1]) ? message->magic[1] : '?',
               std::isprint(message->magic[2]) ? message->magic[2] : '?',
               std::isprint(message->magic[3]) ? message->magic[3] : '?',
               message->magic[0], message->magic[1], message->magic[2], message->magic[3],
               kIPCMessageMagic[0], kIPCMessageMagic[1], kIPCMessageMagic[2], kIPCMessageMagic[3]);

          OnReceiveError(p->GetFD());
          return;
        }
        if (message->version != kIPCMessageVersion) {
          loge("ipc-endpoint: recv'd version %u, expected %u",
               message->version, kIPCMessageVersion);
          OnReceiveError(p->GetFD());
          return;
        }
        if (message->length > kIPCMessageMaxLength) {
          loge("ipc-endpoint: recv'd bad length (%u). max is %zu",
               message->length, kIPCMessageMaxLength);
          OnReceiveError(p->GetFD());
          return;
        }
        size_t totalLength = sizeof(*message) + message->length;
        if (incoming_data.size() >= totalLength) {
          std::vector<uint8_t> data;
          if (message->length > 0) {
            std::copy(incoming_data.begin() + sizeof(*message),
                      incoming_data.begin() + totalLength,
                      std::back_inserter(data));
          }
          OnReceiveIPCMessage(p->GetFD(), message->type, data);
          incoming_data.erase(incoming_data.begin(),
                              incoming_data.begin() + totalLength);
        } else {
          break;
        }
      }
    }
  } while (bytesRead > 0);

  if (bytesRead == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    loge("ipc-endpoint: recv. p->GetFD() = %d, errno = %d (%s)",
         p->GetFD(), errno, strerror(errno));
    OnReceiveError(p->GetFD());
    return;
  }

  if (bytesRead == 0) {
    // Other side closed the socket in an orderly manner
    OnPeerClose(p->GetFD());
    return;
  }
}

void IPCEndpoint::AddPeerByFD(const int fd)
{
  ev::io* read_write_watcher = new ev::io(loop_);
  read_write_watcher->set <IPCEndpoint, &IPCEndpoint::ReadWriteWatcherCallback> (this);
  read_write_watcher->start(fd, ev::READ);
  peers_[fd] = new PeerState(read_write_watcher, task_executor_);
}

bool IPCEndpoint::SendMessageToAllPeers(const IPCMessageType type,
                                        uint32_t length,
                                        uint8_t* val)
{
  for (auto const& p : peers_) {
    bool result = SendMessageToPeer(p.first, type, length, val);
    if (!result) {
      return result;
    }
  }
  return true;
}

bool IPCEndpoint::SendMessageToPeer(const int fd,
                                    const IPCMessageType type,
                                    uint32_t length,
                                    uint8_t* val)
{
  auto search = peers_.find(fd);
  if (search == peers_.end()) {
    return false;
  }
  if (length > kIPCMessageMaxLength) {
    return false;
  }
  IPCMessage message;
  memcpy(message.magic, kIPCMessageMagic, sizeof(message.magic));
  message.version = kIPCMessageVersion;
  message.type = type;
  message.length = length;

  uint8_t* raw_pointer = (uint8_t *) &message;
  std::vector<uint8_t> packed_message(raw_pointer, raw_pointer + sizeof(message));
  if (length > 0) {
    std::vector<uint8_t> data(val, val + length);
    std::copy(data.begin(), data.end(), std::back_inserter(packed_message));
  }

  search->second->AddMessageToQueue(packed_message);
  return true;
}
void IPCEndpoint::ReadWriteWatcherCallback(ev::io& w, int revents)
{
  if (revents & ev::ERROR) {
    loge("ipc-endpoint: RWCB error");
    return;
  }

  if (revents & ev::WRITE) {
    SendQueuedMessagesToPeer(w.fd);
  }

  if (revents & ev::READ) {
    auto search = peers_.find(w.fd);
    if (search == peers_.end()) {
      return;
    }
    ReceiveMessage(search->second);
  }

}

void IPCEndpoint::SendQueuedMessagesToPeer(const int sockfd)
{
  auto search = peers_.find(sockfd);
  if (search == peers_.end()) {
    return;
  }
  while (!search->second->IsQueueEmpty()) {
    const std::vector<uint8_t>& packed_message = search->second->GetMessageAtFrontOfQueue();
    ssize_t bytesSent = send(sockfd, packed_message.data(), packed_message.size(), 0);
    if (-1 == bytesSent) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        logw("ipc-endpoint: send would block");
        break;
      }
      OnSendError(sockfd, errno);
      break;
    } else if (bytesSent < packed_message.size()) {
      loge("ipc-endpoint: send sent %zd bytes, expecting %zu bytes", bytesSent, packed_message.size());
      OnSendError(sockfd, EIO);
      break;
    } else {
      search->second->EraseMessageFromFrontOfQueue();
    }
  }
}

void IPCEndpoint::OnReceiveError(const int sockfd)
{
  peers_.erase(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

void IPCEndpoint::OnPeerClose(const int sockfd)
{
  peers_.erase(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

void IPCEndpoint::OnSendError(const int sockfd, const int error)
{
  peers_.erase(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

IPCEndpoint::PeerState::~PeerState()
{
  delete read_write_watcher_; read_write_watcher_ = nullptr;
}

int IPCEndpoint::PeerState::GetFD() const
{
  return read_write_watcher_->fd;
}

bool IPCEndpoint::PeerState::IsQueueEmpty()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return outgoing_queue_.empty();
}

void IPCEndpoint::PeerState::AddMessageToQueue(const std::vector<uint8_t>& message)
{
  std::lock_guard<std::mutex> lock(mutex_);
  outgoing_queue_.push_back(message);
  task_executor_->Wake([this]() {
      read_write_watcher_->start(read_write_watcher_->fd, ev::READ | ev::WRITE);
    });
}

std::vector<uint8_t> IPCEndpoint::PeerState::GetMessageAtFrontOfQueue()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return outgoing_queue_.front();
}

void IPCEndpoint::PeerState::EraseMessageFromFrontOfQueue()
{
  std::lock_guard<std::mutex> lock(mutex_);
  outgoing_queue_.pop_front();
  if (outgoing_queue_.empty()) {
    task_executor_->Wake([this]() {
        read_write_watcher_->set(read_write_watcher_->fd, ev::READ);
      });
  }
}

ScanResultRecord::ScanResultRecord(const std::string& address,
                                   const int rssi,
                                   const std::vector<uint8_t>& adv_data)
{
  (void) strlcpy(this->address, address.c_str(), sizeof(this->address));
  this->rssi = rssi;

  auto it = adv_data.begin();
  while (it != adv_data.end()) {
    uint8_t length = *it;
    if (length == 0) {
      break;
    }
    it++;
    uint8_t type = *it;
    if (length > 1) {
      if (std::distance(adv_data.begin(), it) + length > adv_data.size()) {
        break;
      }
      std::vector<uint8_t> data(it + 1, it + length);
      switch(type) {
        case kADTypeFlags:
          // ignore for now
          break;
        case kADTypeCompleteListOf16bitServiceClassUUIDs:
          {
            size_t sz = data.size();
            size_t offset = 0;
            while ((num_service_uuids < 4) && ((offset + 2) <= sz)) {
              std::vector<uint8_t> v(data.begin() + offset, data.begin() + offset + 2);
              std::reverse(std::begin(v), std::end(v));
              std::string short_uuid = byteVectorToHexString(v);
              std::string full_uuid = kBluetoothBase_128_BIT_UUID;
              full_uuid.replace(4, 4, short_uuid);
              (void) strlcpy(service_uuids[num_service_uuids],
                             full_uuid.c_str(),
                             sizeof(service_uuids[num_service_uuids]));
              num_service_uuids++;
              offset += 2;
            }
          }
          break;
        case kADTypeCompleteListOf128bitServiceClassUUIDs:
          {
            size_t sz = data.size();
            size_t offset = 0;
            while ((num_service_uuids < 4) && ((offset + 16) <= sz)) {
              std::vector<uint8_t> v(data.begin() + offset, data.begin() + offset + 16);
              std::reverse(std::begin(v), std::end(v));
              auto it = v.begin();
              std::vector<int> lengths = {4,2,2,2,6};
              std::string uuidString;
              for (auto l : lengths) {
                if (it != v.begin()) {
                  uuidString.push_back('-');
                }
                std::vector<uint8_t> part(it, it + l);
                uuidString += byteVectorToHexString(part);
                it += l;
              }
              (void) strlcpy(service_uuids[num_service_uuids],
                             uuidString.c_str(),
                             sizeof(service_uuids[num_service_uuids]));
              num_service_uuids++;
              offset += 16;
            }
          }
          break;
        case kADTypeCompleteLocalName:
          {
            std::string complete_local_name(data.begin(), data.end());
            (void) strlcpy(local_name, complete_local_name.c_str(), sizeof(local_name));
          }
          break;
        case kADTypeManufacturerSpecificData:
          {
            manufacturer_data_len = length - 1;
            memcpy(manufacturer_data,
                   data.data(),
                   std::min(sizeof(manufacturer_data), (size_t) length - 1));
          }
          break;
        default:
          break;
      }
    }
    it += length;
  }
  advertisement_length = adv_data.size();
  memcpy(advertisement_data,
         adv_data.data(),
         std::min(sizeof(advertisement_data), (size_t) advertisement_length));

}

bool ScanResultRecord::HasServiceUUID(const std::string& uuid)
{
  for (int i = 0 ; i < num_service_uuids; i++) {
    if (AreCaseInsensitiveStringsEqual(std::string(service_uuids[i]), uuid)) {
      return true;
    }
  }
  return false;
}

} // namespace BluetoothDaemon
} // namespace Anki