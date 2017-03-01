/**
 * File: cpuProfileSampleShared
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Data shared between all instances of a single profile (across calls and ticks)
 *              Currently just the name of the sample
 *              Can easily be extended to track stats like min/avg/max time, num calls (per-tick or ever)
 *              but as that can also be collected externally we skip it here for now to minimize skewing performance
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_CpuProfiler_CpuProfileSampleShared_H__
#define __Util_CpuProfiler_CpuProfileSampleShared_H__


#include "util/cpuProfiler/cpuProfilerSettings.h"
#include "util/stats/recentStatsAccumulator.h"
#include <assert.h>
#include <stdint.h>


#if ANKI_CPU_PROFILER_ENABLED


// Enable to track per-thread min/max/avg etc. for each sample over time
// (at slight increase in cost for each profile sample)
#define ANKI_CPU_PROFILE_TRACK_SAMPLE_STATS  1


namespace Anki {
namespace Util {


class CpuSampleStats
{
public:
  CpuSampleStats() { }
  
  void Reset()
  {
    _lifetimeStats.Clear();
    _recentStats[0].Clear();
    _recentStats[1].Clear();
  }
  
  void ResetRecent(uint32_t index)
  {
    assert(index < 2);
    if (index < 2)
    {
      _recentStats[index].Clear();
    }
  }
  
  void AddStat(double newVal)
  {
    _recentStats[0] += newVal;
    _recentStats[1] += newVal;
    _lifetimeStats += newVal;
  }
  
  const Stats::StatsAccumulator& GetRecentStats() const
  {
    return (_recentStats[0].GetNumDbl() > _recentStats[1].GetNumDbl()) ? _recentStats[0] : _recentStats[1];
  }
  const Stats::StatsAccumulator& GetLifetimeStats() const { return _lifetimeStats; }
  
private:
  Stats::StatsAccumulator       _lifetimeStats;
  // For recent stats we reset the larger of the 2 stats every N ticks, so after the first N ticks we always have (N..2N) recent samples in GetRecentStats()
  Stats::StatsAccumulator       _recentStats[2];
};
  
  
class CpuSamplePerThreadData
{
public:
  
  CpuSamplePerThreadData()
    : _isCalledFromThread(false)
  {
  }
  
  void Reset()
  {
    _stats.Reset();
    _isCalledFromThread = false;
  }
  
  CpuSampleStats&       GetSampleStats()       { return _stats; }
  const CpuSampleStats& GetSampleStats() const { return _stats; }
 
  bool IsCalledFromThread() const { return _isCalledFromThread; }
  void SetIsCalledFromThread(bool newVal = true) { _isCalledFromThread = newVal; }
  
private:
  
  CpuSampleStats _stats;
  bool           _isCalledFromThread;
};


class CpuProfileSampleShared
{
public:
  
  explicit CpuProfileSampleShared(const char* name)
    : _name(name)
  {
  }
  
  ~CpuProfileSampleShared() {}
  
  void AddSample(uint32_t threadIndex, double duration_ms)
  {
  #if ANKI_CPU_PROFILE_TRACK_SAMPLE_STATS
    GetSampleStats(threadIndex).AddStat(duration_ms);
  #endif // ANKI_CPU_PROFILE_TRACK_SAMPLE_STATS
  }
  
  const char* GetName() const { return _name; }
  
  void ResetForThread(uint32_t threadIndex) { GetThreadData(threadIndex).Reset(); }
  
  CpuSampleStats&       GetSampleStats(uint32_t threadIndex)         { return GetThreadData(threadIndex).GetSampleStats(); }
  const CpuSampleStats& GetSampleStats(uint32_t threadIndex)   const { return GetThreadData(threadIndex).GetSampleStats(); }
  const Stats::StatsAccumulator& GetRecentStats(uint32_t threadIndex)   const { return GetSampleStats(threadIndex).GetRecentStats(); }
  const Stats::StatsAccumulator& GetLifetimeStats(uint32_t threadIndex) const { return GetSampleStats(threadIndex).GetLifetimeStats(); }
  
  bool IsCalledFromThread(uint32_t threadIndex) const { return GetThreadData(threadIndex).IsCalledFromThread(); }
  void SetIsCalledFromThread(uint32_t threadIndex, bool newVal = true) { GetThreadData(threadIndex).SetIsCalledFromThread(newVal); }
  
private:
  
  const CpuSamplePerThreadData& GetThreadData(uint32_t threadIndex) const
  {
    assert(threadIndex < kCpuProfilerMaxThreads);
    return _threadData[threadIndex];
  }
  
  CpuSamplePerThreadData& GetThreadData(uint32_t threadIndex)
  {
    return const_cast<CpuSamplePerThreadData&>( const_cast<const CpuProfileSampleShared*>(this)->GetThreadData(threadIndex) );
  }
  
  const char*             _name;
  CpuSamplePerThreadData  _threadData[kCpuProfilerMaxThreads];
};


} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED


#endif // __Util_CpuProfiler_CpuProfileSampleShared_H__
