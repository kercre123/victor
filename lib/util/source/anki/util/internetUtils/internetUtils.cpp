/**
* File: internetUtils.cpp
*
* Author: Paul Aluri
* Created: 3/9/2018
*
* Description: Functions related to internet connectivity
*
* Copyright: Anki, Inc. 2018
*
*/

#include "util/internetUtils/internetUtils.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

namespace Anki {
namespace Util {

bool InternetUtils::CanConnectToHostName(const char* hostName, uint16_t port) {
  struct sockaddr_in addr;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // we will try to make tcp connection using IPv4
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  char ipAddr[100];

  if(strlen(hostName) > 100) {
    // don't allow host names larger than 100 chars
    return false;
  }

  // Try to get IP from host name (will do DNS resolve)
  bool gotIp = GetIpFromHostName(hostName, ipAddr);

  if(!gotIp) {
    return false;
  }

  if(inet_pton(AF_INET, ipAddr, &addr.sin_addr) <= 0) {
    // can't resolve hostname
    return false;
  }

  if(connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    // can't connect to hostname
    return false;
  }

  close(sockfd);

  // success, return true!
  return true;
}

bool InternetUtils::GetIpFromHostName(const char* hostName, char* ipAddressOut) {
  struct hostent* hostEntry;
  struct in_addr** ip;

  hostEntry = gethostbyname(hostName);

  if(hostEntry == nullptr) {
    return false;
  }

  ip = (struct in_addr**)hostEntry->h_addr_list;

  if(ip[0] == nullptr) {
    return false;
  }

  // we have an ip!
  strcpy(ipAddressOut, inet_ntoa(*ip[0]));

  return true;
}

}
}
