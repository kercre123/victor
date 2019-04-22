/**
 * File: switchboardd/pairingMessages.h
 *
 * Author: paluri
 * Created: 1/24/2018
 *
 * Description: Actual pairing messages for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef SecurePairingMessages_h
#define SecurePairingMessages_h

#define SB_PAIRING_PROTOCOL_VERSION V6
#define SB_IPv4_SIZE 4
#define SB_IPv6_SIZE 16

#include <stdint.h>

namespace Anki {
namespace Switchboard {
  const std::vector<uint16_t> kRtsVersions = { 2, 4, 5, 6 };

  enum PairingProtocolVersion : uint32_t {
    INVALID                   = 0,
    V1                        = 1,
    V2                        = 2,
    V3                        = 3,
    V4                        = 4,
    V5                        = 5,
    V6                        = 6,
    FACTORY                   = V2,
    CURRENT                   = V6,
  };
  
  enum SetupMessage : uint8_t {
    MSG_RESERVED              = 0,
    MSG_HANDSHAKE             = 1,
  };
  
  enum WifiStatus : uint8_t {
    Success                   = 0,
    WrongPassword             = 1,
    Failure                   = 2,
  };
  
  enum WifiIpFlags : uint8_t {
    None                      = 0,
    Ipv4                      = 1 << 0,
    Ipv6                      = 1 << 1,
  };
} // Switchboard
} // Anki

#endif /* SecurePairingMessages_h */
