#include <iostream>

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "coretech/common/shared/types.h"
#include "coretech/messaging/shared/LocalUdpClient.h"
#include "coretech/messaging/shared/socketConstants.h"

namespace ARF {

class RobotToArfConverter {
public:
  RobotToArfConverter() { buffer_ = new u8[MAX_PACKET_BUFFER_SIZE]; }

  ~RobotToArfConverter() { delete[] buffer_; }

  bool Connect() {
    const std::string client_path =
        Anki::Vector::ANIM_ROBOT_CLIENT_PATH + std::to_string(0);
    const std::string server_path =
        Anki::Vector::ANIM_ROBOT_SERVER_PATH + std::to_string(0);

    const bool ok = udp_client_.Connect(client_path, server_path);
    if (!ok) {
      std::cout << "Could not connect to robot process" << std::endl;
      return false;
    }
    return true;
  }

  bool SendSync() {
    Anki::Vector::RobotInterface::SyncRobot sync;
    Anki::Vector::RobotInterface::EngineToRobot sync_message(sync);
    return SendMessage(sync_message);
  }

  bool
  SendHeadCommand(const Anki::Vector::RobotInterface::SetHeadAngle &angle) {
    Anki::Vector::RobotInterface::EngineToRobot head_angle_message(angle);
    return SendMessage(head_angle_message);
  }

  void Disconnect() { udp_client_.Disconnect(); }

  bool SendMessage(const Anki::Vector::RobotInterface::EngineToRobot &message) {
    if (!udp_client_.IsConnected())
      return false;
    const ssize_t bytes_sent = udp_client_.Send(
        reinterpret_cast<const char *>(message.GetBuffer()), message.Size());
    if (bytes_sent != message.Size()) {
      std::cout << "Did not send enough data: " << bytes_sent << " instead of "
                << message.Size() << std::endl;
      return false;
    }
    return true;
  }

  // Returns true if there's a state message, and puts the last one into msg.
  bool ReadMessages(Anki::Vector::RobotState *latest_state) {
    Anki::Vector::RobotInterface::RobotToEngine msg;
    bool got_state = false;
    while (true) {
      const ssize_t data_length = udp_client_.Recv(
          reinterpret_cast<char *>(buffer_), MAX_PACKET_BUFFER_SIZE);
      if (data_length < 0) {
        std::cout << "Some problem receiving data\n";
        return false;
      } else if (data_length == 0) {
        break;
      }
      memcpy(msg.GetBuffer(), buffer_, data_length);
      if (msg.Size() != data_length) {
        std::cout << "Size mismatch with RobotToEngine" << std::endl;
        return false;
      }
      if (!msg.IsValid()) {
        std::cout << "Message reports invalid data" << std::endl;
        return false;
      }
      if (msg.tag != Anki::Vector::RobotInterface::RobotToEngine::Tag_micData) {
        if (msg.tag ==
            Anki::Vector::RobotInterface::RobotToEngine::Tag_state) {
          *latest_state = msg.state;
          got_state = true;
        } else {
          std::cout << "Got message with tag " << static_cast<int>(msg.tag)
                    << std::endl;
        }
      }
    }
    return got_state;
  }

private:
  static constexpr int MAX_PACKET_BUFFER_SIZE = 2048;
  u8 *buffer_;
  LocalUdpClient udp_client_;
}; // namespace ARF

} // namespace ARF