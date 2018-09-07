// todo:? #define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "animProcess/src/cozmoAnim/minimp3.h"
#include "animProcess/src/cozmoAnim/audioDataBuffer.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cassert>
#include <deque>

using namespace Anki::Vector;

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



int main()
{
  // AudioDataBuffer buffer{15};
  // std::vector<unsigned char> tmp = {0,1,2,3,4,5,6};
  // buffer.AddData( tmp.data(), tmp.size() );
  // std::cout << (buffer.ReadData( 6 ) == nullptr) << std::endl;
  // std::cout << (buffer.ReadData( 7 ) == nullptr) << std::endl;
  // std::cout << (buffer.ReadData( 8 ) == nullptr) << std::endl;
  // std::cout << "Advancing: " << buffer.AdvanceCursor( 5 ) << std::endl;
  // std::cout << (buffer.ReadData( 1 ) == nullptr) << std::endl;
  // std::cout << (buffer.ReadData( 2 ) == nullptr) << std::endl;
  // std::cout << (buffer.ReadData( 3 ) == nullptr) << std::endl;
  // buffer.AddData( tmp.data(), tmp.size() );
  // buffer.AddData( tmp.data(), tmp.size() );
  // buffer.AddData( tmp.data(), tmp.size() );
  // std::cout << (buffer.ReadData( 3 ) == nullptr) << std::endl;
  
  // return 0;
  const char* const filename = "dropfree3.mp3"; //"test.mp3";
  auto data = ReadAllBytes(filename);
  std::cout << data.size() << std::endl;

  //constexpr int bufSiz = 1024*2*2;
  //DataBuffer mp3Data{bufSiz};
  AudioDataBuffer buffer{1<<20, 1014};
  // fill databuffer
  int cnt = 0;
  for( int i=0; i< data.size(); ) {
    int size = 1024;
    if( cnt % 4 == 0 ) {
      size = 561;
    }
    if( i + size >= data.size() ) {
      size = data.size() - i;
    }
    std::vector<unsigned char> vec = {data.begin() + i, data.begin() + i + size};
    
    buffer.AddData( vec.data(), size );

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
      //mp3Data.Print();
    //}
    int buf_size = 1024;
    unsigned char* input_buf = buffer.ReadData( buf_size );

    if( buf_size == 0 || input_buf == nullptr ) {
      std::cout << "eof! " << std::endl;
      break;
    }

    int samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
    std::cout << "samples=" << samples << ", frame_bytes=" << info.frame_bytes << std::endl;
    if( cnt2 == 1 ) {
      PrintInfo(info);
    }
    int used = info.frame_bytes;
    if( info.frame_bytes > 0  ) {
      const bool moved = buffer.AdvanceCursor(used);
      assert(moved);
      if( samples > 0 ) {
        // valid wav data!

        if( info.channels == 2 ) {
          // audio engine only supports mono
          int j = 0;
          for( int i=0; i<samples; i += 2, j += 1) {
            short left = pcm[i];
            short right = pcm[i + 1];
            short monoSample = (int(left) + right) / 2;
            pcm[j] = monoSample;
          }
          samples = samples/2;
          info.channels = 1;
        }


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

