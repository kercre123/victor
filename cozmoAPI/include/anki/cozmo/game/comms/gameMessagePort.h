/**
 * File: gameMessagePort
 *
 * Author: baustin
 * Created: 6/16/16
 *
 * Description: Contains functions to send & receive Unity messages that can be
 *              injected into C++ layer communication objects
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef ANKI_COZMOAPI_GAME_MESSAGE_PORT_H
#define ANKI_COZMOAPI_GAME_MESSAGE_PORT_H

#include "clad/externalInterface/messageShared.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal.hpp"
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>

namespace Anki {
namespace Cozmo {

class GameMessagePort : private Util::noncopyable
{
public:
  using ReceiveFromUnityFunc = std::function<void(const uint8_t*, size_t)>;

  // for engine to send message to unity
  void PushToGameMessage(const uint8_t* buffer, size_t size);

  // for engine to grab messages from unity
  std::list<std::vector<uint8_t>> PullFromGameMessages();

  // when unity sends messages to engine
  void PushFromGameMessages(const uint8_t* buffer, size_t size);

  // for unity to grab outbound messages from engine
  size_t PullToGameMessages(uint8_t* buffer, size_t bufferSize);

private:
  uint8_t _toGameBuffer[ExternalInterface::kDirectCommsBufferSize];
  size_t _toGameSize{0};
  std::mutex _toGameMutex;

  std::list<std::vector<uint8_t>> _toEngineMessages;
  std::mutex _toEngineMutex;
  bool _toGameBufferFull = false;
};

}
}

#endif
