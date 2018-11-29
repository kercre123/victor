/**
 * File: audioFFT.h
 *
 * Author: ross
 * Created: November 25 2018
 *
 * Description: Vector wrapper for a coretech-external FFT library.
 *              This is not a templated class (on, say, the window length) so that the
 *              library methods are not in the header
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/micData/audioFFT.h"
#include "util/logging/logging.h"
#include "fftw3.h"
#include <math.h>

namespace Anki {
namespace Vector {
  
// this library has a feature where it will do some quick FFTs upon init to choose the best algorithm.
// this means that subsequent FFTs would non-deterministic. If you require deterministic outcomes or
// don't care as much about performance, set this to 1.
#define FFTW_DETERMINISTIC 0

// aliases for library methods and types
#if FFTW_SINGLE
  using FFTW_plan = fftwf_plan;
  const auto& FFTW_alloc_plan = fftwf_plan_r2r_1d;
  const auto& FFTW_alloc_real = fftwf_alloc_real;
  const auto& FFTW_free = fftwf_free;
  const auto& FFTW_destroy_plan = fftwf_destroy_plan;
  const auto& FFTW_execute = fftwf_execute;
#else
  using FFTW_plan = fftw_plan;
  const auto& FFTW_alloc_plan = fftw_plan_r2r_1d;
  const auto& FFTW_alloc_real = fftw_alloc_real;
  const auto& FFTW_free = fftw_free;
  const auto& FFTW_destroy_plan = fftw_destroy_plan;
  const auto& FFTW_execute = fftw_execute;
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioFFT::AudioFFT( unsigned int N )
: _N{ N }
, _buff{ N, N }
{
  Reset();
  
  // hann window coefficients
  _windowCoeffs.resize( _N, 0.0 );
  for( int i=0; i<_N/2; i++ ) {
    DataType value = (1.0 - cos(2.0 * M_PI * i/(_N-1))) * 0.5;
    _windowCoeffs[i] = value;
    _windowCoeffs[_N-i] = value; // window is symmetric
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioFFT::~AudioFFT()
{
  Cleanup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::AddSamples( const BuffType* samples, size_t numSamples )
{
  size_t available = _buff.Capacity() - _buff.Size();
  if( numSamples > available ) {
    _buff.AdvanceCursor( (unsigned int)(numSamples - available) );
  }
  const size_t numAdded = _buff.AddData( samples, (unsigned int) numSamples );
  DEV_ASSERT( numAdded == numSamples, "AudioFFT.AddSamples.CouldNotAdd" );
  
  if( !_hasEnoughSamples ) {
    _hasEnoughSamples = _buff.Size() >= _N;
  }
  
  _dirty |= (numSamples > 0);
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<AudioFFT::DataType> AudioFFT::GetPower()
{
  std::vector<DataType> ret;
  
  if( !_hasEnoughSamples ) {
    return ret;
  }
  
  ret.reserve( _N/2 + 1 );
  
  // do dft if needed
  DoDFT();
  
  // compute power from _outData
  static const DataType normFactor = 1.0 / (_N*_N);
  ret.push_back( _outData[0]*_outData[0]*normFactor );
  for( int i=1; i<_N/2; ++i ) {
    const DataType mag = _outData[i]*_outData[i] + _outData[_N-i]*_outData[_N-i];
    ret.push_back( 2*normFactor*mag );
  }
  ret.push_back( normFactor * _outData[_N/2]*_outData[_N/2] );
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::Reset()
{
  Cleanup();

  // these input and output arrays can't be on the stack because we use a library
  // method to generate aligned arrays. Ideally our backing buffer RingBuffContiguousRead
  // would have aligned floats, but since it doesn't, we need to copy into _inData for
  // any DFT (if the input has changed).
  _inData = FFTW_alloc_real( _N );
  _outData = FFTW_alloc_real( _N );
  
#if FFTW_DETERMINISTIC
  const auto flags = FFTW_ESTIMATE;
#else
  const auto flags = FFTW_MEASURE;
#endif
  _plan = (void*)FFTW_alloc_plan( _N, _inData, _outData, FFTW_R2HC, flags);
  
  _hasEnoughSamples = false;
  _dirty = false;
  _buff.Reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::DoDFT()
{
  if( !_dirty ) {
    return;
  }
  _dirty = false;
  
  // copy into aligned memory. note that _inData may get modified when the _plan is executed
  const BuffType* buffData = _buff.ReadData( _N );
  assert( buffData != nullptr );
  static const DataType factor = 1.0 / std::numeric_limits<BuffType>::max();
  for( int i=0; i<_N; ++i ) {
    const DataType fVal = *(buffData + i) * factor;
    _inData[i] = _windowCoeffs[i] * fVal;
  }
  FFTW_execute( (FFTW_plan)_plan );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::Cleanup()
{
  if( _inData ) {
    FFTW_free( _inData );
    _inData = nullptr;
  }
  if( _outData ) {
    FFTW_free( _outData );
    _outData = nullptr;
  }
  if( _plan != nullptr ) {
    FFTW_destroy_plan( (FFTW_plan)_plan );
    _plan = nullptr;
  }
}

} // namespace Vector
} // namespace Anki
