#ifndef __AnimProcess_CozmoAnim_FFTTypes_H__
#define __AnimProcess_CozmoAnim_FFTTypes_H__

// Members for managing the results of async FFT processing
struct FFTResult {
  uint32_t mostProminentFreq_hz;
  float loudness;
};
using FFTResultList = std::vector<FFTResult>;

#endif
