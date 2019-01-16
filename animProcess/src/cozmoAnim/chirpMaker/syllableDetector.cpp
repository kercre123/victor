#include "syllableDetector.h"
#include "pffft.h"
#include "util/logging/logging.h"
#include "util/container/fixedCircularBuffer.h"
#include <vector>
#include <map>
#include <fstream>
#include <math.h>
#include <iostream>
#include <cassert>
#include <array>

#include "util/helpers/ankiDefines.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Vector {
  
  namespace {
    CONSOLE_VAR_RANGED(float, kStoppingThreshold, "Chirps", 30.0f, 0.0f, 50.0f);
  }
  
// ---------------------------------------------------------------------------
// zeroethOrderBessel
//  GPLd!!!!!
// https://github.com/johnglover/simpl/blob/master/src/loris/KaiserWindow.C
//
static double ZerothOrderBessel( double x )
{
  const double eps = 0.000001;
  
  //  initialize the series term for m=0 and the result
  double besselValue = 0;
  double term = 1;
  double m = 0;
  
  //  accumulate terms as long as they are significant
  while(term  > eps * besselValue)
  {
    besselValue += term;
    
    //  update the term
    ++m;
    term *= (x*x) / (4*m*m);
  }
  
  return besselValue;
}

//  GPLd!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! Build a new Kaiser analysis window having the specified shaping
//! parameter. See Oppenheim and Schafer:  "Digital Signal Processing"
//! (1975), p. 452 for further explanation of the Kaiser window. Also,
//! see Kaiser and Schafer, 1980.
//!
//! \param      win is the vector that will store the window
//!             samples. The number of samples computed will be
//!             equal to the length of this vector. Any previous
//!             contents will be overwritten.
//! \param      shape is the Kaiser shaping parameter, controlling
//!             the sidelobe rejection level.
//
void BuildKaiserWindow( std::vector<float> & win, double shape = 0.5 )
{
    //  Pre-compute the shared denominator in the Kaiser equation.
    const double oneOverDenom = 1.0 / ZerothOrderBessel( shape );
  
    const unsigned int N = static_cast<unsigned int>(win.size() - 1);
    const double oneOverN = 1.0 / N;
  
    for ( unsigned int n = 0; n <= N; ++n )
    {
        const double K = (2.0 * n * oneOverN) - 1.0;
        const double arg = sqrt( 1.0 - (K * K) );
      
        win[n] = ZerothOrderBessel( shape * arg ) * oneOverDenom;
    }
}
  

template < int N, int sampleRate >
std::array<float, N> ComputeFilterCoeffs()
{
  const float lowCutoff = 50.0f;
  const float highCutoff = 300.0f;
  static_assert( N % 2, "N must be odd" );

  std::array<float, N> filterCoeffs;
  // http://digitalsoundandmusic.com/7-3-2-low-pass-high-pass-bandpass-and-bandstop-filters/
  const float f1 = lowCutoff/sampleRate;
  const float f2 = highCutoff/sampleRate;
  const float omega1 = 2*M_PI_F*f1;
  const float omega2 = 2*M_PI_F*f2;
  const int middle = N/2;
  for( int i=-N/2; i<=N/2; ++i ) {
    if (i == 0) {
      filterCoeffs[middle] = 2*(f2 - f1);
    } else {
      filterCoeffs[i + middle] = sinf(omega2*i)/(M_PI_F*i) - sinf(omega1*i)/(M_PI_F*i);
    }
  }
  return filterCoeffs;
}
std::array<float, SyllableDetector::kFilterSize> SyllableDetector::_filterCoeffs16
  = ComputeFilterCoeffs<SyllableDetector::kFilterSize,16000>();


SyllableDetector::SyllableDetector( unsigned int sampleRate_Hz,
                                    unsigned int windowLen,
                                    unsigned int numOverlap,
                                    unsigned int fftLen,
                                    float stoppingThreshold_dB )
: _sampleRate_Hz{ static_cast<float>(sampleRate_Hz) }
, _windowLen{ windowLen }
, _numOverlap{ numOverlap }
, _fftLen{ fftLen }
//, _stoppingThreshold_dB{ stoppingThreshold_dB }
, _filterBuffer{ std::make_unique<Anki::Util::FixedCircularBuffer<BuffType, kFilterSize>>() }
{
  
  assert( (_fftLen & (_fftLen - 1)) == 0); // "Should be power of 2 for efficiency"
  assert( _fftLen >= _windowLen );
  assert( _numOverlap < _windowLen );
  
  _plan = (void*) PFFFT::pffft_new_setup( _fftLen, PFFFT::PFFFT_REAL );
  
  int numBytes = _fftLen * sizeof(float);
  _inData = (float*) PFFFT::pffft_aligned_malloc( numBytes );
  _outData = (float*) PFFFT::pffft_aligned_malloc( numBytes );
  
  
  // window func uses _windowLen instead of _fftLen since window only affects actual data, not zero padding region
  _windowCoeffs.resize( _windowLen, 0.0 );
  if( false ) {
    BuildKaiserWindow( _windowCoeffs );
  } else {
    // Hann window
    for( int i=0; i<_windowLen/2; i++ ) {
      DataType value = (1.0 - cos(2.0 * M_PI * i/(_windowLen-1))) * 0.5;
      _windowCoeffs[i] = value;
      _windowCoeffs[_windowLen-i-1] = value; // window is symmetric
    }
  }
}

SyllableDetector::~SyllableDetector()
{
  if( _plan != nullptr ) {
    PFFFT::pffft_destroy_setup( (PFFFT::PFFFT_Setup*)_plan );
    _plan = nullptr;
  }
  if( _inData ) {
    PFFFT::pffft_aligned_free( (void*)_inData );
    _inData = nullptr;
  }
  if( _outData ) {
    PFFFT::pffft_aligned_free( (void*)_outData );
    _outData = nullptr;
  }
}

std::vector<SyllableDetector::SyllableInfo> SyllableDetector::Run( const SyllableDetector::BuffType* signal, int signalLen )
{
  std::vector<BuffType> filteredSignal;
  filteredSignal.resize(signalLen);
  if( true ) {
    for( int i=0; i<signalLen; ++i ) {
      _filterBuffer->push_front( (float)signal[i] );
      float value = 0.0f;
      for( int j=0; j<_filterBuffer->size(); ++j ) {
        value += _filterCoeffs16[j] * (*_filterBuffer)[j];
      }
      filteredSignal[i] = (short)value;
    }
  } else {
    std::copy( signal, signal + signalLen, filteredSignal.begin());
  }
  
  // S is matrix of freq by time
  // T is vector of times
  // F is vector of frequencies
  std::vector< std::vector<float> > spectrogram;
  
  unsigned int numWindows = signalLen / (_windowLen - _numOverlap);
  unsigned int numFreqs = _fftLen / 2;
  
  std::vector<float> spectrogramTimes;
  spectrogramTimes.reserve( numWindows );
  
  std::vector<float> spectrogramFreqs;
  spectrogramFreqs.reserve( numFreqs );
  for( int i=0; i<numFreqs; ++i ) {
    spectrogramFreqs.push_back( 0.5f*(i * _sampleRate_Hz) / numFreqs );
  }
  
  spectrogram.reserve(numWindows);
  for( int idxWindow=0; idxWindow<numWindows; ++idxWindow ) {
    std::vector<float> result;
    result.reserve( numFreqs );
    
    const unsigned int offset = idxWindow*(_windowLen - _numOverlap);
    spectrogramTimes.push_back( offset/_sampleRate_Hz );
    
    // copy into aligned memory (so many copies this may not be cheaper)
    const BuffType* start = filteredSignal.data() + offset;
    static const DataType factor = 1.0 / std::numeric_limits<BuffType>::max();
    for( int i=0; i<_windowLen; ++i ) {
      const DataType fVal = *(start + i) * factor;
      _inData[i] = _windowCoeffs[i] * fVal;
    }
    // zero pad
    for( int i=_windowLen; i<_fftLen; ++i ) {
      _inData[i] = 0.0;
    }
    
    // pffft docs say: "If 'work' is NULL, then stack will be used instead (this is probably the
    // best strategy for small FFTs, say for N < 16384)."
    float* work = nullptr;
    PFFFT::pffft_transform_ordered( (PFFFT::PFFFT_Setup*)_plan, _inData, _outData, work, PFFFT::PFFFT_FORWARD );
    
    const unsigned int N = _fftLen;
    // compute power from _outData. real and imag components are interleaved
    static const DataType normFactor = 1.0f / (N*N);
    result.push_back( (_outData[0]*_outData[0] + _outData[1]*_outData[1])*normFactor );
    for( int i=2; i<N; i+=2 ) {
      const DataType mag = _outData[i]*_outData[i] + _outData[i+1]*_outData[i+1];
      result.push_back( 2*normFactor*mag );
    }
    
    spectrogram.push_back( std::move(result) );
  }
  
  //DumpSpectrogram( spectrogram );
  
  if( spectrogram.empty() ) {
    PRINT_NAMED_WARNING("WHATNOW", "spectrogram empty!");
    // jic
    return {};
  }
  
  
  
  //  mag = abs(S); ????????????
  
  std::map<float,SyllableInfo> syllables;
  
  // in matlab, each column is a different time slice! "Each column of s contains an estimate of the short-term, time-localized frequency content of x."
  
  float cutoff = 0.0f;
  bool first = true;
  std::vector<bool> usedTimes;
  usedTimes.resize(spectrogramTimes.size(), false);
  
  while( true ) {
    // find the maximum remaining magnitude in the spectrogram (scanning over time)
    // this should probably be done with a data structure from above
    unsigned int maxPowerFreqIdx = 0;
    unsigned int maxPowerTimeIdx = 0;
    float maxPower = 0.0f;
    std::vector<float> maxPowerAtTime;
    std::vector<int> freqIdxMaxPowerAtTime;
    maxPowerAtTime.resize(spectrogram.size(), -std::numeric_limits<float>::max());
    freqIdxMaxPowerAtTime.resize(spectrogram.size(), -1);
    for( int iTime=0; iTime<spectrogram.size(); ++iTime ) {
      if( usedTimes[iTime] ) {
        continue;
      }
      float timeMax = 0.0f;
      int timeMaxIdx = 0;
      for( int iFreq=0; iFreq<spectrogram[iTime].size(); ++iFreq ) {
        if( maxPower <= spectrogram[iTime][iFreq] ) {
          maxPower = spectrogram[iTime][iFreq];
          maxPowerTimeIdx = iTime;
          maxPowerFreqIdx = iFreq;
        }
        if( timeMax <= spectrogram[iTime][iFreq] ) {
          timeMax = spectrogram[iTime][iFreq];
          timeMaxIdx = iFreq;
        }
      }
      maxPowerAtTime[iTime] = 20*log10(timeMax);
      freqIdxMaxPowerAtTime[iTime] = timeMaxIdx;
    }
    
    float amp = 20*log10(maxPower);
    
    if( first ) {
      // store the cutoff value
      cutoff = amp - kStoppingThreshold; // _stoppingThreshold_dB;
      first = false;
    }
    
    // Is it time to stop looking for syllables?
    if( amp < cutoff ) {
      break;
    }
    
    const float minAmp = amp - kStoppingThreshold;//_stoppingThreshold_dB;
    
    SyllableInfo info;
    info.syllableTime_s = spectrogramTimes[maxPowerTimeIdx];
    int count = 0;
    
    float maxPowerHere = -std::numeric_limits<float>::max();
    for( int i=maxPowerTimeIdx; i>=0; --i ) {
      if( maxPowerAtTime[i] < minAmp ) {
        break;
      }
      info.startTime_s = spectrogramTimes[i];
      info.startIdx = i*(_windowLen - _numOverlap);
      if( !ANKI_VERIFY( i<freqIdxMaxPowerAtTime.size(), "A0", "%d %zu", i, freqIdxMaxPowerAtTime.size()) ) {
        return {};
      }
      if( !ANKI_VERIFY( freqIdxMaxPowerAtTime[i]<spectrogramFreqs.size(), "B0", "%d %zu", freqIdxMaxPowerAtTime[i],spectrogramFreqs.size()) ) {
        return {};
      }
      const float freq = spectrogramFreqs[freqIdxMaxPowerAtTime[i]];
      info.avgFreq += freq;
      info.avgPower += maxPowerAtTime[i];
      info.firstFreq = freq;
      if( maxPowerAtTime[i] > maxPowerHere ) {
        info.peakFreq = freq;
      }
      usedTimes[i] = true;
      ++count;
    }
    
    info.endIdx = maxPowerTimeIdx*(_windowLen - _numOverlap) + _windowLen;
    info.endTime_s = spectrogramTimes[maxPowerTimeIdx];
    for( int i=maxPowerTimeIdx+1; i<=maxPowerAtTime.size(); ++i ) {
      if( maxPowerAtTime[i] < minAmp ) {
        break;
      }
      info.endTime_s = spectrogramTimes[i];
      info.endIdx = i*(_windowLen - _numOverlap) + _windowLen;
      if( !ANKI_VERIFY( i<freqIdxMaxPowerAtTime.size(), "A1", "%d %zu", i, freqIdxMaxPowerAtTime.size()) ) {
        return {};
      }
      if( !ANKI_VERIFY( freqIdxMaxPowerAtTime[i]<spectrogramFreqs.size(), "B1", "%d %zu", freqIdxMaxPowerAtTime[i],spectrogramFreqs.size()) ) {
        return {};
      }
      const float freq = spectrogramFreqs[freqIdxMaxPowerAtTime[i]];
      info.avgFreq += freq;
      info.avgPower += maxPowerAtTime[i];
      info.lastFreq = freq;
      if( maxPowerAtTime[i] > maxPowerHere ) {
        info.peakFreq = freq;
      }
      usedTimes[i] = true;
      ++count;
    }
    
    info.avgFreq /= count;
    info.avgPower /= count;
    
    syllables.emplace( info.startTime_s, std::move(info) );
  }
  
  std::vector<SyllableInfo> result;
  result.reserve(syllables.size());
  for( const auto& syllable : syllables ) {
    PRINT_NAMED_WARNING("WHATNOW", "syllable from %f to %f, avgFreq=%f, peakFreq=%f, pwr=%f, firstFreq=%f, lastFreq=%f", syllable.second.startTime_s, syllable.second.endTime_s, syllable.second.avgFreq, syllable.second.peakFreq, syllable.second.avgPower, syllable.second.firstFreq, syllable.second.lastFreq);
    //std::cout << "syllable from " << syllable.second.startTime_s << " to " << syllable.second.endTime_s << ", avgFreq=" << syllable.second.avgFreq << ", peakfreq=" << syllable.second.peakFreq << ", pwr=" << syllable.second.avgPower  << std::endl;
    result.push_back( std::move( syllable.second ) );
  }
  
  return result;
}

void SyllableDetector::DumpSpectrogram( const std::vector<std::vector<float>>& spectrogram )
{
#if defined(ANKI_PLATFORM_VICOS)
  std::ofstream outFile("/data/data/com.anki.victor/cache/spectrogram.csv");
#else
  std::ofstream outFile("spectrogram.csv");
#endif
  for( int i=0; i<spectrogram.size(); ++i ) {
    for( int j=0; j<spectrogram[i].size(); ++j ) {
      outFile << spectrogram[i][j] << ",";
    }
    outFile << std::endl;
  }
  outFile.close();
}
  
  
}
}
