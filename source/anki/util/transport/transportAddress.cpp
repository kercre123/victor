/**
 * File: transportAddress
 *
 * Author: baustin
 * Created: 1/22/15
 *
 * Description: Describes an endpoint on any of multiple possible transport networks (IP, BLE)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/logging/logging.h"
#include "util/transport/transportAddress.h"
#include <cstdlib>
#include <algorithm>
#include <assert.h>
#include <arpa/inet.h>
#include <sstream>

namespace Anki {
namespace Util {

TransportAddress::TransportAddress()
  : _type(Type::None)
{
}

TransportAddress::TransportAddress(const struct sockaddr_storage& sockaddr)
{
  assert(sockaddr.ss_family == AF_INET || sockaddr.ss_family == AF_INET6);
  if (sockaddr.ss_family == AF_INET) {
    const struct sockaddr_in* sin = (const struct sockaddr_in *)&sockaddr;
    SetIPAddress(sin->sin_addr.s_addr, ntohs(sin->sin_port));
  } else if (sockaddr.ss_family == AF_INET6) {
    const struct sockaddr_in6* sin6 = (const struct sockaddr_in6 *)&sockaddr;
    SetIPv6Address(sin6->sin6_addr, ntohs(sin6->sin6_port));
  }
}

TransportAddress::TransportAddress(uint32_t ipAddress, uint16_t port)
{
  SetIPAddress(ipAddress, port);
}

TransportAddress::TransportAddress(const struct in6_addr& ipv6Address, uint16_t port)
{
  SetIPv6Address(ipv6Address, port);
}

TransportAddress::TransportAddress(uint64_t bleId)
{
  SetBLEEndpointAddress(bleId);
}

TransportAddress::TransportAddress(const char* addressAsString, int32_t port)
{
  SetIPAddressFromString(addressAsString, port);
}

TransportAddress::TransportAddress(const std::string& addressAsString, int32_t port)
{
  SetIPAddressFromString(addressAsString.c_str(), port);
}

void TransportAddress::SetIPAddress(uint32_t address, uint16_t port)
{
  _type = Type::IP;
  _address.ip.addr = address;
  _address.ip.port = port;
}

void TransportAddress::SetIPv6Address(const struct in6_addr& ipv6Address, uint16_t port)
{
  _type = Type::IPv6;
  _address.ipv6.addr = ipv6Address;
  _address.ipv6.port = port;
}

void TransportAddress::SetIPAddressFromString(const char* addressAsString, int32_t port)
{
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_WARNING("TransportAddress.PortOutOfRange", "IP address %s specified with bad port %d", addressAsString, port);
    _type = Type::None;
    return;
  }
  struct in6_addr ipv6addr;
  int rc;
  rc = inet_pton(AF_INET6, addressAsString, &ipv6addr);
  if (1 == rc) {
    SetIPv6Address(ipv6addr, static_cast<uint16_t>(port));
    return;
  } else if (-1 == rc) {
    PRINT_NAMED_WARNING("TransportAddress", "inet_pton(AF_INET6, %s) returned errno = %d",
                        addressAsString, errno);
  }

  struct in_addr ipv4addr;
  rc = inet_pton(AF_INET, addressAsString, &ipv4addr);
  if (1 == rc) {
    SetIPAddress(ipv4addr.s_addr, static_cast<uint16_t>(port));
    return;
  } else if (-1 == rc) {
    PRINT_NAMED_WARNING("TransportAddress", "inet_pton(AF_INET, %s) returned errno = %d",
                        addressAsString, errno);
  }
  PRINT_NAMED_WARNING("TransportAddress", "inet_pton failed to parse %s", addressAsString);
  _type = Type::None;
  return;
}

void TransportAddress::SetIPAddressFromString(const std::string& addressAsString, int32_t port)
{
  SetIPAddressFromString(addressAsString.c_str(), port);
}

void TransportAddress::SetVirtualAddress(uint32_t id)
{
  _type = Type::Virtual;
  _address.virt.id = id;
}

void TransportAddress::SetBLEEndpointAddress(uint64_t id)
{
  _type = Type::BLE;
  _address.ble.id = id;
}

bool TransportAddress::IsIPAddress() const
{
  return _type == Type::IP;
}

bool TransportAddress::IsIPv6Address() const
{
  return _type == Type::IPv6;
}

bool TransportAddress::IsVirtualAddress() const
{
  return _type == Type::Virtual;
}

bool TransportAddress::IsBLEAddress() const
{
  return _type == Type::BLE;
}

uint32_t TransportAddress::GetIPAddress() const
{
  assert(_type == Type::IP);
  if (_type != Type::IP) {
    return 0;
  }
  return _address.ip.addr;
}

struct in6_addr TransportAddress::GetIPv6Address() const
{
  assert(_type == Type::IPv6);
  if (_type != Type::IPv6) {
    return in6addr_any;
  }
  return _address.ipv6.addr;
}

uint16_t TransportAddress::GetIPPort() const
{
  assert(_type == Type::IP || _type == Type::IPv6);
  if (_type == Type::IP) {
    return _address.ip.port;
  } else if (_type == Type::IPv6) {
    return _address.ipv6.port;
  }
  return 0;
}

uint32_t TransportAddress::GetVirtualAddress() const
{
  assert(_type == Type::Virtual);
  if (_type != Type::Virtual) {
    return 0;
  }
  return _address.virt.id;
}

uint64_t TransportAddress::GetBLEAddress() const
{
  assert(_type == Type::BLE);
  if (_type != Type::BLE) {
    return 0;
  }
  return _address.ble.id;
}

constexpr unsigned int TransportAddress::GetSerializedSize()
{
  return sizeof(TransportAddress::_type) + sizeof(TransportAddress::_address);
}

std::string TransportAddress::ToString() const
{
  if (IsVirtualAddress()) {
    return std::string("vr:" + std::to_string(_address.virt.id));
  }
  else if (IsIPAddress()) {
    uint32_t addr = _address.ip.addr;
    const uint8_t* ipBytes = (const uint8_t*)&addr;
    return std::string(std::to_string(ipBytes[0]) + "." + std::to_string(ipBytes[1]) + "." + std::to_string(ipBytes[2]) + "." + std::to_string(ipBytes[3]) + ":" + std::to_string(_address.ip.port));
  }
  else if (IsIPv6Address()) {
    char ipv6_buf[INET6_ADDRSTRLEN];
    const char* ipv6_str = inet_ntop(AF_INET6, &(_address.ipv6.addr), ipv6_buf, sizeof(ipv6_buf));
    std::ostringstream oss;
    oss << '[' << ipv6_str << "]:" << std::to_string(_address.ipv6.port);
    return oss.str();
  }
  else if (IsBLEAddress()) {
    return std::string("ble:" + std::to_string(_address.ble.id));
  }
  return std::string("");
}

bool TransportAddress::operator==(const TransportAddress& rhs) const
{
  if (_type != rhs._type) {
    return false;
  }
  switch (_type) {
    case Type::None:
      return true;
    case Type::IP:
      return (_address.ip.addr == rhs._address.ip.addr && _address.ip.port == _address.ip.port);
    case Type::IPv6:
      return (IN6_ARE_ADDR_EQUAL(&(_address.ipv6.addr),&(rhs._address.ipv6.addr))
              && _address.ipv6.port == _address.ipv6.port);
    case Type::Virtual:
      return (_address.virt.id == rhs._address.virt.id);
    case Type::BLE:
      return (_address.ble.id == rhs._address.ble.id);
  }
}
  
bool TransportAddress::operator!=(const TransportAddress& rhs) const
{
  return !operator==(rhs);
}

static inline int in6_addr_compare(const struct in6_addr* a, const struct in6_addr* b)
{
  return memcmp(&a->s6_addr[0], &b->s6_addr[0], sizeof(struct in6_addr));
}

static inline bool in6_is_addr_less(const struct in6_addr* a, const struct in6_addr* b)
{
  return in6_addr_compare(a, b) < 0;
}

bool TransportAddress::operator<(const TransportAddress& rhs) const
{
  if (_type < rhs._type)
  {
    return true;
  }
  else if (_type == rhs._type)
  {
    // Only compare the union elements relevant for the type - otherwise might be invalid / uninitialized
    switch(_type)
    {
      case Type::None:
        return false;
      case Type::IP:
        return ( (_address.ip.addr <  rhs._address.ip.addr) ||
                ((_address.ip.addr == rhs._address.ip.addr) && (_address.ip.port < rhs._address.ip.port)));
      case Type::IPv6:
        return ( (in6_is_addr_less(&(_address.ipv6.addr), &(rhs._address.ipv6.addr))) ||
                ((IN6_ARE_ADDR_EQUAL(&(_address.ipv6.addr), &(rhs._address.ipv6.addr))
                   && (_address.ipv6.port < rhs._address.ipv6.port))));
      case Type::Virtual:
        return (_address.virt.id < rhs._address.virt.id);
      case Type::BLE:
        return (_address.ble.id < rhs._address.ble.id);
    }
  }
  else
  {
    return false;
  }
}
  
bool TransportAddress::operator>(const TransportAddress& rhs) const
{
  return !operator<=(rhs);
}

bool TransportAddress::operator<=(const TransportAddress& rhs) const
{
  return (operator<(rhs) || operator==(rhs));
}

bool TransportAddress::operator>=(const TransportAddress& rhs) const
{
  return !operator<(rhs);
}

std::size_t TransportAddress::GetHash() const
{
  std::uint64_t result = 0;
  switch (_type) {
    case Type::None:
      break;
    case Type::IP:
      result = (static_cast<std::uint64_t>(_address.ip.port) << 32) | _address.ip.addr;
      break;
    case Type::IPv6:
      result = std::hash<std::string>()(ToString());
      break;
    case Type::Virtual:
      result = _address.virt.id;
      break;
    case Type::BLE:
      result = _address.ble.id;
      break;
  }
  return std::hash<uint64_t>()((static_cast<uint64_t>(_type) << 48) ^ result);
}

} // end namespace Util
} // end namespace Anki
