#include "gtest/gtest.h"

#include <math.h>

#define private public
#include "cozmoAnim/micData/audioFFT.h"

using namespace Anki;
using namespace Anki::Vector;

TEST(AudioFFT, SineWave)
{
  auto checkPower = [](AudioFFT& fft) {
    const auto& power = fft.GetPower();
    ASSERT_EQ( power.size(), 256 );
    for( int i=0; i<power.size(); ++i ) {
      if( i == 10 ) {
        const float tol = 2e-5f;
        EXPECT_NEAR( power[i], 0.5f, tol ) << "index=" << i;
      } else {
        const float tol = 1e-7f;
        EXPECT_NEAR( power[i], 0.0f, tol ) << "index=" << i;
      }
    }
  };
  
  auto checkPowerWindowed = [](AudioFFT& fft) {
    const auto& power = fft.GetPower();
    ASSERT_EQ( power.size(), 256 );
    for( int i=0; i<power.size(); ++i ) {
      if( i == 10 ) {
        const float tol = .001f;
        EXPECT_NEAR( power[i], 0.125f, tol ) << "index=" << i;
      } else if( i==9 || i==11 ) {
        const float tol = .001f;
        EXPECT_NEAR( power[i], 0.031f, tol ) << "index=" << i;
      } else {
        const float tol = 1e-7f;
        EXPECT_NEAR( power[i], 0.0f, tol ) << "index=" << i;
      }
    }
  };
  
  constexpr int windowSize = 512;
  AudioFFT fft{windowSize};
  
  std::vector<float> sinWave;
  sinWave.reserve( windowSize );
  double step = (1.0 / windowSize);
  const double freq_Hz = 10.0;
  const float factor = std::numeric_limits<short>::max();
  for( int i=0; i<windowSize*2; ++i ) {
    const double t = step * i;
    const float value = sin( 2 * M_PI * freq_Hz * t ) * factor;
    const short shortVal = value;
    fft.AddSamples( &shortVal, 1 );
    
    const bool enough = fft.HasEnoughSamples();
    EXPECT_EQ( enough, i >= 511 );
    
    if( i == 511 ) {
      checkPowerWindowed(fft);
    }
  }
  // do it again at 1024, without windowing
  std::fill(fft._windowCoeffs.begin(), fft._windowCoeffs.end(), 1.0);
  checkPower(fft);
  
}

