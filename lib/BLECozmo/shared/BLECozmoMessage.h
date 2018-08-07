//
//  BLECozmoMessage.h
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/18/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#ifndef BLECozmoMessage_h
#define BLECozmoMessage_h

#include <stdint.h>

namespace Anki {
namespace Vector {

class BLECozmoMessage {
public:
  static constexpr uint8_t kMessageMaxTotalSize = 127;
  static constexpr uint8_t kMessageMaxChunks = 8;
  static constexpr uint8_t kMessageChunkSize = 16;
  static constexpr uint8_t kMessageFlagsSize = 1;
  static constexpr uint8_t kMessageExactMessageLength = kMessageChunkSize + kMessageFlagsSize;
  
  // Sets up the chunks for the input message bytes, returns number of chunks
  uint8_t ChunkifyMessage(const uint8_t* bytes, const uint32_t numBytesTotal, bool encrypted = false);
  
  // Writes the stored message to the input buffer. Returns number of bytes written
  uint8_t DechunkifyMessage(uint8_t* const bufferIn, uint32_t bufferLength) const;
  
  // If there is room and the message is not complete, appends a chunk
  bool AppendChunk(const uint8_t* bytes, const uint32_t numBytes);
  
  // 'Clears' the local chunks. In reality just resets our chunks used count
  void Clear() { _numChunksUsed = 0; }
  
  // Returns back the nubmer of chunks currently used
  uint8_t GetNumChunks() const { return _numChunksUsed; }
  
  // Returns a pointer to the data for a given chunk, or nullptr if the chunk index isn't used
  const uint8_t* GetChunkData(uint8_t chunkIndex) { return chunkIndex < _numChunksUsed ? _messageChunks[chunkIndex] : nullptr; }
  
  // The number of bytes in a chunk
  uint8_t GetChunkSize() const { return kMessageExactMessageLength; }
  
  bool IsMessageComplete() const;
  
private:
  static constexpr uint8_t kBLEFlagsBase = 0;
  enum BLEChunkFlags {
    MESSAGE_START   = 1 << 0,
    MESSAGE_END     = 1 << 1,
    MESSAGE_ENCRYPT = 1 << 2,
  };
  
  uint8_t _messageChunks[kMessageMaxChunks][kMessageExactMessageLength];
  uint8_t _numChunksUsed = 0;
  bool _isEncrypted = false;
};

} // namespace Vector
} // namespace Anki


#endif /* BLECozmoMessage_h */
