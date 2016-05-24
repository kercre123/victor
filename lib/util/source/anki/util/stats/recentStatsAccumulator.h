/**
 * File: recentStatsAccumulator
 *
 * Author: Mark Wesley
 * Created: 06/25/15
 *
 * Description: wraps 2 offset statsAccumulators that are reset every N samples, and offset by N/2 from each other
 *              so you can always calculate from N/2..N recent samples
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Util_Stats_RecentStatsAccumulator_H__
#define __Util_Stats_RecentStatsAccumulator_H__


#include "util/stats/statsAccumulator.h"
#include <stdint.h>


namespace Anki {
namespace Util {
namespace Stats {
  
class RecentStatsAccumulator
{
public:
  
  RecentStatsAccumulator(uint32_t maxValuesToTrack);
  
  void Clear();
  
  void AddStat(const double v);
  RecentStatsAccumulator& operator+=(const double v)
  {
    AddStat(v);
    return *this;
  }
  
  const StatsAccumulator& GetPrimaryAccumulator() const { return (_stats[1].GetNumDbl() > _stats[0].GetNumDbl()) ? _stats[1] : _stats[0]; }
  
  double GetVal()      const { return GetPrimaryAccumulator().GetVal(); }
  double GetMean()     const { return GetPrimaryAccumulator().GetMean(); }
  double GetStd()      const { return GetPrimaryAccumulator().GetStd(); }
  double GetVariance() const { return GetPrimaryAccumulator().GetVariance(); }
  double GetMin()      const { return GetPrimaryAccumulator().GetMin(); }
  double GetMax()      const { return GetPrimaryAccumulator().GetMax(); }
  double GetNumDbl()   const { return GetPrimaryAccumulator().GetNumDbl(); }
  
  int GetNum()    const { return GetPrimaryAccumulator().GetNum(); }
  int GetIntVal() const { return GetPrimaryAccumulator().GetIntVal(); }
  int GetIntMax() const { return GetPrimaryAccumulator().GetIntMax(); }
  int GetIntMin() const { return GetPrimaryAccumulator().GetIntMin(); }
  
  void     SetMaxValuesToTrack(uint32_t newVal);
  uint32_t GetMaxValuesToTrack() const { return _maxValuesToTrack; }
  
private:
  
  StatsAccumulator  _stats[2];
  uint32_t          _maxValuesToTrack;
};
  
} // end namespace Stats
} // end namespace Util
} // end namespace Anki

#endif // __Util_Stats_RecentStatsAccumulator_H__
