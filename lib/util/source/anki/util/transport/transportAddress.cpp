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

namespace Anki {
namespace Util {

TransportAddress::TransportAddress()
  : _type(Type::None)
{
}

TransportAddress::TransportAddress(uint32_t ipAddress, uint16_t port)
{
  SetIPAddress(ipAddress, port);
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

void TransportAddress::SetIPAddressFromString(const char* addressAsString, int32_t port)
{
  if (port < 0 || port >= 0x10000) {
    PRINT_NAMED_WARNING("TransportAddress", "IP address %s specified with bad port %d.", addressAsString, port);
    _type = Type::None;
    return;
  }
  
  uint32_t ipAddress = IPAddressStringToU32(addressAsString);
  SetIPAddress(ipAddress, static_cast<uint16_t>(port));
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

uint16_t TransportAddress::GetIPPort() const
{
  assert(_type == Type::IP);
  if (_type != Type::IP) {
    return 0;
  }
  return _address.ip.port;
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
  else if (IsBLEAddress()) {
    return std::string("ble:" + std::to_string(_address.ble.id));
  }
  return std::string("");
}

uint32_t TransportAddress::IPAddressStringToU32(const char* addressAsString)
{
  const char ipSeparator = '.';
  const char * pch = addressAsString;
  
  uint32_t returnVal = 0;
  uint32_t numValues = 0;
  while (pch != nullptr)
  {
    const int partValue = atoi(pch);
    if ((partValue >= 0) && (partValue <= 0xff))
    {
      returnVal = (returnVal << 8) | (partValue);
      ++numValues;
    }
    else
    {
      PRINT_NAMED_WARNING("TransportAddress", "pAddress part '%s' (of '%s') converts to out of range value %d!", pch, addressAsString, partValue);
      return 0;
    }
    pch = strchr(pch, ipSeparator);
    pch = (pch != nullptr) ? pch + 1 : nullptr; // skip the delimeter
  }
  
  if (numValues != 4)
  {
    PRINT_NAMED_WARNING("TransportAddress", "ipAddress '%s' had %d (!= 4) valid numeric '%c' separated values!", addressAsString, numValues, ipSeparator);
    return 0;
  }
  
  return htonl(returnVal);
}
  
uint32_t TransportAddress::IPAddressStringToU32(const std::string& addressAsString)
{
  return IPAddressStringToU32(addressAsString.c_str());
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
