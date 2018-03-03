#pragma once

// namespace is sf to avoid renaming all types. this is a vestige from SFML
namespace sf {

class IpAddress {
public:
  static IpAddress getLocalAddress() { return IpAddress(); }
  IpAddress& operator=(const std::string& other) {
    return *this;
  }
  bool operator == (const IpAddress& other) { return false; }
};

class Socket {
public:
  enum Status {
    Done,
//    NotReady,
//    Partial,
//    Disconnected,
//    Error
  };

};


class UdpSocket {
public:
  Socket::Status send (const void *data, std::size_t size, const IpAddress &remoteAddress, unsigned short remotePort){ return Socket::Status::Done;}
  Socket::Status receive (void *data, std::size_t size, std::size_t &received, IpAddress &remoteAddress, unsigned short &remotePort){return Socket::Status::Done;}
  bool isBlocking() const { return false; }
  void setBlocking(bool b) { }
  
  Socket::Status bind (unsigned short port /*, const IpAddress &address=IpAddress::Any*/){ return Socket::Status::Done; }
};
  

  
  
}
