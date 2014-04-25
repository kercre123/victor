/**
File: benchmarking.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/benchmarking.h"
#include "anki/common/robot/utilities.h"

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__EDG__)
#include "anki/cozmo/robot/hal.h"
#else
#include <sys/time.h>
#endif

typedef enum
{
  BENCHMARK_EVENT_BEGIN,
  BENCHMARK_EVENT_END
} BenchmarkEventType;

typedef struct
{
  const char * name; // WARNING: name must be in globally available memory
  u64 time;
  BenchmarkEventType type;
} BenchmarkEvent;

typedef enum
{
  BENCHMARK_PRINT_ALL,
  BENCHMARK_PRINT_TOTALS,
} BenchmarkPrintType;

// A big array, full of events
OFFCHIP static BenchmarkEvent benchmarkEvents[MAX_BENCHMARK_EVENTS];

// The index of the next place to record a benchmark event
static s32 numBenchmarkEvents;

OFFCHIP static const char * eventNames[MAX_BENCHMARK_EVENTS];
static s32 numEventNames;

OFFCHIP static f64 totalTimes[MAX_BENCHMARK_EVENTS];
OFFCHIP static f64 minTimes[MAX_BENCHMARK_EVENTS];
OFFCHIP static f64 maxTimes[MAX_BENCHMARK_EVENTS];
OFFCHIP static u32 numEvents[MAX_BENCHMARK_EVENTS];
OFFCHIP static s32 lastBeginIndex[MAX_BENCHMARK_EVENTS];

staticInline void AddBenchmarkEvent(const char *name, u64 time, BenchmarkEventType type);

static void PrintBenchmarkResults(const BenchmarkPrintType printType);

void InitBenchmarking(void)
{
  numBenchmarkEvents = 0;
}

staticInline u64 GetBenchmarkTime(void)
{
#if defined(_MSC_VER)
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return counter.QuadPart;
#elif defined(__APPLE_CC__)
  struct timeval time;
  gettimeofday(&time, NULL);
  return (u64)time.tv_sec*1000000ULL + (u64)time.tv_usec;
#elif defined (__EDG__)  // MDK-ARM
  return Anki::Cozmo::HAL::GetMicroCounter();
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
#endif
}

staticInline void AddBenchmarkEvent(const char *name, const u64 time, BenchmarkEventType type)
{
  benchmarkEvents[numBenchmarkEvents].name = name;
  benchmarkEvents[numBenchmarkEvents].time = time;
  benchmarkEvents[numBenchmarkEvents].type = type;

  numBenchmarkEvents++;

  // If we run out of space, just keep overwriting the last event
  if(numBenchmarkEvents >= MAX_BENCHMARK_EVENTS)
    numBenchmarkEvents = MAX_BENCHMARK_EVENTS-1;
}

void BeginBenchmark(const char *name)
{
  AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_BEGIN);
} // void startBenchmark(const char *name)

void EndBenchmark(const char *name)
{
  AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_END);
} // void endBenchmark(const char *name)

static s32 GetNameIndex(const char * const name, u32 * index)
{
  s32 i;
  for(i=0; i<numEventNames; i++)
  {
    if(strcmp(name, eventNames[i]) == 0)
    {
      *index = i;
      return 0;
    }
  }

  return -1;
}

static u32 AddName(const char * const name)
{
  u32 index = 0;

  if(GetNameIndex(name, &index)) {
    eventNames[numEventNames++] = name;
    return numEventNames-1;
  } else {
    return index;
  }
}

void PrintBenchmarkResults_All(void)
{
  PrintBenchmarkResults(BENCHMARK_PRINT_ALL);
}

void PrintBenchmarkResults_OnlyTotals(void)
{
  PrintBenchmarkResults(BENCHMARK_PRINT_TOTALS);
}

static void PrintBenchmarkResults(const BenchmarkPrintType printType)
{
  s32 i;

#if defined(_MSC_VER)
  LARGE_INTEGER frequency;
  f64 freqencyDouble;
  QueryPerformanceFrequency(&frequency);
  freqencyDouble = (f64)(frequency.QuadPart);
#endif

  numEventNames = 0;

  for(i=0; i<MAX_BENCHMARK_EVENTS; i++) {
    totalTimes[i] = 0.0;
    minTimes[i] = (f64)(0x7FFFFFFFFFFFFFFFLL);
    maxTimes[i] = (f64)(-0x7FFFFFFFFFFFFFFFLL);
    numEvents[i] = 0;
    lastBeginIndex[i] = -1;
  }

  for(i=0; i<numBenchmarkEvents; i++) {
    const u32 index = AddName(benchmarkEvents[i].name);
    if(benchmarkEvents[i].type == BENCHMARK_EVENT_BEGIN) {
      lastBeginIndex[index] = i;
    } else { // BENCHMARK_EVENT_END
      if(lastBeginIndex[index] < 0) {
        printf("Benchmark parse error: Perhaps BeginBenchmark() and EndBenchmark() were nested, or there were more than %d benchmark events.\n", MAX_BENCHMARK_EVENTS);
        continue;
      }

      {
        const u64 rawElapsedTime = benchmarkEvents[i].time - benchmarkEvents[lastBeginIndex[index]].time;
        //#pragma unused (rawElapsedTime) // may or may not get used depending on #ifs below

#if defined(_MSC_VER)
        const f64 elapsedTime = (f64)rawElapsedTime / freqencyDouble;
#elif defined(__APPLE_CC__)
        const f64 elapsedTime = (f64)rawElapsedTime / 1000000.0;
#elif defined(__EDG__)  // MDK-ARM
        const f64 elapsedTime = (f64)rawElapsedTime / 1000000.0;
#else
        const f64 elapsedTime = (f64)rawElapsedTime / 1000000000.0;
#endif

        minTimes[index] = MIN(minTimes[index], elapsedTime);
        maxTimes[index] = MAX(maxTimes[index], elapsedTime);

        totalTimes[index] += elapsedTime;

        numEvents[index]++;

        lastBeginIndex[index] = -1;
      }
    }
  } // for(i=0; i<numBenchmarkEvents; i++)

  for(i=0; i<numEventNames; i++) {
    printf("Event ");
    printf(eventNames[i]);
    if(printType == BENCHMARK_PRINT_ALL) {
      printf(": Mean:%dus Min:%dus Max:%dus Total:%dus NumEvents:%d\n",
        (s32)DBL_ROUND(1000000*totalTimes[i]/(f64)numEvents[i]), (s32)DBL_ROUND(1000000*minTimes[i]), (s32)DBL_ROUND(1000000*maxTimes[i]), (s32)DBL_ROUND(1000000*totalTimes[i]), (s32)numEvents[i]);
    } else if (printType == BENCHMARK_PRINT_TOTALS) {
      printf(": Total:%dus\n",
        (s32)DBL_ROUND(1000000*totalTimes[i]));
    }
  } // for(i=0; i<numEventNames; i++)
} // void PrintBenchmarkResults()
