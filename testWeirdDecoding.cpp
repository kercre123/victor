// todo:? #define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "animProcess/src/cozmoAnim/minimp3.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cassert>

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
  const std::string filename = "sports.mp3"; // "freshing.mp3";// "dropfree3.mp3"; //"test.mp3";
  auto data = ReadAllBytes(filename.c_str());
  std::cout << data.size() << std::endl;

  int cursor = 0;
  
  static mp3dec_t mp3d;
  mp3dec_init(&mp3d);

  
  mp3dec_frame_info_t info;
  short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

  std::vector<short> outData;
  
  int cnt2=0;
  while( true ) {
    ++cnt2;
    unsigned char* input_buf = (unsigned char*)data.data() + cursor;
    int buf_size = (int)data.size() - cursor;

    if( buf_size == 0 || input_buf == nullptr ) {
      std::cout << "eof! " << buf_size << " " << (input_buf == nullptr) << " cursor=" << cursor << std::endl;
      break;
    }

    int samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
    //std::cout << "samples=" << samples << ", frame_bytes=" << info.frame_bytes << std::endl;
    if( cnt2 == 1 ) {
      PrintInfo(info);
    }
    int consumed = info.frame_bytes;
    if( consumed > 0  ) {
      cursor += consumed;
      if( samples > 0 ) {
        // valid wav data!

        // if( info.channels == 2 ) {
        //   // audio engine only supports mono
        //   int j = 0;
        //   for( int i=0; i<samples; i += 2, j += 1) {
        //     short left = pcm[i];
        //     short right = pcm[i + 1];
        //     short monoSample = (int(left) + right) / 2;
        //     pcm[j] = monoSample;
        //   }
        //   samples = samples/2;
        //   info.channels = 1;
        // }


        //std::cout << "decoded!" << std::endl;
        outData.insert( outData.end(), std::begin(pcm), std::begin(pcm) + 2*samples );
      } else {
        std::cout << "skipped" << std::endl;
      }
    } else {
      std::cout << "done!" << std::endl;
      PrintInfo(info);
      break;
    }
  }
  
  std::ofstream fout(filename + ".raw" /*"dropfree.raw"*/ /*"test.raw"*/, std::ios::out | std::ios::binary);
  if( fout.is_open() ) {
    fout.write((const char*)outData.data(), sizeof(short)*outData.size());
  }
  fout.close();

  return 0;
}

