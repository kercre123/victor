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


#include "util/stats/recentStatsAccumulator.h"


namespace Anki {
namespace Util {
namespace Stats {
  
      
RecentStatsAccumulator::RecentStatsAccumulator(uint32_t maxValuesToTrack)
  : _maxValuesToTrack(maxValuesToTrack)
{
}
  
  
void RecentStatsAccumulator::Clear()
{
  _stats[0].Clear();
  _stats[1].Clear();
}

  
void RecentStatsAccumulator::AddStat(const double v)
{
  const double maxValuesToTrackDbl = (double)_maxValuesToTrack;
  const double minValuesToTrackDbl = 0.5 * maxValuesToTrackDbl;
  
  _stats[0] += v;
  // Don't start adding to the 2nd stat collector until 1st one is half full
  if ((_stats[0].GetNumDbl() > minValuesToTrackDbl) || (_stats[1].GetNumDbl() > 0.001))
  {
    _stats[1] += v;
  }
  
  // Reset if either is over the number of samples
  if (_stats[0].GetNumDbl() > maxValuesToTrackDbl)
  {
    _stats[0].Clear();
  }
  if (_stats[1].GetNumDbl() > maxValuesToTrackDbl)
  {
    _stats[1].Clear();
  }
}
  
  
void RecentStatsAccumulator::SetMaxValuesToTrack(uint32_t newVal)
{
  if (_maxValuesToTrack != newVal)
  {
    Clear();
    _maxValuesToTrack = newVal;
  }
}


} // end namespace Stats
} // end namespace Util
} // end namespace Anki

