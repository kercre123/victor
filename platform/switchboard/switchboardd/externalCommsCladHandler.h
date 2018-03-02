#include "signals/simpleSignal.hpp"
#include "clad/externalInterface/messageExternalComms.h"

namespace Anki {
namespace Switchboard {
  class ExternalCommsCladHandler {
    static Anki::Victor::ExternalComms::ExternalComms ReceiveExternalCommsMsg(uint8_t* buffer, size_t length) {
      Anki::Victor::ExternalComms::ExternalComms msg;

      const size_t unpackSize = msg.Unpack(buffer, length);
      if(unpackSize != length) {
        // bugs
      }

      return msg;
    }

    static std::vector<uint8_t> SendExternalCommsMsg(Anki::Victor::ExternalComms::ExternalComms msg) {
      std::vector<uint8_t> messageData(msg.Size());

      const size_t packedSize = msg.Pack(messageData.data(), msg.Size());

      if(packedSize != msg.Size()) {
        // bugs
      }

      return messageData;
    }
  };
} // Switchboard
} // Anki