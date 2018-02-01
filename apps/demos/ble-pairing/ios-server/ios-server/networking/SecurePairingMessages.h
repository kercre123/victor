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
      MSG_ACK                   = 1,
      MSG_REQUEST_INITIAL_PAIR  = 2,
      MSG_REQUEST_RENEW         = 3,
      MSG_PUBLIC_KEY            = 4,
      MSG_NONCE                 = 5,
      MSG_CANCEL_SETUP          = 6,
    };

    enum SecureMessage : uint8_t {
      CRYPTO_PING               = 1,
      CRYPTO_SEND               = 2,
      CRYPTO_RECEIVE            = 3,
      CRYPTO_ACCEPTED           = 4,
      CRYPTO_WIFI_CRED          = 5,
      CRYPTO_WIFI_RESULT        = 6,
    };
  }
}

#endif /* SecurePairingMessages_h */
