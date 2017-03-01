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

#ifndef __NetworkService_TransportAddress_H__
#define __NetworkService_TransportAddress_H__

#include <cstdint>
#include <string>
#include <ostream>
#include <netinet/in.h>

namespace Anki {
namespace Util {

class TransportAddress {

public:
  TransportAddress();
  // Specify port in host order
  TransportAddress(const struct sockaddr_storage& sockaddr);
  TransportAddress(uint32_t ipAddress, uint16_t port);
  TransportAddress(const struct in6_addr& ipv6Address, uint16_t port);
  TransportAddress(const char* addressAsString, int32_t port);
  TransportAddress(const std::string& addressAsString, int32_t port);
  explicit TransportAddress(uint64_t bleId);

  // Specify port in host order
  void SetIPAddress(uint32_t address, uint16_t port);
  void SetIPv6Address(const struct in6_addr& ipv6Address, uint16_t port);
  void SetIPAddressFromString(const char* addressAsString, int32_t port);
  void SetIPAddressFromString(const std::string& addressAsString, int32_t port);
  void SetVirtualAddress(uint32_t id);
  void SetBLEEndpointAddress(uint64_t id);

  bool IsIPAddress() const;
  bool IsIPv6Address() const;
  bool IsVirtualAddress() const;
  bool IsBLEAddress() const;

  uint32_t GetIPAddress() const;
  struct in6_addr GetIPv6Address() const;
  uint16_t GetIPPort() const;
  uint32_t GetVirtualAddress() const;
  uint64_t GetBLEAddress() const;

  constexpr static unsigned int GetSerializedSize();

  std::string ToString() const;
  
  // simple comparison operator
  bool operator==(const TransportAddress& rhs) const;
  bool operator!=(const TransportAddress& rhs) const;
  
  // so this can be used as a key in a map
  bool operator<(const TransportAddress& rhs) const;
  bool operator>(const TransportAddress& rhs) const;
  bool operator<=(const TransportAddress& rhs) const;
  bool operator>=(const TransportAddress& rhs) const;
  
  std::size_t GetHash() const;
  
private:
  enum class Type : char {
      None = 'n'
    , IP = 'i'
    , IPv6 = '6'
    , Virtual = 'v'
    , BLE = 'b'
  };

  Type _type;
  union {
    struct {
      uint32_t addr;
      uint16_t port;
    } ip;
    struct {
      struct in6_addr addr;
      uint16_t port;
    } ipv6;
    struct {
      uint32_t id;
    } virt;
    struct {
      uint64_t id;
    } ble;
  } _address;
};

// for easier debugging
inline std::ostream& operator<<(std::ostream& os, const Anki::Util::TransportAddress& base)
{
  return os << base.ToString();
}
  
} // end namespace util
} // end namespace Anki

// so this can be used as a key in an unordered_map
namespace std {
  template<>
  struct hash<Anki::Util::TransportAddress>
  {
    inline std::size_t operator()(const Anki::Util::TransportAddress& address) const
    {
      return address.GetHash();
    }
  };
}

#endif // __NetworkService_TransportAddress_H__
