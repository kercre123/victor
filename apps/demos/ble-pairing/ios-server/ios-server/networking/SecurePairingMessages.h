//
//  SecurePairingMessages.h
//  ios-server
//
//  Created by Paul Aluri on 1/24/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef SecurePairingMessages_h
#define SecurePairingMessages_h

#include <stdlib.h>
#include "../sodium/sodium.h"

namespace Anki {
  namespace Networking {
    enum SetupMessage : uint8_t {
      MSG_RESERVED              = 0,
      MSG_ACK                   = 1,
      MSG_REQUEST_INITIAL_PAIR  = 2,
      MSG_REQUEST_RENEW         = 3,
      MSG_PUBLIC_KEY            = 4,
      MSG_NONCE                 = 5,
      MSG_CANCEL_SETUP          = 6,
    };

    enum SecureMessage : uint8_t {
      CRYPTO_RESERVED           = 0,
      CRYPTO_PING               = 1,
      CRYPTO_PONG               = 9,
      CRYPTO_SEND               = 2,
      CRYPTO_RECEIVE            = 3,
      CRYPTO_ACCEPTED           = 4,
      CRYPTO_WIFI_CREDENTIALS   = 5,
      CRYPTO_WIFI_STATUS        = 6,
      CRYPTO_WIFI_REQUEST_IP    = 7,
      CRYPTO_WIFI_RESPOND_IP    = 8,
    };
  }
}

#endif /* SecurePairingMessages_h */
