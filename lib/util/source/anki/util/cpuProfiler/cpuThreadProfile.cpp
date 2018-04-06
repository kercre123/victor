/**
 * File: cpuThreadProfile
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Profile for 1 tick on 1 thread
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/cpuProfiler/cpuThreadProfile.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <pthread.h>
#include <unistd.h>

#include "json/json.h"

#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {
  
  
void CpuThreadProfile::EndTick()
{
  _endTimePoint = CpuProfileClock::now();
  _tickDuration = CalcDuration_ms(GetStartTimePoint(), GetEndTimePoint());
  assert(_started);
  _started = false;
}


void CpuThreadProfile::SortSamples()
{
  std::sort(_samples.begin(),
            _samples.end(),
            []( const CpuProfileSample& lhs, const CpuProfileSample& rhs )
            {
              return lhs.GetStartTimePoint() < rhs.GetStartTimePoint();
            }
            );
}


const char* depthToPrefix(int depth)
{
  static const char* kPrefixSpaces = "................................................................"; // Some loggers (e.g. Webots) strip spaces
  static const int   kPrefixSpacesLen = (int)strlen(kPrefixSpaces);
  const int desiredIndent = depth * 2;
  const int desiredOffset = kPrefixSpacesLen - desiredIndent;
  const int offset = Util::Clamp(desiredOffset, 0, kPrefixSpacesLen);
  return &kPrefixSpaces[offset];
}


void CpuThreadProfile::LogProfile(uint32_t threadIndex) const
{
  const size_t kMaxSampleDepth = 32;
  const CpuProfileSample* sampleStack[kMaxSampleDepth];
  size_t sampleStackDepth = 0;
  
  for (const CpuProfileSample& sample : _samples)
  {
    while (sampleStackDepth > 0)
    {
      const CpuProfileSample* latestSample = sampleStack[sampleStackDepth-1];
      if (latestSample->GetEndTimePoint() <= sample.GetStartTimePoint())
      {
        // new sample starts after latestSample finished - pop latestSample off the stack
        --sampleStackDepth;
      }
      else
      {
        // Nothing left to pop off
        break;
      }
    }
    
    {
      const CpuSampleStats& sampleStats = sample.GetSharedData().GetSampleStats(threadIndex);
      const Stats::StatsAccumulator& lifeTimeStats = sampleStats.GetLifetimeStats();
      const Stats::StatsAccumulator& recentStats   = sampleStats.GetRecentStats();
      
      PRINT_CH_INFO("CpuProfiler", "CPS", "%s%s: %.3fms [Start:%.3f, End:%.3f] (recent: %.3f..%.3f avg: %.3f num: %.0f) (lifetime: %.3f..%.3f avg: %.3f num: %.0f)",
                    depthToPrefix((int)sampleStackDepth),
                    sample.GetName(),
                    sample.GetDuration_ms(),
                    CalcDuration_ms(_startTimePoint, sample.GetStartTimePoint()),
                    CalcDuration_ms(_startTimePoint, sample.GetEndTimePoint()),
                    recentStats.GetMinSafe(), recentStats.GetMaxSafe(), recentStats.GetMean(), recentStats.GetNumDbl(),
                    lifeTimeStats.GetMinSafe(), lifeTimeStats.GetMaxSafe(), lifeTimeStats.GetMean(), lifeTimeStats.GetNumDbl()
                    );
    }
    
    if (sampleStackDepth < kMaxSampleDepth)
    {
      sampleStack[sampleStackDepth++] = &sample;
    }
  }
}
  

float CalcAvgMsPerTick(const Stats::StatsAccumulator& stats, float oneOverTickCount)
{
  const float callsPerTick = stats.GetNumDbl() * oneOverTickCount;
  const float avgMsPerTick = callsPerTick * stats.GetMean();
  return avgMsPerTick;
}


void CpuThreadProfile::LogAllCalledSamples(uint32_t threadIndex,
                                           const std::vector<CpuProfileSampleShared*>& samplesCalledFromThread) const
{
  std::vector<CpuProfileSampleShared*> sortedSamples = samplesCalledFromThread;
  
  const uint32_t recentTickCount = GetLargestRecentTickCount();
  const float oneOverRecentTickCount = (recentTickCount > 0) ? (1.0f / float(recentTickCount)) : 1.0f;
  
  std::sort(sortedSamples.begin(),
            sortedSamples.end(),
            [threadIndex, oneOverRecentTickCount]( const CpuProfileSampleShared* lhs, const CpuProfileSampleShared* rhs )
            {
              // Sort from highest average-per-tick to lowest
              const float avgMsPerTickL = CalcAvgMsPerTick(lhs->GetRecentStats(threadIndex), oneOverRecentTickCount);
              const float avgMsPerTickR = CalcAvgMsPerTick(rhs->GetRecentStats(threadIndex), oneOverRecentTickCount);
              return avgMsPerTickL > avgMsPerTickR;
            }
            );
  
  for (const CpuProfileSampleShared* sample : sortedSamples)
  {
    const CpuSampleStats& sampleStats = sample->GetSampleStats(threadIndex);
    const Stats::StatsAccumulator& lifeTimeStats = sampleStats.GetLifetimeStats();
    const Stats::StatsAccumulator& recentStats   = sampleStats.GetRecentStats();
    
    const float callsPerTick = recentStats.GetNumDbl() * oneOverRecentTickCount;
    const float avgMsPerTick = callsPerTick * recentStats.GetMean();
    
    PRINT_CH_INFO("CpuProfiler", "CpuProfileList", "[%.3f ms per tick] %s: (recent: %.3f..%.3f avg: %.3f num: %.0f) (lifetime: %.3f..%.3f avg: %.3f num: %.0f) (%.2f calls per tick)",
                  avgMsPerTick, sample->GetName(),
                  recentStats.GetMinSafe(), recentStats.GetMaxSafe(), recentStats.GetMean(), recentStats.GetNumDbl(),
                  lifeTimeStats.GetMinSafe(), lifeTimeStats.GetMaxSafe(), lifeTimeStats.GetMean(), lifeTimeStats.GetNumDbl(),
                  callsPerTick);
  }
}


void CpuThreadProfile::SaveChromeTracingProfile(FILE* fp, uint32_t threadIndex) const
{
  const size_t kMaxSampleDepth = 32;
  const CpuProfileSample* sampleStack[kMaxSampleDepth];
  size_t sampleStackDepth = 0;

  pid_t pid = getpid();

#if defined(ANDROID)
  uint64_t tid = (uint64_t)gettid();
#else
  uint64_t tid;
  pthread_threadid_np(NULL, &tid);
#endif

  for (const CpuProfileSample& sample : _samples)
  {
    while (sampleStackDepth > 0)
    {
      const CpuProfileSample* latestSample = sampleStack[sampleStackDepth-1];
      if (latestSample->GetEndTimePoint() <= sample.GetStartTimePoint())
      {
        // new sample starts after latestSample finished - pop latestSample off the stack
        fprintf(fp, "{\"name\": \"%s\", \"cat\": \"c++\", \"ph\": \"E\", \"ts\": %lld, \"pid\": %d, \"tid\": %llu},\n",
                latestSample->GetName(),
                std::chrono::duration_cast<std::chrono::milliseconds>(latestSample->GetEndTimePoint().time_since_epoch()).count(),
                pid,
                tid);
        --sampleStackDepth;
      }
      else
      {
        // Nothing left to pop off
        break;
      }
    }

    {
      fprintf(fp, "{\"name\": \"%s\", \"cat\": \"c++\", \"ph\": \"B\", \"ts\": %lld, \"pid\": %d, \"tid\": %llu},\n",
              sample.GetName(),
              std::chrono::duration_cast<std::chrono::milliseconds>(sample.GetStartTimePoint().time_since_epoch()).count(),
              pid,
              tid);
    }
    fflush(fp);
    
    if (sampleStackDepth < kMaxSampleDepth)
    {
      sampleStack[sampleStackDepth++] = &sample;
    }
  }

  while (sampleStackDepth > 0)
  {
    const CpuProfileSample* latestSample = sampleStack[sampleStackDepth-1];
    fprintf(fp, "{\"name\": \"%s\", \"cat\": \"c++\", \"ph\": \"E\", \"ts\": %lld, \"pid\": %d, \"tid\": %llu},\n",
            latestSample->GetName(),
            std::chrono::duration_cast<std::chrono::milliseconds>(latestSample->GetEndTimePoint().time_since_epoch()).count(),
            pid,
            tid);
    --sampleStackDepth;
  }
}


void CpuThreadProfile::PublishToWebService(const std::function<void(const Json::Value&)>& callback,
                                           uint32_t threadIndex,
                                           const std::vector<CpuProfileSampleShared*>& samplesCalledFromThread) const
{
  std::vector<CpuProfileSampleShared*> sortedSamples = samplesCalledFromThread;

  const uint32_t recentTickCount = GetLargestRecentTickCount();
  const float oneOverRecentTickCount = (recentTickCount > 0) ? (1.0f / float(recentTickCount)) : 1.0f;

  std::sort(sortedSamples.begin(),
            sortedSamples.end(),
            [threadIndex, oneOverRecentTickCount]( const CpuProfileSampleShared* lhs, const CpuProfileSampleShared* rhs )
            {
              // Sort from highest average-per-tick to lowest
              const float avgMsPerTickL = CalcAvgMsPerTick(lhs->GetRecentStats(threadIndex), oneOverRecentTickCount);
              const float avgMsPerTickR = CalcAvgMsPerTick(rhs->GetRecentStats(threadIndex), oneOverRecentTickCount);
              return avgMsPerTickL > avgMsPerTickR;
            }
            );


  Json::Value data;
  data["time"] = GetTickNum();

  auto& sampleJson = data["sample"];

  for (const CpuProfileSampleShared* sample : sortedSamples)
  {
    const CpuSampleStats& sampleStats = sample->GetSampleStats(threadIndex);
    const Stats::StatsAccumulator& recentStats = sampleStats.GetRecentStats();

    Json::Value entry;
    entry["name"] = sample->GetName();
    entry["min"] = recentStats.GetMinSafe();
    entry["max"] = recentStats.GetMaxSafe();
    entry["mean"] = recentStats.GetMean();
    sampleJson.append( entry );
  }
  callback(data);
}

} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED
