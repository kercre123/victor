#include <iostream>

#include "coretech/common/shared/types.h"
#include "coretech/messaging/shared/LocalUdpClient.h"
#include "coretech/messaging/shared/socketConstants.h"

namespace ARF {

class RobotToArfConverter {
public:
  RobotToArfConverter() = default;

  bool Connect() {
    const std::string client_path =
        Anki::Vector::ENGINE_ANIM_CLIENT_PATH + std::to_string(1);
    const std::string server_path =
        Anki::Vector::ENGINE_ANIM_SERVER_PATH + std::to_string(1);

    const bool ok = udp_client_.Connect(client_path, server_path);
    if (!ok) {
      std::cout << "Could not connect to robot process" << std::endl;
      return false;
    }
    return true;
  }

  void Disconnect() { udp_client_.Disconnect(); }

private:
  LocalUdpClient udp_client_;
};

} // namespace ARF