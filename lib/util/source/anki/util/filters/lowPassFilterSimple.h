/**
 * File: lowPassFilterSimple
 *
 * Author: Matt Michini
 * Created: 02/27/2018
 *
 * Description: Implementation of a simple infinite impulse response
 *              first-order low pass filter (sometimes known as an
 *              exponentially weighted moving average). This filter
 *              can be used to smooth a noisy input signal, for example.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Util_Filters_LowPassFilterSimple_H__
#define __Util_Filters_LowPassFilterSimple_H__

namespace Anki {
namespace Util {

class LowPassFilterSimple
{
public:
  
  // Create a low-pass filter for signals that are read at a constant
  // rate (samplePeriod_sec). The filter coefficient is automatically
  // computed from the given time constant.
  //
  // samplePeriod_sec : The period at which you plan to call AddSample()
  // desiredTimeConstant_sec : The desired RC time constant of the filter (see
  //                           https://en.wikipedia.org/wiki/Time_constant). Using
  //                           a smaller time constant will cause the filtered value
  //                           to track more closely to the raw input (less 'smoothing').
  //                           A longer time constant will result in more 'smoothing'.
  LowPassFilterSimple(const float samplePeriod_sec,
                      const float desiredTimeConstant_sec);
  
  // Adds a new sample and returns the current filtered value.
  // Samples must only be added _once_ per sample period. If
  // this is not followed, the filtered value will be inaccurate.
  float AddSample(const float value);
  
  // Returns the current filtered value
  float GetFilteredValue() const;
  
  // Reset filter. Next call to AddSample() will re-initialize
  // the filter. Does not affect the sample period or desired
  // time constant (they are const).
  void Reset();
  
private:
  
  const float kFilterCoef;
  
  float _filteredValue = 0.f;
  bool _initialized = false;
};

} // namespace Util
} // namespace Anki

#endif // __Util_Filters_LowPassFilterSimple_H__
