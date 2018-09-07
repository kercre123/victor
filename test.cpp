// todo:? #define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "animProcess/src/cozmoAnim/minimp3.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <deque>

using FileDataType = char;
static std::vector<FileDataType> ReadAllBytes(char const* filename)
{
  std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
  std::ifstream::pos_type pos = ifs.tellg();

  std::vector<FileDataType> result(pos);

  ifs.seekg(0, std::ios::beg);
  ifs.read(&result[0], pos);

  return result;
}

void PrintInfo( const mp3dec_frame_info_t& info ) {
  std::cout << "frame_bytes=" << info.frame_bytes << " "
      << "channels=" << info.channels << " "
      << "hz=" << info.hz << " "
      << "layer=" << info.layer << " "
      << "bitrate_kbps=" << info.bitrate_kbps << std::endl;
}

struct OnlineMusicData
{
  std::vector<uint8_t> data;
  uint16_t siz;
};

// // a circular buffer with overlapping data ==> continguous memory
// // todo: consider doing this instead with a deque of OnlineMusicData with some mechanic to span the gaps
// // to avoid (some) copies, and only copy those data on the border of two regions
// class DataBuffer {
// public:
//   DataBuffer( int maxReadSize, int capacity)
//    : _maxReadSize( maxReadSize )
//    , _capacity(capacity)
//   {
//     _data.resize( _capacity + _maxReadSize, 0 );
//   }

//   size_t GetNextPointer( size_t size, unsigned char*& ptr )
//   {
//     assert( size <= _maxReadSize );
//     if( !_looped ) {
//       int remaining = (_prodIdx - _consIdx);
//       ptr = (remaining == 0) ? nullptr : (_data.data() + _consIdx);
//       return std::min((int)size,remaining);
//     } else {
//       ptr = _data.data() + _consIdx;
//       return size;
//     }
//   }

//   void AdvanceBy( size_t size ) 
//   {
//     _consIdx += size;
//     if( _consIdx >= _capacity ) {
//       _consIdx = _consIdx % _capacity;
//     }
//   }

//   void AddData( const OnlineMusicData& data )
//   {
//     for( int i=0; i<data.siz; ++i ) {
//       _data[_prodIdx] = data.data[i];

//       if( _prodIdx >= _capacity ) {
//         _data[(_prodIdx - _capacity)] = data.data[i];
//       }

//       ++_prodIdx;
//       if( _prodIdx >= _data.size() ) {
//         _looped = true;
//         _prodIdx = _prodIdx - _capacity;
//       }
//     }
//   }
//   void Print() {
//     std::cout << "_prodIdx=" << _prodIdx << ", _consIdx=" << _consIdx << ": ";
//     for( int i=0; i<_data.size(); ++i ) {
//       std::cout << std::to_string(_data[i]) << ",";
//     }
//     std::cout << std::endl;
//   }

// private:
//   int _prodIdx = 0;
//   int _consIdx = 0;
//   int _maxReadSize;
//   int _capacity;
//   bool _looped = false;
//   std::vector<unsigned char> _data;
// };

class DataBuffer {
public:
  explicit DataBuffer( int maxReadSize )
   : _maxReadSize( maxReadSize )
  {
    _gap.resize( 2 * _maxReadSize, 0 );
  }

  ssize_t GetNextPointer( size_t size, unsigned char*& ptr )
  {
    assert( size <= _maxReadSize );
    if( _data.empty() ) {
      ptr = nullptr;
      return 0;
    }
    if( !_usingGap ) {
      const int remainingInDeque = (_data.front().siz - _idxInDeque);
      const auto& front = _data[0].data;

      if( _data.size() == 1 ) {
        // already at the end of the deque.
        ptr = (unsigned char*)front.data() + _idxInDeque;
        return std::min(size, (size_t)remainingInDeque);
      } else if( size <= remainingInDeque ) {
        // may be multiple stages, but not requesting more than what's in the current one
        ptr = (unsigned char*)front.data() + _idxInDeque;
        return size;
      } else {
        // requested more than in the current deque, and there are more data stages. 
        // switch to gap
        
        // copy from front
        for( int i=0; i<remainingInDeque; ++i ) {
          _gap[i] = front[_idxInDeque + i];
        }
        _gapSize = remainingInDeque;
        // copy from next
        const auto& next = _data[1].data;
        // -1 because worst cast read is from the last position in front, of size _maxReadSize
        int numTakenFromNext = std::min( (int)_maxReadSize - 1, (int)_data[1].siz  );
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
      return std::min((size_t)_gapSize, size);
    }

    assert(false);
    ptr = nullptr;
    return 0;
  }

  void AdvanceBy( size_t size )
  {
    if( _usingGap ) {
      const int remainingInDeque = (_data.front().siz - _idxInDeque);
      if( size >= remainingInDeque ) {
        // move to next deque and stop using the gap
        _usingGap = false;
        _gapSize = 0;
        _idxInDeque = (size - remainingInDeque);
        _data.pop_front();
        while( !_data.empty() && (_idxInDeque >= _data.front().siz) ) {
          _idxInDeque -= _data.front().siz;
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
      while( _idxInDeque >= _data.front().siz && !_data.empty() ) {
        _idxInDeque -= _data.front().siz;
        _data.pop_front();
      }
      if( _data.empty() ) {
        _idxInDeque = 0;
      }
    }
  }

  void AddData( OnlineMusicData&& data )
  {
    _data.push_back(std::move(data));
  }

  void Print(bool verbose = false) {
    std::cout << "_idxInDeque=" << _idxInDeque << ", _idxInGap=" << _idxInGap << ", _usingGap=" << _usingGap << ", _gapSize=" << _gapSize << ", layers=" << _data.size() << ", bytes=" << Size() << ": " << std::endl << " ";
    if( verbose ) {
      for( int i=0; i<_data.size(); ++i ) {
        for( int j=0; j<_data[i].data.size(); ++j ) {
          std::cout << std::to_string(_data[i].data[j]) << ",";
        }
        std::cout << " | ";
      }
      std::cout << std::endl << " ";
      for( int i=0; i<_gap.size(); ++i ) {
        std::cout << std::to_string(_gap[i]) << ",";
      }
    }
    std::cout << std::endl;
  }

  size_t Size() const {
    size_t size = 0;
    for( const auto& layer : _data ) {
      size += layer.siz;
    }
    return size;
  }
  size_t Layers() const {
    return _data.size();
  }

private:
  int _maxReadSize;

  int _idxInDeque=0;
  int _idxInGap=0;
  bool _usingGap = false;
  int _gapSize = 0;

  std::vector<unsigned char> _gap;
  std::deque<OnlineMusicData> _data;
};

OnlineMusicData GetData()
{
  OnlineMusicData tmp1;
  tmp1.data = {0,1,2,3,4,5,6};
  tmp1.siz = 7;
  return tmp1;
}

int main()
{
  // DataBuffer test(5);
  // auto tmp1 = GetData();
  // test.AddData(std::move(tmp1));
  // auto tmp2 = GetData();
  // test.AddData(std::move(tmp2));
  // test.Print();
  // unsigned char* ptr = nullptr;
  // size_t read = test.GetNextPointer(5, ptr);
  // std::cout << "allowed_read=" << read << std::endl;
  // test.AdvanceBy(read);
  // test.Print();
  // read = test.GetNextPointer(5, ptr);
  // std::cout << "allowed_read=" << read << std::endl;
  // test.Print();
  // test.AdvanceBy(1);
  // test.Print();
  // read = test.GetNextPointer(5, ptr);
  // std::cout << "allowed_read=" << read << std::endl;
  // test.Print();
  // test.AdvanceBy(1);
  // test.Print();
  
  
  const char* const filename = "dropfree3.mp3"; //"test.mp3";
  auto data = ReadAllBytes(filename);
  std::cout << data.size() << std::endl;

  constexpr int bufSiz = 1024*2*2;
  DataBuffer mp3Data{bufSiz};
  // fill databuffer
  int cnt = 0;
  for( int i=0; i< data.size(); ) {
    int size = bufSiz;
    if( cnt % 4 == 0 ) {
      size = 561;
    }
    if( i + size >= data.size() ) {
      size = data.size() - i;
    }
    OnlineMusicData omd;
    omd.data = {data.begin() + i, data.begin() + i + size};
    omd.siz = size;
    mp3Data.AddData( std::move(omd) );

    ++cnt;

    //std::cout << i << " " << size << std::endl;
    // if( cnt == 10 ) {
    //   break;
    // }

    i += size;
    if( i >= data.size() ) {
      break;
    }
  }

  //mp3Data.Print();

  static mp3dec_t mp3d;
  mp3dec_init(&mp3d);

  /*typedef struct
  {
      int frame_bytes;
      int channels;
      int hz;
      int layer;
      int bitrate_kbps;
  } mp3dec_frame_info_t;*/
  mp3dec_frame_info_t info;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

  std::vector<short> outData;
  
  int cnt2=0;
  while( true ) {
    ++cnt2;
    //if( cnt2 >= 827 ) {
      mp3Data.Print();
    //}
    unsigned char* input_buf = nullptr;
    int buf_size = mp3Data.GetNextPointer( 1024, input_buf );

    if( buf_size == 0 || input_buf == nullptr ) {
      std::cout << "eof! " << cnt2 << " " << mp3Data.Size() << ", " << mp3Data.Layers() << std::endl;
      break;
    }

    int samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
    std::cout << "samples=" << samples << ", frame_bytes=" << info.frame_bytes << std::endl;
    int used = info.frame_bytes;
    if( info.frame_bytes > 0  ) {
      mp3Data.AdvanceBy(used);
      if( samples > 0 ) {
        // valid wav data!
        std::cout << "decoded!" << std::endl;
        outData.insert( outData.end(), std::begin(pcm), std::begin(pcm) + samples );
      } else {
        std::cout << "skipped" << std::endl;
      }
    } else {
      std::cout << "done!" << std::endl;
      PrintInfo(info);
      break;
    }
  }
  

  std::ofstream fout("dropfree.raw" /*"test.raw"*/, std::ios::out | std::ios::binary);
  if( fout.is_open() ) {
    fout.write((const char*)outData.data(), 2*outData.size());
  }
  fout.close();

  // you must remove the data corresponding to the frame_bytes field from the input buffer before the next decoder invocation.


  // The decoding function returns the number of decoded samples. The following cases are possible:

  // 0: No MP3 data was found in the input buffer
  // 384: Layer 1
  // 576: MPEG 2 Layer 3
  // 1152: Otherwise
  // The following is a description of the possible combinations of the number of samples and frame_bytes field values:

  // More than 0 samples and frame_bytes > 0: Succesful decode
  // 0 samples and frame_bytes > 0: The decoder skipped ID3 or invalid data
  // 0 samples and frame_bytes == 0: Insufficient data

  return 0;
}

