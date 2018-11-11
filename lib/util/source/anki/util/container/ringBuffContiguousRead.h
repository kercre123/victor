/**
 * File: ringBuffContiguousRead.h
 *
 * Author: ross
 * Date:   Jun 9 2018
 *
 * Description: Ring buffer with contiguous READ memory of a guaranteed minimum size.
 *              The size of the backing buffer is increased by the expected max read size.
 *              This is intended for use with routines that need access to C arrays.
 *              Unlike CircularBuffer and FixedCircularBuffer, you will not be able to
 *              add data until enough is consumed.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Util_Container_RingBuffContiguousRead_H__
#define __Util_Container_RingBuffContiguousRead_H__
#pragma once

#include <vector>
#include <cassert>

namespace Anki {
namespace Util {
  
template <typename T>
class RingBuffContiguousRead
{
public:
  
  RingBuffContiguousRead( size_t size, size_t maxReadSize )
    : _capacity( size )
    , _maxReadSize( maxReadSize )
    , _actualSize( _capacity + _maxReadSize )
  {
    Reset();
  }
  
  void Reset()
  {
    _buffer = std::vector<T>(_actualSize, T{});
    _head = 0;
    _tail = 0;
    _full = false;
  }
  
  // Returns bytes added. Does not add anything if not enough space!
  size_t AddData( const T* data, const unsigned int len )
  {
    const size_t available = GetNumAvailable();
    if( len > available ) {
      return 0;
    }
    
    for( size_t i=0; i<len; ++i ) {
      size_t idx = i + _head;
      if( idx < _actualSize ) {
        _buffer[ idx ] = data[ i ];
      }
      if( idx >= _capacity ) {
        // also write at the beginning of the array
        assert(  idx < 2*_capacity );
        _buffer[ idx - _capacity ] = data[ i ];
      }
      if( idx < _maxReadSize ) {
        // also write at the end of the array
        _buffer[ (_actualSize + idx) - _maxReadSize ] = data[ i ];
      }
    }
    _head += len;
    if( _head >= _capacity ) {
      _head = _head % _capacity;
    }
    
    
    _full = (len == available);
    
    return len;
  }
  
  // Returns null if can't read length len
  const T* ReadData( const unsigned int len ) const
  {
    assert( len <= GetContiguousSize() );
    if( IsEmpty() ) {
      return nullptr;
    } else if( _head > _tail ) {
      if( len <= _head - _tail ) {
        // enough data available
        return _buffer.data() + _tail;
      } else {
        return nullptr;
      }
    } else {
      // tail >= head (tail in front, possibly full)
      if( len <= GetNumUsed() ) {
        assert( len <= _actualSize - _tail );
        return _buffer.data() + _tail;
      } else {
        return nullptr;
      }
    }
  }
  
  // If enough read data is available, moves the read cursor forward by len and returns true, otherwise false.
  bool AdvanceCursor( const unsigned int len )
  {
    if( IsEmpty() ) {
      return false;
    } else if( _head > _tail ) {
      if( len <= _head - _tail ) {
        _full = false;
        _tail += len;
        return true;
      } else {
        return false;
      }
    } else {
      // tail >= head (tail in front, possibly full)
      size_t maxNumRead = GetNumUsed();
      if( len <= maxNumRead ) {
        _full = false;
        assert( len <= _actualSize - _tail );
        _tail += len;
        if( _tail >= _capacity ) {
          _tail = _tail % _capacity;
        }
        return true;
      } else {
        return false;
      }
    }
  }
  
  // This class guarantees at least _maxReadSize contiguous elements on a read as long as there are
  // at least that many elements written. But the contiguous region may be larger. Check the size here.
  size_t GetContiguousSize() const
  {
    size_t contiguousSize;
    if( IsEmpty() ) {
      contiguousSize = 0;
    } else if( _head > _tail ) {
      contiguousSize = _head - _tail;
    } else {
      contiguousSize = std::min( Size(), _actualSize - _tail );
    }
    assert( contiguousSize >= std::min(Size(), _maxReadSize) );
    return contiguousSize;
  }
  
  inline size_t Capacity() const { return _capacity; }
  inline size_t Size() const { return GetNumUsed(); }
  bool IsEmpty() const { return !_full && (_head == _tail); }
  
private:
  
  bool IsFull() const { return _full; }
  
  inline size_t GetNumAvailable() const {
    return _capacity - GetNumUsed();
  }
  
  size_t GetNumUsed() const {
    if( IsEmpty() ) {
      return 0;
    }
    size_t used = _capacity;
    if( !_full ) {
      if( _head >= _tail) {
        used = _head - _tail;
      } else {
        used = _capacity + _head - _tail;
      }
    }
    return used;
  }
  
  std::vector<T> _buffer;
  
  size_t _head = 0;
  size_t _tail = 0;
  
  bool _full = false;
  
  const size_t _capacity;
  const size_t _maxReadSize;
  const size_t _actualSize;
  
};

} // namespace
} // namespace

#endif // __Util_Container_RingBuffContiguousRead_H__
