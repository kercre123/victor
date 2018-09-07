/**
 * File: audioLayerBuffer.h
 *
 * Author: ross
 * Date:   Jun 9 2018
 *
 * Description: Container for caching allocated audio data that offers pointers to continguous memory without copying
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __AnimProcess_CozmoAnim_AudioLayerBuffer_H_
#define __AnimProcess_CozmoAnim_AudioLayerBuffer_H_
#pragma once

#include <sstream>

namespace Anki {
namespace Vector {
  

class AudioLayerBuffer {
public:
  explicit AudioLayerBuffer( int maxReadSize )
   : _maxReadSize( maxReadSize )
  {
    _gap.resize( 2 * _maxReadSize, 0 );
  }

  int GetNextPointer( size_t size, unsigned char*& ptr )
  {
    assert( size <= _maxReadSize );
    if( _data.empty() ) {
      ptr = nullptr;
      return 0;
    }
    if( !_usingGap ) {
      const int remainingInDeque = ((int)_data.front().size() - _idxInDeque);
      const auto& front = _data[0];

      if( _data.size() == 1 ) {
        // already at the end of the deque.
        ptr = (unsigned char*)front.data() + _idxInDeque;
        return std::min((int)size, remainingInDeque);
      } else if( size <= remainingInDeque ) {
        // may be multiple stages, but not requesting more than what's in the current one
        ptr = (unsigned char*)front.data() + _idxInDeque;
        return (int)size;
      } else {
        // requested more than in the current deque, and there are more data stages.
        // switch to gap
        
        // copy from front
        for( int i=0; i<remainingInDeque; ++i ) {
          _gap[i] = front[_idxInDeque + i];
        }
        _gapSize = remainingInDeque;
        // copy from next
        const auto& next = _data[1];
        // -1 because worst cast read is from the last position in front, of size _maxReadSize
        int numTakenFromNext = std::min( (int)_maxReadSize - 1, (int)_data[1].size()  );
        for( int i=0; i<numTakenFromNext; ++i ) {
          _gap[i + _gapSize] = next[i];
        }
        _gapSize += numTakenFromNext;
        _idxInGap = 0;
        _usingGap = true;
      }
    }

    if( _usingGap ) {
      ptr = (unsigned char*)_gap.data() + _idxInGap;
      return std::min(_gapSize, (int)size);
    }

    assert(false);
    ptr = nullptr;
    return 0;
  }

  void AdvanceBy( size_t size )
  {
    if( _usingGap ) {
      const int remainingInDeque = ((int)_data.front().size() - _idxInDeque);
      if( size >= remainingInDeque ) {
        // move to next layer and stop using the gap
        _usingGap = false;
        _gapSize = 0;
        _idxInDeque = ((int)size - remainingInDeque);
        _data.pop_front();
        while( !_data.empty() && (_idxInDeque >= _data.front().size()) ) {
          _idxInDeque -= _data.front().size();
          _data.pop_front();
        }
        if( _data.empty() ) {
          _idxInDeque = 0;
        }
        _idxInGap = 0;
      } else {
        _idxInGap += size;
        _idxInDeque += size;
      }
    } else {
      _idxInDeque += size;
      while( _idxInDeque >= _data.front().size() && !_data.empty() ) {
        _idxInDeque -= _data.front().size();
        _data.pop_front();
      }
      if( _data.empty() ) {
        _idxInDeque = 0;
      }
    }
  }

  using Layer = std::vector<uint8_t>;
  void AddData( Layer&& data )
  {
    _data.push_back(std::move(data));
  }

  void Print(bool verbose = false) {
    PRINT_NAMED_WARNING("WHATNOW", "_idxInDeque=%d, _idxInGap=%d, _usingGap=%d, _gapSize=%d, layers=%zu, bytes=%zu",
                        _idxInDeque, _idxInGap, _usingGap, _gapSize, _data.size(), Size());
    if( verbose ) {
      std::stringstream ss;
      for( int i=0; i<_data.size(); ++i ) {
        for( int j=0; j<_data[i].size(); ++j ) {
          ss << std::to_string(_data[i][j]) << ",";
        }
        ss << " | ";
      }
      ss << std::endl << " ";
      for( int i=0; i<_gap.size(); ++i ) {
        ss << std::to_string(_gap[i]) << ",";
      }
      PRINT_NAMED_WARNING("WHATNOW", "%s", ss.str().c_str());
    }
    
  }

  size_t Size() const {
    size_t size = 0;
    for( const auto& layer : _data ) {
      size += layer.size();
    }
    return size;
  }
  size_t Layers() const {
    return _data.size();
  }
  
  void Reset() {
    _idxInDeque = 0;
    _idxInGap = 0;
    _usingGap = false;
    _gapSize = 0;
    _data.clear();
    std::fill( _gap.begin(), _gap.end(), 0);
  }

private:
  int _maxReadSize;

  int _idxInDeque=0;
  int _idxInGap=0;
  bool _usingGap = false;
  int _gapSize = 0;

  std::vector<unsigned char> _gap;
  std::deque<Layer> _data;
};

} // namespace
} // namespace

#endif // __AnimProcess_CozmoAnim_AudioLayerBuffer_H_
