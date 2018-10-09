/**
 * File: util/histogram/histogram.cpp
 *
 * Description: C++ wrapper for 3rd party hdr_histogram
 *
 **/

#include "histogram.h"
#include "hdr_histogram.h"
#include "util/logging/logging.h"

#include <cmath>

namespace Anki {
namespace Util {

Histogram::Histogram(int64_t lowest, int64_t highest, int significant_figures)
{
  hdr_init(lowest, highest, significant_figures, &_hdr);
  DEV_ASSERT(_hdr != nullptr, "Histogram.Constructor.InvalidHistogram");
}

Histogram::~Histogram()
{
  if (_hdr != nullptr) {
    hdr_close(_hdr);
    _hdr = nullptr;
  }
}

int64_t Histogram::GetMin() const
{
  DEV_ASSERT(_hdr != nullptr, "Histogram.GetMin.InvalidHistogram");
  return hdr_min(_hdr);
}

int64_t Histogram::GetMax() const
{
  DEV_ASSERT(_hdr != nullptr, "Histogram.GetMax.InvalidHistogram");
  return hdr_max(_hdr);
}

double Histogram::GetMean() const {
  DEV_ASSERT(_hdr != nullptr, "Histogram.GetMean.InvalidHistogram");
  if (_hdr->total_count > 0) {
    return hdr_mean(_hdr);
  }
  return std::nan("");
}


bool Histogram::Record(int64_t value)
{
  DEV_ASSERT(_hdr != nullptr, "Histogram.RecordValueInvalidHistogram");
  return hdr_record_value(_hdr, value);
}

} // namespace Util
} // namespace Anki
