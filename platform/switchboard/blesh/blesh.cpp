#include "blesh.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "switchboardd/log.h"

namespace Anki {
namespace Switchboard {

Blesh::Blesh(struct ev_loop* loop) :
_loop(loop) 
{}

Blesh::~Blesh() 
{
  if(_connected) {
    ev_io_stop(_loop, &_sshWatcher.watcher);
    close(_sshWatcher.fd);
  }
}

bool Blesh::Connect()
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  if(sock == -1) {
    return false;
  }

  sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(22);

  inet_pton(AF_INET, "127.0.0.1", &(saddr.sin_addr));

  int conn = connect(sock, (sockaddr*)&saddr, sizeof(sockaddr_in));

  if(conn == -1) {
    Log::Write("Blesh: conn == -1");
    Log::Write("Blesh: %d", errno);
    return false;
  }

  _connected = true;

  _sshWatcher.blesh = this;
  _sshWatcher.fd = sock;

  int flags = fcntl(sock, F_GETFL, 0);

  if(flags == -1) {

    return false;
  }

  flags = flags | O_NONBLOCK;
  int setflags = fcntl(sock, F_SETFL, flags);

  if(setflags != 0) {
    return false;
  }

  ev_io_init(&_sshWatcher.watcher, WatcherCallback, sock, EV_READ);
  ev_io_start(_loop, &_sshWatcher.watcher);

  return true;
}

bool Blesh::Disconnect()
{
  if(_connected) {
    ev_io_stop(_loop, &_sshWatcher.watcher);
    close(_sshWatcher.fd);
    _connected = false;
  }
  return true;  
}

bool Blesh::SendData(const std::vector<uint8_t>& data)
{
  ssize_t totalWritten = 0;

  while(totalWritten < data.size()) {
    ssize_t length = write(_sshWatcher.fd, data.data() + totalWritten, data.size() - totalWritten);

    if(length > 0) {
      totalWritten += length;
    } else {
      break;
    }
  }

  return true;
}

void Blesh::WatcherCallback(EV_P_ ev_io *w, int revents)
{
  struct ev_BleshStruct* watcher = (struct ev_BleshStruct*)w;
  uint8_t buffer[4096];
  ssize_t length = read(watcher->fd, buffer, sizeof(buffer));

  if(length < 0) {
    return;
  } else if(length == 0) {
    // Socket reached EOF
    Log::Write("Blesh SSH server disconnected from us.");
    watcher->blesh->Disconnect();
    return;
  }

  std::vector<uint8_t> data(buffer, buffer+length);
  watcher->blesh->OnReceiveData().emit(data);
}

} // Switchboard
} // Ank