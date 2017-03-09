/**
 * File: iIpRetriever
 *
 * Author: baustin
 * Created: 4/24/15
 *
 * Description: Interface for an object that can be injected to retrieve our IP address
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __UtilTransport_IIpRetriever_H__
#define __UtilTransport_IIpRetriever_H__

#include <cstdint>
#include <netinet/in.h>

namespace Anki {
namespace Util {

class IIpRetriever
{
public:
  virtual ~IIpRetriever() = default;
  virtual uint32_t GetIpAddress() = 0;
  virtual struct in6_addr GetIPv6LinkLocalAddress() = 0;
};

} // end namespace Util
} // end namespace Anki

#endif // __UtilTransport_IIpRetriever_H__
