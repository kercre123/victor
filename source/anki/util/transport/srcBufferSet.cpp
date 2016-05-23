/**
 * File: srcBufferSet
 *
 * Author: Mark Wesley
 * Created: 2/03/15
 *
 * Description: Manage a set of non contiguous src buffers that are all to be merged in-order into one outbound message
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "util/transport/srcBufferSet.h"
#include <assert.h>
#include <string.h>


namespace Anki {
namespace Util {

  
// ============================== SizedSrcBuffer ==============================


SizedSrcBuffer::SizedSrcBuffer()
  : _buffer(nullptr)
  , _bufferSize(0)
{
 #if SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  _descriptor[0] = 0;
 #endif
}
  
SizedSrcBuffer::SizedSrcBuffer(const uint8_t* inbuffer, uint32_t inbufferSize, const char* inDescriptor)
  : _buffer(inbuffer)
  , _bufferSize(inbufferSize)
{
 #if SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  if (inDescriptor != nullptr)
  {
    strncpy(_descriptor, inDescriptor, k_MaxDescriptorLen);
    _descriptor[k_MaxDescriptorLen - 1] = 0; // ensure null terminated
  }
  else
  {
    _descriptor[0] = 0;
  }
 #endif
}
  
  
const char* SizedSrcBuffer::GetDescriptor() const
{
 #if SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  return _descriptor;
 #else
  return "";
 #endif
}

  
// ============================== SrcBufferSet ==============================


SrcBufferSet::SrcBufferSet()
  : _bufferCount(0)
{
}


void SrcBufferSet::AddBuffer(const SizedSrcBuffer& inBuffer)
{
  assert(_bufferCount < k_MaxBuffers);
  if (_bufferCount < k_MaxBuffers)
  {
    _buffers[_bufferCount] = inBuffer;
    ++_bufferCount;
  }
}

  
uint32_t SrcBufferSet::CalculateTotalSize() const
{
  uint32_t totalSize = 0;
  for (uint32_t i=0; i < _bufferCount; ++i)
  {
    totalSize += _buffers[i].GetSize();
  }
  return totalSize;
}
  
  
uint8_t* SrcBufferSet::CreateCombinedBuffer() const
{
  const uint32_t totalSize = CalculateTotalSize();
  
  if (totalSize > 0)
  {
    uint8_t* buffer = new uint8_t[totalSize];
    
    uint32_t bytesWritten = 0;
    for (uint32_t i=0 ; i < _bufferCount; ++i)
    {
      const SizedSrcBuffer& srcBuffer = GetBuffer(i);
      const uint32_t srcBufferSize = srcBuffer.GetSize();
      if (srcBufferSize > 0)
      {
        memcpy(&buffer[bytesWritten], srcBuffer.GetBuffer(), srcBufferSize);
        bytesWritten += srcBufferSize;
      }
    }
    assert(bytesWritten == totalSize);
    
    return buffer;
  }
  else
  {
    return nullptr;
  }
}


void SrcBufferSet::DestroyCombinedBuffer(uint8_t* combinedBuffer)
{
   delete[] combinedBuffer;
}
  

} // end namespace Util
} // end namespace Anki

