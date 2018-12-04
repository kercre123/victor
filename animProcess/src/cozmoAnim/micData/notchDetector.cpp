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

  // This computes the average and minimum of two bands A and C that sandwich the notch band.
  // It then computes the average of the band B (where we expect a notch) and the number of powers
  // that exceed the minimum of either A or C. If the number of outliers is small, then there's likely
  // a notch there. If not, then we check the standard deviation of the power of B. When we cut out
  // a notch, that band is typically flat, whereas alexa's speech in region A is not flat. Thus, we compare
  // the std of B to that of A.
  // The hardest issue to overcome is that fricatives like S and X bleed into the area where we'd expect a notch.
  // This is especially important for the wake word, since aleXa will bleed into the notch region at the very
  // same moment we try to detect a notch. To deal with this, we're averaging over multiple PSDs, and
  // the standard deviation tricks helps with this a bit. Jordan also had an idea to replace the band stop
  // filter with a low pass filter, cutting off the top half of the spectrum we previously allowed
  // through (after the band stop).
  // (NOTE: currently anything to do with band C is disabled, since in testing, some normal signals from users
  // speaking had no power there)
  
  // All of these values are eyeballed from plots of PSD from mic input, using the response to the test phrases
  // "Whats the weather in [some location where she says "low of seventy ___"]"
  // "Whats your name" (be sure to disable kUsePlaybackRecognizer)
  // "Make a call"
  // "Set an alarm for 5pm"
  
  // notch band: centered at 5200 Hz with width 128 is ~ index 82. The width is half an octave, which is logarithmic, so
  // choose a higher width above the center.
  unsigned int startB = 78;
  unsigned int endB   = 88;
  
  // band A
  unsigned int startA = 62;
  unsigned int endA   = 72;
  
  float minA = 0.0f;
  float avgA = 0.0f;
  GetMinAndAvg( startA, endA, minA, avgA );
  
  float minC = 0.0f;
  float avgC = 0.0f;
  if( kDebugNotch ) {
    // band C: the width is tiny because the power may drop off because of our mic/speakers, and we don't want
    // the power dropoff to drop below the notch energy.
    unsigned int startC = 101;
    unsigned int endC   = 104;
    GetMinAndAvg( startC, endC, minC, avgC );
  }
  
  unsigned int numOutliers;
  float avgB = 0.0f;
  const float fakeAvgC = std::numeric_limits<float>::max(); // to disable band C
  GetOutliersAndAvg( startB, endB, avgA, fakeAvgC, numOutliers, avgB );
  
  // 2 or 3 seems to knock out areas where alexa is speaking but find no notch when alexa is not speaking.
  _hasNotch = (numOutliers <= 2);
  if( !_hasNotch || kDebugNotch ) {
    // also examine std of power in region B. if it's tiny compared to that of A, we likely cut a notch out there
    // NOTE: these are missing some factor of N-1 or N, but it doesn't matter here since it's relative
    float stdB = 0.0f;
    for( int i=startB; i<endB; ++i ) {
      float sumPower = 0.0f;
      for( int j=0; j<kNumToAvg; ++j ) {
        sumPower += _powers[j][i];
      }
      const float dp = (avgB - sumPower*kNumToAvgRecip);
      stdB += dp*dp;
    }
    float stdA = 0.0f;
    for( int i=startA; i<endA; ++i ) {
      float sumPower = 0.0f;
      for( int j=0; j<kNumToAvg; ++j ) {
        sumPower += _powers[j][i];
      }
      const float dp = (avgA - sumPower*kNumToAvgRecip);
      stdA += dp*dp;
    }
    LOG_INFO( "NotchDetector.HasNotch.CheckSTD",
              "stdB=%f, stdA=%f, hadnotch=%d",
              (avgB!=0.0f) ? stdB/(avgB*avgB) : -999.0,
              (avgA!=0.0f) ? stdA/(avgA*avgA) : -999.0,
              _hasNotch );
    
    if( !_hasNotch && (avgB != 0.0f) && (avgA != 0.0f) ) {
      const float b = stdB/(avgB*avgB);
      const float a = stdA/(avgA*avgA);
      const float factor = 1.6f; // eyeballed
      if( a > factor*b ) {
        _hasNotch = true;
      }
    }
  }
  
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
  if( kDebugNotch ) {
    // if kSaveNotches, useful to go check sIdx for why it did or didn't work
    LOG_INFO( "NotchDetector.HasNotch.Debug",
              "sIdx=%d, minA=%f, minC=%f, avgA=%f, avgC=%f, avgB=%f, outliers=%d, hasNotch=%d",
              sIdx-1, log10(minA), log10(minC), log10(avgA), log10(avgC), log10(avgB), numOutliers, _hasNotch );
  }
  return _hasNotch;
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NotchDetector::GetMinAndAvg(const size_t minIdx, const size_t maxIdx, float& minVal, float& avg)
{
  minVal = std::numeric_limits<float>::max();
  avg = 0.0f;
  for( size_t i=minIdx; i<maxIdx; ++i ) {
    float avgPower = 0.0f;
    for( int j=0; j<kNumToAvg; ++j ) {
      avgPower += _powers[j][i];
    }
    avgPower *= kNumToAvgRecip;
    if( avgPower < minVal ) {
      minVal = avgPower;
    }
    avg += avgPower;
  }
  if( avg > 0.0f ) {
    avg /= (maxIdx - minIdx + 1);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NotchDetector::GetOutliersAndAvg( const size_t minIdx,
                                       const size_t maxIdx,
                                       const float minVal1,
                                       const float minVal2,
                                       unsigned int& numOutliers,
                                       float& avg )
{
  numOutliers = 0;
  avg = 0.0f;
  const float minValTimesNum1 = kNumToAvg*minVal1;
  const float minValTimesNum2 = kNumToAvg*minVal2;
  for( size_t i=minIdx; i<maxIdx; ++i ) {
    float sumPower = 0.0f;
    for( int j=0; j<kNumToAvg; ++j ) {
      sumPower += _powers[j][i];
    }
    if( (sumPower > minValTimesNum1) || (sumPower > minValTimesNum2) ) {
      ++numOutliers;
    }
    avg += sumPower;
  }
  if( avg > 0.0f ) {
    avg /= kNumToAvg*(maxIdx - minIdx + 1);
  }
}
 
  
} // Vector
} // Anki
