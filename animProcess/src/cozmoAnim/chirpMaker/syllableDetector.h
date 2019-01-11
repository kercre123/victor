#ifndef __syllableDetector_h
#define __syllableDetector_h

#include <cstddef>
#include <vector>

namespace Anki {
namespace Util {
  template <typename T, size_t x>
  class FixedCircularBuffer;
}
namespace Vector {
    

class SyllableDetector
{
  using BuffType = short;
  using DataType = float;
public:
  explicit SyllableDetector( unsigned int sampleRate_Hz,
                             unsigned int windowLen,
                             unsigned int numOverlap,
                             unsigned int fftLen,
                             float stoppingThreshold_dB = 20.0f );
  ~SyllableDetector();
  
  struct SyllableInfo {
    float syllableTime_s = 0.0f;
    float startTime_s = 0.0f;
    float endTime_s = 0.0f;
    unsigned int startIdx;
    unsigned int endIdx;
    float avgFreq = 0.0f;
    float peakFreq = 0.0f;
    float avgPower = 0.0f;
  };
  
  std::vector<SyllableInfo> Run( const BuffType* signal, int signalLen );
  
private:
  
  void DumpSpectrogram( const std::vector<std::vector<float>>& spectrogram );
  
  float _sampleRate_Hz;
  unsigned int _windowLen;
  unsigned int _numOverlap;
  unsigned int _fftLen;
  //float _stoppingThreshold_dB;
  
  void* _plan;
  DataType* _inData;
  DataType* _outData;
  
  std::vector<DataType> _windowCoeffs;
  
  constexpr static unsigned int kFilterSize = 151;
  std::unique_ptr<Util::FixedCircularBuffer<BuffType,kFilterSize>> _filterBuffer;
  static std::array<float, kFilterSize> _filterCoeffs16;
  
};
    
  }
}

#endif
