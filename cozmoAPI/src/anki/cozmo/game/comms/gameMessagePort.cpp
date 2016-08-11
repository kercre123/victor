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

#include "anki/cozmo/game/comms/gameMessagePort.h"
#include "util/logging/logging.h"
#include <algorithm>
#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {

void GameMessagePort::PushToGameMessage(const uint8_t* buffer, size_t size)
{
  // add to end of buffer, if room
  if (_toGameSize + size > ExternalInterface::kDirectCommsBufferSize) {
    ASSERT_NAMED(false, "GameMessagePort.PushToGameMessage.send_buffer_full");
    return;
  }

  {
    std::lock_guard<std::mutex> bufferLock{_toGameMutex};
    std::copy(buffer, buffer+size, &_toGameBuffer[_toGameSize]);
    _toGameSize += size;
  }
}

size_t GameMessagePort::PullToGameMessages(uint8_t* buffer, size_t bufferSize)
{
  // deal with catastrophes
  ASSERT_NAMED(bufferSize == ExternalInterface::kDirectCommsBufferSize, "GameMessagePort.MismatchedOutboundBuffer");
  if (_toGameSize > bufferSize) {
    ASSERT_NAMED(false, "GameMessagePort.PullToGameMessages.buffer_too_large_to_send");
    _toGameSize = 0;
    return 0;
  }

  if (_toGameSize == 0) {
    // nothing to send
    return 0;
  }

  // copy buffer and zero out size
  size_t sentBytes;
  {
    std::lock_guard<std::mutex> bufferLock{_toGameMutex};
    std::copy(_toGameBuffer, _toGameBuffer + _toGameSize, buffer);
    sentBytes = _toGameSize;
    _toGameSize = 0;
  }
  return sentBytes;
}


std::list<std::vector<uint8_t>> GameMessagePort::PullFromGameMessages()
{
  std::list<std::vector<uint8_t>> returnList;
  if (!_toEngineMessages.empty())
  {
    std::lock_guard<std::mutex> lock{_toEngineMutex};
    returnList = std::move(_toEngineMessages);
    _toEngineMessages.clear();
  }
  return returnList;
}

void GameMessagePort::PushFromGameMessages(const uint8_t* buffer, size_t size)
{
  // add this buffer to our list of outgoing buffers
  {
    std::lock_guard<std::mutex> lock{_toEngineMutex};
    _toEngineMessages.emplace_back(buffer, buffer+size);
  }
}

}
}
