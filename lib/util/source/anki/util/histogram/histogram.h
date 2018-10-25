/**
 * File: util/histogram/histogram.h
 *
 * Description: C++ wrapper for third-party hdr_histogram
 *
 **/

#ifndef __util_histogram_histogram_h
#define __util_histogram_histogram_h

#include <cstdint>

// Forward declarations
struct hdr_histogram;

namespace Anki {
namespace Util {

class Histogram
{
  public:
    // Initialize histogram to track value between lowest and highest.
    // Significant_figures describes the level of precision for this histogram, i.e. the number
    // of figures in a decimal number that will be maintained.  E.g. a value of 3 will mean
    // the results from the histogram will be accurate up to the first three digits.
    //
    // Note lowest must be > 0.
    // Note signficant_figures must be a value between 1 and 5 (inclusive).
    Histogram(int64_t lowest, int64_t highest, int significant_figures);

    // Release resources owned by this histogram
    ~Histogram();

    // Record a value in the histogram, rounded to a precision at or better than
    // significant_figures specified at construction time.
    // Return false if the value is larger than highest trackable value and can't be recorded,
    // else true.
    bool Record(int64_t value);

    // Return minimum value from histogram data or INT64_MAX if histogram is empty
    int64_t GetMin() const;

    // Return maximum value from histogram data or 0 if histogram is empty
    int64_t GetMax() const;

    // Return mean value from histogram data or NaN if histogram is empty
    double GetMean() const;

  private:
    struct hdr_histogram * _hdr = nullptr;

};


} // namespace Util
} // namespace Anki

#endif // __util_histogram_histogram_h
