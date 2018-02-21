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

#include "anki-ble/ipc.h"
#include "anki-ble/log.h"
#include "libev/libev.h"

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
  strncpy(addr_.sun_path, kSocketName.c_str(), sizeof(addr_.sun_path) - 1);
  task_executor_ = new TaskExecutor(loop);
}

IPCEndpoint::~IPCEndpoint()
{
  delete task_executor_; task_executor_ = nullptr;
  CloseSocket();
}

void IPCEndpoint::CloseSocket()
{
  RemovePeerByFD(sockfd_);
  (void) close(sockfd_); sockfd_ = -1;
}

void IPCEndpoint::ReceiveMessage(PeerState& p)
{
  ssize_t bytesRead;
  do {
    uint8_t buf[kIPCMessageMaxSize];
    bytesRead = recv(p.GetFD(), buf, sizeof(buf), 0);
    if (bytesRead > 0) {
      std::vector<uint8_t>& incoming_data = p.GetIncomingDataVector();
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

          OnReceiveError(p.GetFD());
          return;
        }
        if (message->version != kIPCMessageVersion) {
          loge("ipc-endpoint: recv'd version %u, expected %u",
               message->version, kIPCMessageVersion);
          OnReceiveError(p.GetFD());
          return;
        }
        if (message->length > kIPCMessageMaxLength) {
          loge("ipc-endpoint: recv'd bad length (%u). max is %zu",
               message->length, kIPCMessageMaxLength);
          OnReceiveError(p.GetFD());
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
          OnReceiveIPCMessage(p.GetFD(), message->type, data);
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
    loge("ipc-endpoint: recv. p.GetFD() = %d, errno = %d (%s)",
         p.GetFD(), errno, strerror(errno));
    OnReceiveError(p.GetFD());
    return;
  }

  if (bytesRead == 0) {
    // Other side closed the socket in an orderly manner
    OnPeerClose(p.GetFD());
    return;
  }
}

void IPCEndpoint::AddPeerByFD(const int fd)
{
  ev::io* read_write_watcher = new ev::io(loop_);
  read_write_watcher->set <IPCEndpoint, &IPCEndpoint::ReadWriteWatcherCallback> (this);
  read_write_watcher->start(fd, ev::READ);
  peers_.emplace_back(read_write_watcher, task_executor_);
}

std::vector<IPCEndpoint::PeerState>::iterator IPCEndpoint::FindPeerByFD(const int fd)
{
  return std::find_if(peers_.begin(),
                      peers_.end(),
                      [fd](const IPCEndpoint::PeerState& state) {
                        return (fd == state.GetFD());
                      });
}

void IPCEndpoint::RemovePeerByFD(const int fd)
{
  auto it = FindPeerByFD(fd);
  if (it != peers_.end()) {
    peers_.erase(it);
  }
}

bool IPCEndpoint::SendMessageToAllPeers(const IPCMessageType type,
                                        uint32_t length,
                                        uint8_t* val)
{
  for (auto const& p : peers_) {
    bool result = SendMessageToPeer(p.GetFD(), type, length, val);
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
  auto it = FindPeerByFD(fd);
  if (it == peers_.end()) {
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

  it->AddMessageToQueue(packed_message);
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
    auto it = FindPeerByFD(w.fd);
    if (it == peers_.end()) {
      return;
    }
    ReceiveMessage(*it);
  }

}

void IPCEndpoint::SendQueuedMessagesToPeer(const int sockfd)
{
  auto it = FindPeerByFD(sockfd);
  if (it == peers_.end()) {
    return;
  }
  while (!it->IsQueueEmpty()) {
    const std::vector<uint8_t>& packed_message = it->GetMessageAtFrontOfQueue();
    ssize_t bytesSent = send(sockfd, packed_message.data(), packed_message.size(), 0);
    if (-1 == bytesSent) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        logw("ipc-endpoint: send would block");
        break;
      }
      OnSendError(sockfd, errno);
      break;
    } else {
      it->EraseMessageFromFrontOfQueue();
    }
  }
}

void IPCEndpoint::OnReceiveError(const int sockfd)
{
  RemovePeerByFD(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

void IPCEndpoint::OnPeerClose(const int sockfd)
{
  RemovePeerByFD(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

void IPCEndpoint::OnSendError(const int sockfd, const int error)
{
  RemovePeerByFD(sockfd);
  if (sockfd == sockfd_) {
    CloseSocket();
  }
}

IPCEndpoint::PeerState::~PeerState()
{
  delete read_write_watcher_; read_write_watcher_ = nullptr;
  delete mutex_; mutex_ = nullptr;
}

int IPCEndpoint::PeerState::GetFD() const
{
  return read_write_watcher_->fd;
}

bool IPCEndpoint::PeerState::IsQueueEmpty()
{
  std::lock_guard<std::mutex> lock(*mutex_);
  return outgoing_queue_.empty();
}

void IPCEndpoint::PeerState::AddMessageToQueue(const std::vector<uint8_t>& message)
{
  std::lock_guard<std::mutex> lock(*mutex_);
  outgoing_queue_.push_back(message);
  task_executor_->Wake([this]() {
      read_write_watcher_->start(read_write_watcher_->fd, ev::READ | ev::WRITE);
    });
}

std::vector<uint8_t> IPCEndpoint::PeerState::GetMessageAtFrontOfQueue()
{
  std::lock_guard<std::mutex> lock(*mutex_);
  return outgoing_queue_.front();
}

void IPCEndpoint::PeerState::EraseMessageFromFrontOfQueue()
{
  std::lock_guard<std::mutex> lock(*mutex_);
  outgoing_queue_.pop_front();
  if (outgoing_queue_.empty()) {
    task_executor_->Wake([this]() {
        read_write_watcher_->set(read_write_watcher_->fd, ev::READ);
      });
  }
}

} // namespace BluetoothDaemon
} // namespace Anki