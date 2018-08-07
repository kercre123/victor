/**
 * File: directGameComms
 *
 * Author: baustin
 * Created: 6/17/16
 *
 * Description: Implements comms interface for direct communication w/ the game
 *              (doesn't actually use sockets)
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef ANKI_COZMOAPI_DIRECT_GAME_COMMS_H
#define ANKI_COZMOAPI_DIRECT_GAME_COMMS_H

#include "engine/cozmoAPI/comms/iSocketComms.h"
#include <list>
#include <mutex>
#include <vector>

namespace Anki {
namespace Vector {

class GameMessagePort;

class DirectGameComms : public ISocketComms
{
public:
  DirectGameComms(GameMessagePort* messagePipe, ISocketComms::DeviceId hostId);

  virtual bool Init(UiConnectionType connectionType, const Json::Value& config) override;

  virtual bool AreMessagesGrouped() const override { return true; }

  virtual bool ConnectToDeviceByID(DeviceId deviceId) override;
  virtual bool DisconnectDeviceByID(DeviceId deviceId) override;
  virtual bool DisconnectAllDevices() override;

  virtual void GetAdvertisingDeviceIDs(std::vector<ISocketComms::DeviceId>& outDeviceIds) override;

  virtual uint32_t GetNumConnectedDevices() const override;

protected:
  virtual void UpdateInternal() override;

private:
  
  virtual bool SendMessageInternal(const Comms::MsgPacket& msgPacket) override;
  virtual bool RecvMessageInternal(std::vector<uint8_t>& outBuffer) override;

  // received messages = list of uint8_t buffers (vectors)
  std::list<std::vector<uint8_t>> _receivedMessages;

  enum class ConnectionState {
      Disconnected
    , Connecting
    , Connected
  };
  ConnectionState _connectState;
  GameMessagePort* _messagePort;
  ISocketComms::DeviceId _hostId;
};

}
}

#endif
