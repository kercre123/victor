/**
 * File: fixedCircularBuffer
 *
 * Author: Lee Crippen
 * Created: 11/29/17
 *
 * Description: Fixed memory and storage size (no dynamic allocations) circular buffer.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#ifndef __Util_Container_FixedCircularBuffer_H__
#define __Util_Container_FixedCircularBuffer_H__


#include <assert.h>


namespace Anki {
namespace Util {

  
template <class T, size_t kCapacity>
class FixedCircularBuffer
{
public:
  FixedCircularBuffer() = default;
  ~FixedCircularBuffer() = default;

  FixedCircularBuffer(const FixedCircularBuffer& rhs)
  {
    if (rhs.size() > 0)
    {
      push_back(&(rhs.front()), rhs.size());
    }
  }

  FixedCircularBuffer& operator=(const FixedCircularBuffer& rhs)
  {
    if (this != &rhs)
    {
      _firstIndex = 0;
      _numEntries = 0;
      if (rhs.size() > 0)
      {
        push_back(&(rhs.front()), rhs.size());
      }
    }
    return *this;
  }

  FixedCircularBuffer(FixedCircularBuffer&& rhs) noexcept
  {
    if (rhs.size() > 0)
    {
      push_back(&(rhs.front()), rhs.size());
    }
    // we only need to reset the following to ensure the buffer is still in a valid and reliable state
    rhs._firstIndex = 0;
    rhs._numEntries = 0;
  }
  
  FixedCircularBuffer& operator=(FixedCircularBuffer&& rhs) noexcept
  {
    if (this != &rhs)
    {
      _firstIndex = 0;
      _numEntries = 0;
      if (rhs.size() > 0)
      {
        push_back(&(rhs.front()), rhs.size());
      }
      // we only need to reset the following to ensure the buffer is still in a valid and reliable state
      rhs._firstIndex = 0;
      rhs._numEntries = 0;
    }
    return *this;
  }

  bool operator==(const Anki::Util::FixedCircularBuffer<T, kCapacity>& rhs) const
  {
    if (size() != rhs.size()) {
      return false;
    }
    
    for (size_t idx = 0; idx < size(); ++idx) {
      if ((*this)[idx] != rhs[idx]) {
        return false;
      }
    }
    
    return true;
  }
  
  bool operator!=(const Anki::Util::FixedCircularBuffer<T, kCapacity>& rhs) const
  {
    return !(*this == rhs);
  }
  
  void Reset()
  {
    _firstIndex = 0;
    _numEntries = 0;
  }
  
  void clear() { Reset(); }
  
  void pop_front()
  {
    // Remove oldest entry
    
    assert(_numEntries > 0);
    
    ++_firstIndex;
    --_numEntries;
    if (_firstIndex >= kCapacity)
    {
      _firstIndex = 0;
    }
  }
  
  void pop_front( size_t popCount )
  {
    assert(_numEntries >= popCount);
    
    _firstIndex = ( _firstIndex + popCount ) % kCapacity;
    _numEntries -= popCount;
  }
  
  void pop_back()
  {
    // Remove newest entry
    
    assert(_numEntries > 0);
    --_numEntries;
  }
  
  T& push_front()
  {
    assert(_numEntries <= kCapacity);
    if (_numEntries >= kCapacity)
    {
      pop_back();
    }
    
    _firstIndex = (_firstIndex > 0) ? (_firstIndex - 1) : (kCapacity - 1);
    ++_numEntries;
    return _buffer[_firstIndex];
  }
  
  T& push_back()
  {
    assert(_numEntries <= kCapacity);
    if (_numEntries >= kCapacity)
    {
      pop_front();
    }
    
    const size_t newIndex = ItemIndexToBufferIndex(_numEntries);
    ++_numEntries;
    return _buffer[newIndex];
  }
  
  void push_front(const T& newEntry)
  {
    auto& nextSlot = push_front();
    nextSlot = newEntry;
  }
  
  // Push back continous data without copy
  // Return false if there is not enough continuous memory
  bool push_back(T*& out_dataHeadPtr, size_t pushSize)
  {
    out_dataHeadPtr = nullptr;
    const size_t dataIndex = ItemIndexToBufferIndex(_numEntries);
    // Check if there is enough room
    const long remainingCap = capacity() - size();
    // Remaing space before end of array
    const long remainingSpace = capacity() - dataIndex - pushSize;
    if ( (remainingCap < pushSize) || (remainingSpace < 0) )
    {
      return false;
    }
    out_dataHeadPtr = &_buffer[dataIndex];
    _numEntries += pushSize;
    return true;
  }
  
  void push_back(const T& newEntry)
  {
    auto& nextSlot = push_back();
    nextSlot = newEntry;
  }
  
  void push_back(const T* newEntryArray, size_t arraySize)
  {
    // If the incoming array is too big, skip forward over the entries that would be overwritten
    if (arraySize > capacity())
    {
      newEntryArray += (arraySize - capacity());
      arraySize = capacity();
    }
    for (size_t i=0; i < arraySize; ++i)
    {
      push_back(*(newEntryArray++));
    }
  }
  
  size_t size() const     { return _numEntries; }
  size_t capacity() const { return kCapacity; }
  bool   empty() const    { return (_numEntries == 0); }
  
  const T& operator[](size_t itemIndex) const
  {
    assert(itemIndex < _numEntries);
    const size_t bufferIndex = ItemIndexToBufferIndex(itemIndex);
    return _buffer[bufferIndex];
  }
  
  T& operator[](size_t itemIndex)
  {
    assert(itemIndex < _numEntries);
    const size_t bufferIndex = ItemIndexToBufferIndex(itemIndex);
    return _buffer[bufferIndex];
  }
  
  const T& front() const
  {
    return (*this)[0];
  }
  
  T& front()
  {
    return (*this)[0];
  }
  
  size_t front(T* outArray, size_t arraySize)
  {
    const size_t availableSize = (arraySize <= _numEntries) ? arraySize : _numEntries;
    for (size_t i=0; i < availableSize; ++i)
    {
      *(outArray++) = _buffer[ItemIndexToBufferIndex(i)];
    }
    return availableSize;
  }
  
  const T& back() const
  {
    return (*this)[size()-1];
  }
  
  T& back()
  {
    return (*this)[size()-1];
  }


private:
  
  size_t ItemIndexToBufferIndex(size_t itemIndex) const
  {
    assert(itemIndex < kCapacity);
    const size_t offsetIndex = _firstIndex + itemIndex;
    const size_t bufferIndex = offsetIndex % kCapacity;
    return bufferIndex;
  }
  
  T       _buffer[kCapacity];
  size_t  _firstIndex = 0;
  size_t  _numEntries = 0;
};

} // end namespace Util
} // end namespace Anki

#endif // __Util_Container_FixedCircularBuffer_H__
