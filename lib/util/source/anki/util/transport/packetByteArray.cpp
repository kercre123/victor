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


#include "util/transport/packetByteArray.h"
#include <algorithm>
#include <assert.h>
#include <string.h>


namespace Anki {
namespace Util {


PacketByteArray::PacketByteArray()
  : _buffer(nullptr)
  , _size(0)
{
}
 
  
PacketByteArray::~PacketByteArray()
{
  Free();
}

  
PacketByteArray::PacketByteArray(const void* inBuffer, uint32_t inSize)
  : _buffer(nullptr)
  , _size(0)
{
  Set(inBuffer, inSize);
}


PacketByteArray::PacketByteArray(PacketByteArray&& rhs) noexcept
  : _buffer(rhs._buffer)
  , _size(rhs._size)
{
  rhs._buffer = nullptr;
  rhs._size   = 0;
}


PacketByteArray& PacketByteArray::operator=(PacketByteArray&& rhs) noexcept
{
  if (this != &rhs)
  {
    Free();
    _buffer = rhs._buffer;
    _size   = rhs._size;
    rhs._buffer = nullptr;
    rhs._size   = 0;
  }
  return *this;
}


PacketByteArray::PacketByteArray(const PacketByteArray& rhs)
  : _buffer(nullptr)
  , _size(0)
{
  Set(rhs._buffer, rhs._size);
}


PacketByteArray& PacketByteArray::operator=(const PacketByteArray& rhs)
{
  if (this != &rhs)
  {
    Free();
    Set(rhs._buffer, rhs._size);
  }
  return *this;
}


void PacketByteArray::Free()
{
  delete[] _buffer;
  _buffer = nullptr;
  _size = 0;
}


void PacketByteArray::Allocate(uint32_t inSize)
{
  assert(_buffer == nullptr);
  _buffer = new uint8_t[inSize];
  _size   = inSize;
}


void PacketByteArray::Set(const void* inBuffer, uint32_t inSize)
{
  Free();
  
  if (inBuffer == nullptr)
  {
    assert(inSize == 0);
  }
  else
  {
    Allocate(inSize);
    if (inSize > 0)
    {
      memcpy(_buffer, inBuffer, inSize);
    }
  }
}


void PacketByteArray::CopyTo(void* outBuffer, uint32_t& inOutSize) const
{
  const uint32_t clampedSize = std::min(inOutSize, _size);
  if (clampedSize > 0)
  {
    memcpy(outBuffer, _buffer, clampedSize);
  }
  
  inOutSize = clampedSize;
}


} // end namespace Util
} // end namespace Anki

