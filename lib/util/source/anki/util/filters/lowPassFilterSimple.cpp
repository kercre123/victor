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

#include "util/filters/lowPassFilterSimple.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Util {

LowPassFilterSimple::LowPassFilterSimple(const float samplePeriod_sec,
                                         const float desiredTimeConstant_sec)
: kFilterCoef(samplePeriod_sec / (desiredTimeConstant_sec + samplePeriod_sec))
{
  // For details about the above computation of the filter coefficient see
  // https://en.wikipedia.org/wiki/Low-pass_filter#Simple_infinite_impulse_response_filter
  
  DEV_ASSERT(samplePeriod_sec > 0, "LowPassFilterSimple.LowPassFilterSimple.InvalidSamplePeriod");
  DEV_ASSERT(desiredTimeConstant_sec > 0, "LowPassFilterSimple.LowPassFilterSimple.InvalidTimeConstant");
}


float LowPassFilterSimple::AddSample(const float value)
{
  if (!_initialized) {
    _filteredValue = value;
    _initialized = true;
  } else {
    _filteredValue = kFilterCoef * value + (1 - kFilterCoef) * _filteredValue;
  }
  
  return _filteredValue;
}


float LowPassFilterSimple::GetFilteredValue() const
{
  DEV_ASSERT(_initialized, "LowPassFilterSimple.GetFilteredValue.NotInitialized");
  return _filteredValue;
}


void LowPassFilterSimple::Reset()
{
  _filteredValue = 0.f;
  _initialized = false;
}

} // namespace Util
} // namespace Anki

