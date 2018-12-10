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
#include "kissfft/kiss_fftr.h"
#include <math.h>

namespace Anki {
namespace Vector {

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioFFT::AudioFFT( unsigned int N )
: _N{ N }
, _buff{ N, N }
, _windowCoeffs(N, 0.0)
{
  Reset();
  
  // hann window coefficients
  for( int i=0; i<_N/2; i++ ) {
    DataType value = (1.0 - cos(2.0 * M_PI * i/(_N-1))) * 0.5;
    _windowCoeffs[i] = value;
    _windowCoeffs[_N-1-i] = value; // window is symmetric
  }
  
  static_assert( std::is_same<kiss_fft_scalar,float>::value, "" ); // not using fixed point
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
  kiss_fft_cpx* out = (kiss_fft_cpx*)_outData;
  
  static const DataType normFactor = 1.0 / (_N*_N);
  ret.push_back( normFactor*(out[0].r*out[0].r + out[0].i*out[0].i) );
  const int last = _N/2;
  for( int i=1; i<last; ++i ) {
    const DataType mag = out[i].r*out[i].r + out[i].i*out[i].i;
    ret.push_back( 2*normFactor*mag );
  }
  ret.push_back( normFactor*(out[last].r*out[last].r + out[last].i*out[last].i) );
  
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::Reset()
{
  Cleanup();

  _inData = new DataType[ _N ];
  _outData = new kiss_fft_cpx[ _N ];
  
  _plan = (void*) kiss_fftr_alloc( _N,0,0,0 );
  
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
  kiss_fftr( (kiss_fftr_cfg)_plan, _inData, (kiss_fft_cpx*)_outData );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioFFT::Cleanup()
{
  if( _inData ) {
    delete [] _inData;
    _inData = nullptr;
  }
  if( _outData ) {
    delete [] (kiss_fft_cpx*)_outData;
    _outData = nullptr;
  }
  if( _plan != nullptr ) {
    free( _plan );
    _plan = nullptr;
  }
}

} // namespace Vector
} // namespace Anki
