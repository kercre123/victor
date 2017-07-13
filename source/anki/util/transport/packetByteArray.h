/**
 * File: PacketByteArray
 *
 * Author: Mark Wesley
 * Created: 02/16/15
 *
 * Description: Simple class for dynamically allocating and copying arbitrary data from
 *              network packets for queuing etc. before copying back out again later.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_PacketByteArray_H__
#define __NetworkService_PacketByteArray_H__


#include <stdint.h>
#include "util/math/math.h"
#include "util/math/numericCast.h"


namespace Anki {
namespace Util {


class PacketByteArray
{
public:
  
  PacketByteArray();
  ~PacketByteArray();
  
  PacketByteArray(const void* inBuffer, uint32_t inSize);
  
  // Move Constructors and Move Assignment - marked noexcept so that std::vector will use them on resize
  PacketByteArray(PacketByteArray&& rhs) noexcept;
  PacketByteArray& operator=(PacketByteArray&& rhs) noexcept;
  
  // Copy Constructor and copy assignment
  PacketByteArray(const PacketByteArray& rhs);
  PacketByteArray& operator=(const PacketByteArray& rhs);
  
  void Free();
  void Allocate(uint32_t inSize);
  
  void Set(const void* inBuffer, uint32_t inSize);
  
  void CopyTo(void* outBuffer, uint32_t& inOutSize) const;
  template <typename size_type>
  void CopyTo(void* outBuffer, size_type& inOutSize) const
  {
    uint32_t inOutIntSize = Anki::Util::numeric_cast<uint32_t>(inOutSize);
    CopyTo(outBuffer, inOutIntSize);
    inOutSize = Anki::Util::numeric_cast<size_type>(inOutIntSize);
  }
  
  uint8_t*       GetBuffer()       { return _buffer; }
  const uint8_t* GetBuffer() const { return _buffer; }
  uint32_t       GetSize()   const { return _size; }
  
private:
  
  uint8_t*  _buffer;
  uint32_t  _size;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_PacketByteArray_H__
