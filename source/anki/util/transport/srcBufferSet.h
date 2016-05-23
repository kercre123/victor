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

#ifndef __UtilTransport_SrcBufferSet_H__
#define __UtilTransport_SrcBufferSet_H__


#include <assert.h>
#include <cstdint>


namespace Anki {
namespace Util {


// ============================== SizedSrcBuffer ==============================


// Enable this to store descriptor text with each SizedSrcBuffer - used for debug printing messages to show field names
#define SIZED_SRC_BUFFER_STORE_DESCRIPTOR   0
  

// Helper to ensure that descriptors are compiled out when not required
#if SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  #define SIZED_SRC_BUFFER_DESCRIPTOR( text )   text
#else
  #define SIZED_SRC_BUFFER_DESCRIPTOR( text )   nullptr
#endif
  
  
class SizedSrcBuffer
{
public:
  
  SizedSrcBuffer();
  SizedSrcBuffer(const uint8_t* inbuffer, uint32_t inbufferSize, const char* inDescriptor = nullptr);
  ~SizedSrcBuffer() { }
  
  const uint8_t* GetBuffer() const
  {
    return _buffer;
  }
  
  const uint32_t GetSize() const
  {
    return _bufferSize;
  }
  
  const char* GetDescriptor() const;
  
private:
  
 #if SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  static const uint32_t k_MaxDescriptorLen = 32;
  char            _descriptor[k_MaxDescriptorLen];
 #endif // SIZED_SRC_BUFFER_STORE_DESCRIPTOR
  
  const uint8_t*  _buffer;
  uint32_t        _bufferSize;
};

  
// ============================== SrcBufferSet ==============================


class SrcBufferSet
{
public:
  
  SrcBufferSet();
  ~SrcBufferSet() { }
  
  void AddBuffer(const SizedSrcBuffer& inBuffer);
  
  uint32_t CalculateTotalSize() const;
  
  uint32_t GetCount() const
  {
    return _bufferCount;
  }
  
  const SizedSrcBuffer& GetBuffer(uint32_t index) const
  {
    assert(index < k_MaxBuffers);
    assert(index < _bufferCount);
    return _buffers[index];
  }
  
  uint8_t* CreateCombinedBuffer() const;
  static void DestroyCombinedBuffer(uint8_t* combinedBuffer);
  
  static const uint32_t k_MaxBuffers = 4;
  
private:
  
  SizedSrcBuffer  _buffers[k_MaxBuffers];
  uint32_t        _bufferCount;
};


} // end namespace Util
} // end namespace Anki

#endif // __UtilTransport_SrcBufferSet_H__
