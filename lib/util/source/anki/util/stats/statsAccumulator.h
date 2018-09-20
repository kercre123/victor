/**
 * File: statsAccumulator
 *
 * Author: bneuman (brad)
 * Created: 2013-04-01
 *
 * Description: Simple class that computes stats for a running sample
 *              of doubles
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#ifndef BASESTATION_GENERAL_STATS_ACCUMULATOR_H_
#define BASESTATION_GENERAL_STATS_ACCUMULATOR_H_

#include <float.h>
#include <math.h>

namespace Anki {
namespace Util {
namespace Stats {

class StatsAccumulator
{
public:
  StatsAccumulator() {
    Clear();
  }

  StatsAccumulator& operator+=(const double v) {
    val_ += v;
    num_ += 1.0;

    if(v > max_)
      max_ = v;

    if(v < min_)
      min_ = v;

    akm1_ = ak_;
    ak_ = akm1_ + (v - akm1_) / num_;

    qk_ = qk_ + (v - akm1_) * (v - ak_);


    return *this;
  }

  double GetVal() const {return val_;};
  double GetMean() const {return ak_;};
  double GetStd() const {return sqrt(qk_/num_) ;}; // running standard deviation formula
  double GetVariance() const {return qk_/num_; };
  double GetMin() const {return min_;};
  double GetMax() const {return max_;};
  double GetMinSafe() const {return (num_ > 0.0) ? min_ : 0.0;}; // otherwise it'll be +DBL_MAX
  double GetMaxSafe() const {return (num_ > 0.0) ? max_ : 0.0;}; // otherwise it'll be -DBL_MAX
  double GetNumDbl() const { return num_; }
  
  int GetNum() const {return (int)round(num_);};
  int GetIntVal() const {return (int)round(val_);};
  int GetIntMax() const {return (int)round(max_);};
  int GetIntMin() const {return (int)round(min_);};
  int GetIntMean() const {return (int)round(ak_);};

  void Clear() {
    val_ = 0.0;
    max_ = -DBL_MAX;
    min_ = DBL_MAX;
    num_ = 0;
    ak_ = 0;
    akm1_ = 0;
    qk_ = 0;
  }

protected:
  double val_, max_, min_;
  double num_;

  // running std deviation variables
  double ak_, akm1_, qk_;
};

} // end namespace Stats
} // end namespace Util
} // end namespace Anki

#endif
