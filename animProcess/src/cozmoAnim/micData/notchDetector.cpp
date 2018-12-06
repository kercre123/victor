/**
 * File: notchDetector.h
 *
 * Author: ross
 * Created: November 25 2018
 *
 * Description: Checks recently added power spectrums for a noticable notch around a specific band.
 *              The power is computed periodically during AddSamples (if requested), and HasNotch()
 *              will analyze average power using some ad-hoc rules.
 *              Use of the Sliding DFT algorithm might be useful here. In some quick benchmarks,
 *              the library used in audioFFT.h was still faster than my quick n dirty sliding DFT,
 *              but I've since changed the window size and period.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "cozmoAnim/micData/notchDetector.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"

// only needed for debugging
#include <cmath> // log10
#include <fstream>
#include <sstream>

namespace Anki {
namespace Vector {
  
namespace {
  // saves PSDs that dont contain a notch to a file
  CONSOLE_VAR(bool, kSaveNotches, "MicData", false);
  // Computes things to output
  CONSOLE_VAR(bool, kDebugNotch, "MicData", false);
  
  const float kNotchPower = -5.0f;
  
  #define LOG_CHANNEL "NotchDetector"
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NotchDetector::NotchDetector()
: _audioFFT{ 2*kNumPowers }
{ }
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NotchDetector::AddSamples( const short* samples, size_t numSamples, bool analyze )
{
  
  _audioFFT.AddSamples( samples, numSamples );
  _sampleIdx += numSamples;
  if( !analyze || !_audioFFT.HasEnoughSamples() ) {
    return;
  }
  
  // this may skip ffts if numSamples is larger than the period, but that's fine here, and it doesn't happen in
  // the this very specific ad-hoc scenario this class is designed for where numSamples is 80
  const size_t kPeriod = 320; // 50 ms (at 16kHz)
  if( _sampleIdx > kPeriod ) {
    
    _sampleIdx = _sampleIdx % kPeriod;
    
    _powers[_idx] = _audioFFT.GetPower();
    
    ++_idx;
    if( _idx >= _powers.size() ) {
      _hasEnoughData = true;
      _idx = 0;
    }
    _dirty = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool NotchDetector::HasNotch()
{
  if( !_hasEnoughData ) {
    return false;
  }
  if( !_dirty ) {
    return _hasNotch;
  }
  _dirty = false;
  
  static int sIdx = 0;
  if( kSaveNotches ) {
    std::ofstream fout("/data/data/com.anki.victor/cache/alexa/notch" + std::to_string(sIdx) + ".csv");
    ++sIdx;
    std::stringstream ss;
    for( size_t i=0; i<kNumPowers; ++i ) {
      ss << _powers[0][i] << ",";
      float avgPower = 0.0f;
      for( int j=0; j<kNumToAvg; ++j ) {
        avgPower += _powers[j][i];
      }
      avgPower *= kNumToAvgRecip;
      fout << log10(avgPower) << ",";
    }
    fout << std::endl;
    fout.close();
  }
  
  float sumPower = 0.0f;
  for( int i=3; i<=10; ++i ) {
    for( int j=0; j<kNumToAvg; ++j ) {
      sumPower += _powers[j][i];
    }
  }
  static const float minPower = powf(10, kNotchPower);
  const bool powerful = (sumPower >= minPower); // i.e., log10(sumPower) >= kNotchPower;
  
  if( kDebugNotch ) {
    // if kSaveNotches, useful to go check sIdx for why it did or didn't work
    LOG_INFO( "NotchDetector.HasNotch.Debug",
              "Idx=%d, Power=%f, HUMAN=%d",
              sIdx, log10(sumPower), powerful );
  }
  
  return !powerful;
}
 
  
} // Vector
} // Anki
