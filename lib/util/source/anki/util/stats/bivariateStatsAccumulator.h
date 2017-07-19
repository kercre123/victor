/**
 * File: statsAccumulator
 *
 * Author: ross
 * Created: 2017-06-23
 *
 * Description: same as StatsAccumulator but for bivariate samples
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Util_Stats_BivariateStatsAccumulator_H__
#define __Util_Stats_BivariateStatsAccumulator_H__

#include "util/math/math.h"


namespace Anki {
namespace Util {
namespace Stats {
  
  
class BivariateStatsAccumulator
{
public:
  BivariateStatsAccumulator() {
    Clear();
  }
  
  BivariateStatsAccumulator& Accumulate(float x, float y) {
    
    _minx = Anki::Util::Min(x,_minx);
    _miny = Anki::Util::Min(y,_miny);
    _maxx = Anki::Util::Max(x,_maxx);
    _maxy = Anki::Util::Max(y,_maxy);
    
    
    // welford's method
    _n += 1.0f;
    float xdiff = x - _meanx;
    float ydiff = y - _meany;
    float recip = 1.0f/_n;
    _meanx += xdiff * recip;
    _meany += ydiff * recip;
    float newxdiff = x - _meanx;
    float newydiff = y - _meany;
    _varx += xdiff * newxdiff;
    _vary += ydiff * newydiff;
    _cov += xdiff * newydiff;
    
    return *this;
  }
  
  float GetMeanX() const { return _meanx; }
  float GetMeanY() const { return _meany; }
  float GetMinX() const { return _minx; }
  float GetMinY() const { return _miny; }
  float GetMaxX() const { return _maxx; }
  float GetMaxY() const { return _maxy; }
  float GetMinXSafe() const { return (_n > 0.0f) ? _minx : 0.0; }; // otherwise it'll be +FLT_MAX
  float GetMinYSafe() const { return (_n > 0.0f) ? _miny : 0.0; }; // otherwise it'll be +FLT_MAX
  float GetMaxXSafe() const { return (_n > 0.0f) ? _maxx : 0.0; }; // otherwise it'll be -FLT_MAX
  float GetMaxYSafe() const { return (_n > 0.0f) ? _maxy : 0.0; }; // otherwise it'll be -FLT_MAX
  float GetVarianceX() const { return (_n < 2) ? 0.0f : _varx / (_n - 1); }
  float GetVarianceY() const { return (_n < 2) ? 0.0f : _vary / (_n - 1); }
  float GetCovariance() const { return (_n < 2) ? 0.0f : _cov/(_n-1); }
  
  
  int GetNum() const { return (int)round(_n); };

  void Clear() {
    _n = 0.0f;
    _maxx = std::numeric_limits<float>::lowest();
    _maxy = std::numeric_limits<float>::lowest();
    _minx = std::numeric_limits<float>::max();
    _miny = std::numeric_limits<float>::max();
    _meanx = 0.0f;
    _meany = 0.0f;
    _varx = 0.0f;
    _vary = 0.0f;
    _cov = 0.0f;
  }

protected:
  float _n;
  float _minx;
  float _miny;
  float _maxx;
  float _maxy;
  float _meanx;
  float _meany;
  float _varx;
  float _vary;
  float _cov;
};
  

} // end namespace Stats
} // end namespace Util
} // end namespace Anki

#endif
