/**
* File: internetUtils.h
*
* Author: Paul Aluri
* Created: 3/9/2018
*
* Description: Functions related to internet connectivity
*
* Copyright: Anki, Inc. 2018
*
*/

#ifndef __Util__InternetUtils_H__
#define __Util__InternetUtils_H__

#include <stdint.h>

namespace Anki {
namespace Util {

class InternetUtils{
public:

  // Tries to make a TCP connection to hostname at specified port and
  // returns true if successful.
  static bool CanConnectToHostName(const char* hostname, uint16_t port = 80);

  // Convert URL to IP address
  static bool GetIpFromHostName(const char* hostName, char* ipAddressOut);

};

} // namespace Util
} // namespace Anki

#endif // __Util__InternetUtils_H__
