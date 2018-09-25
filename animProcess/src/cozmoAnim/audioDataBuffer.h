/**
 * File: audioDataBuffer.h
 *
 * Author: ross
 * Date:   Jun 9 2018
 *
 * Description: ring buffer with contiguous read memory (not for write)
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __AnimProcess_CozmoAnim_AudioDataBuffer_H_
#define __AnimProcess_CozmoAnim_AudioDataBuffer_H_
#pragma once

#include <mutex>
#include <iostream>

#include <cassert>

namespace Anki {
namespace Vector {
  
class AudioDataBuffer //: public Anki::Util::noncopyable
{
public:
  AudioDataBuffer(int size, int maxReadSize)
    : _size(size)
    , _maxReadSize(maxReadSize)
    , _actualSize( _size + _maxReadSize )
  {
    Reset();
  }
  
  ~AudioDataBuffer() {
    std::lock_guard<std::mutex> lock{ _mutex };
    delete [] _buffer;
  }
  
  void Reset() {
    std::lock_guard<std::mutex> lock{ _mutex };
    if( _buffer ) {
      delete [] _buffer;
    }
    _buffer = new uint8_t[_actualSize];
    _head = 0;
    _tail = 0;
    _full = false;
  }
  
  // returns bytes added. does not add if not enough space!
  // (we could add whatever is available, but this helps with ffwding audio if playback is slow)
  int AddData(const unsigned char* data, const unsigned int len) {
    std::lock_guard<std::mutex> lock{ _mutex };
    
    const int available = GetNumAvailable();
    std::cout << "ADDING " << len << " into avail=" << available << std::endl;
    if( len > available ) {
      return 0;
    }
    
    for( int i=0; i<len; ++i ) {
      int idx = i + _head;
      if( idx < _actualSize ) {
        _buffer[ idx ] = data[i];
      }
      if( idx >= _size ) {
        _buffer[ idx - _size ] = data[i];
        assert(  idx < 2*_size ); // this can happen if writes are huge. todo: deal with this
      }
    }
    _head += len;
    if( _head >= _size ) {
      _head = _head % _size;
    }
    
    
    _full = (len == available);
    
    return len;
  }
  
  // returns null if can't read length
  unsigned char* ReadData(const unsigned int len, bool debug=false) {
    std::lock_guard<std::mutex> lock{ _mutex };
    
    // there's some article about "doing it all wrong" for ring buffers, and I'm doing it here. todo: check that (+1 issue)
    if( debug ) {
      PRINT_NAMED_WARNING("WHATNOW", "head=%d tail=%d, len=%d, isEmpty=%d, size=%d", _head, _tail, len, IsEmpty(), Size());
    }
    
    if( IsEmpty() ) {
      return nullptr;
    } else if( _head > _tail ) {
      if( len <= _head - _tail ) {
        // enough data available
        return _buffer + _tail;
      } else {
        return nullptr;
      }
    } else {
      // tail >= head (tail in front, possibly full)
      if( len <= GetNumAvailable() ) {
        assert( len < _actualSize - _tail );
        return _buffer + _tail;
      } else {
        return nullptr;
      }
    }
  }
  
  bool AdvanceCursor(const unsigned int len) {
    std::lock_guard<std::mutex> lock{ _mutex };
    if( IsEmpty() ) {
      return false;
    } else if( _head > _tail ) {
      if( len <= _head - _tail ) {
        _tail += len;
        _full = (len == (_head - _tail));
        return true;
      } else {
        return false;
      }
    } else {
      // tail >= head (tail in front, possibly full)
      int available = GetNumAvailable();
      if( len <= available ) {
        assert( len < _actualSize - _tail );
        _tail += len;
        if( _tail >= _size ) {
          _tail = _tail % _size;
        }
        _full = (len == available);
        return true;
      } else {
        return false;
      }
    }
  }
  
  inline int Size() const { return GetNumUsed(); }
  
private:
  
  // todo: public versions of these with a mutex (or just move the mutex out of this)
  bool IsFull() const { return _full; }
  bool IsEmpty() const { return !_full && (_head == _tail); }
  
  inline int GetNumAvailable() const {
    return _size - GetNumUsed();
  }
  
  int GetNumUsed() const {
    // mutex?
    int used = _size;
    if( !_full ) {
      if( _head >= _tail) {
        used = _head - _tail;
      } else {
        used = _size + _head - _tail;
      }
    }
    return used;
  }
  
  
  mutable std::mutex _mutex;
  
  uint8_t* _buffer = nullptr;
  
  int _head = 0;
  int _tail = 0;
  bool _full = false;
  
  const int _size;
  const int _maxReadSize;
  const int _actualSize;
  
};

} // namespace
} // namespace

#endif // __AnimProcess_CozmoAnim_AudioLayerBuffer_H_
