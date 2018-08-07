//
//  BLECozmoMessage.cpp
//  CozmoWifiPOC
//
//  Created by Lee Crippen on 4/18/16.
//  Copyright © 2016 Anki. All rights reserved.
//

#include "BLECozmoMessage.h"
#include "BLECozmoRandom.h"

#include <assert.h>
#include <string.h>
#include <random>

namespace Anki {
namespace Vector {
  
uint8_t BLECozmoMessage::ChunkifyMessage(const uint8_t* bytes, const uint32_t numBytesTotal, bool encrypted)
{
  Clear();
  
  assert(numBytesTotal <= kMessageMaxTotalSize);
  if (numBytesTotal > kMessageMaxTotalSize || numBytesTotal == 0 || nullptr == bytes)
  {
    return 0;
  }
  
  uint8_t numBytesRemaining = numBytesTotal;
  
  const uint8_t* sourceMessageBytes = bytes;
  uint8_t chunkIndex = 0;
  
  const uint8_t flagsBase = encrypted ? kBLEFlagsBase | MESSAGE_ENCRYPT : kBLEFlagsBase;
  const uint8_t flagIndex = kMessageExactMessageLength - kMessageFlagsSize;
  const uint8_t initialChunkDataCap = kMessageChunkSize-1; // we use the first byte to give the size
  
  // Store the overall message size (not including this message reporting byte)
  _messageChunks[chunkIndex][0] = numBytesTotal;

  // Fill out the first chunk
  uint8_t bytesToCopy = numBytesRemaining < initialChunkDataCap ? numBytesRemaining : initialChunkDataCap;
  memcpy(_messageChunks[chunkIndex] + 1, sourceMessageBytes, bytesToCopy);
  sourceMessageBytes += bytesToCopy;
  numBytesRemaining -= bytesToCopy;
  uint8_t lastNumBytesWritten = bytesToCopy + 1; // Include the count byte
  // Init the flags on the first chunk to include message start
  _messageChunks[chunkIndex][flagIndex] = flagsBase | MESSAGE_START;
  
  if (numBytesRemaining > 0)
  {
    ++chunkIndex;
  }
  
  // Loop over middle chunks (not first or last)
  while (numBytesRemaining > kMessageChunkSize)
  {
    memcpy(_messageChunks[chunkIndex], sourceMessageBytes, kMessageChunkSize);
    sourceMessageBytes += kMessageChunkSize;
    numBytesRemaining -= kMessageChunkSize;
    
    _messageChunks[chunkIndex][flagIndex] = flagsBase;
    ++chunkIndex;
  }
  
  // Fill our last chunk here
  if (numBytesRemaining > 0)
  {
    bytesToCopy = numBytesRemaining;
    memcpy(_messageChunks[chunkIndex], sourceMessageBytes, numBytesRemaining);
    numBytesRemaining -= bytesToCopy;
    lastNumBytesWritten = bytesToCopy;
    // Init the flags on the chunk
    _messageChunks[chunkIndex][flagIndex] = flagsBase;
  }
  
  // If we had any leftover bytes on the last chunk, fill them randomly
  if (lastNumBytesWritten < kMessageChunkSize)
  {
    //TODO:  Add android version of this function.  
#ifdef HAVE_GET_RANDOM_BYTES
    BLECozmoGetRandomBytes(_messageChunks[chunkIndex] + lastNumBytesWritten, kMessageChunkSize - lastNumBytesWritten);
#endif
  }
  
  _messageChunks[chunkIndex][flagIndex] |= MESSAGE_END;
  assert(chunkIndex <= kMessageMaxChunks);
  
  _numChunksUsed = chunkIndex + 1;
  _isEncrypted = encrypted;
  
  return _numChunksUsed;
}

uint8_t BLECozmoMessage::DechunkifyMessage(uint8_t* const bufferIn, uint32_t bufferLength) const
{
  if (0 == _numChunksUsed
      || (_messageChunks[0][kMessageExactMessageLength-kMessageFlagsSize] & MESSAGE_ENCRYPT) // TODO:(lc) handle encryption
      || bufferLength < _messageChunks[0][0]
      || nullptr == bufferIn)
  {
    return 0;
  }
  
  uint8_t numBytesRemaining = _messageChunks[0][0];
  uint8_t chunkIndex = 0;
  const uint8_t initialChunkDataCap = kMessageChunkSize-1; // we use the first byte to give the size
  uint8_t* buffer = bufferIn;
  
  uint8_t bytesToCopy = numBytesRemaining < initialChunkDataCap ? numBytesRemaining : initialChunkDataCap;
  memcpy(buffer, _messageChunks[chunkIndex++]+1, bytesToCopy); // Note skipping the first byte that has size
  buffer += bytesToCopy;
  numBytesRemaining -= bytesToCopy;
  
  while (numBytesRemaining > 0 && chunkIndex < _numChunksUsed)
  {
    bytesToCopy = numBytesRemaining < kMessageChunkSize ? numBytesRemaining : kMessageChunkSize;
    memcpy(buffer, _messageChunks[chunkIndex++], bytesToCopy);
    buffer += bytesToCopy;
    numBytesRemaining -= bytesToCopy;
  }
  assert (buffer - bufferIn == _messageChunks[0][0]);
  
  return _messageChunks[0][0];
}

bool BLECozmoMessage::AppendChunk(const uint8_t* bytes, const uint32_t numBytes)
{
  if (kMessageExactMessageLength != numBytes
      || nullptr == bytes
      || IsMessageComplete()
      || kMessageMaxChunks == _numChunksUsed)
  {
    return false;
  }
  
  memcpy(_messageChunks[_numChunksUsed++], bytes, numBytes);
  return true;
}

bool BLECozmoMessage::IsMessageComplete() const
{
  return _numChunksUsed > 0
    && _messageChunks[_numChunksUsed-1][kMessageExactMessageLength-kMessageFlagsSize] & MESSAGE_END;
}
  
} // namespace Vector
} // namespace Anki
