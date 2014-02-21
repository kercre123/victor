/**
File: benchmarking_c.c
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/utilities_c.h"

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__EDG__)
extern u32 XXX_HACK_FOR_PETE(void);
#else
#include <sys/time.h>
#endif

#if defined(__EDG__) // MDK-ARM
#define BENCHMARK_EVENTS_LOCATION __attribute__((section(".ram1")))
#else
#define BENCHMARK_EVENTS_LOCATION
#endif

typedef enum
{
  BENCHMARK_EVENT_BEGIN,
  BENCHMARK_EVENT_END
} BenchmarkEventType;

typedef struct
{
  const char * name; // WARNING: name must be in globally available memory
  unsigned long long time;
  BenchmarkEventType type;
} BenchmarkEvent;

typedef enum
{
  BENCHMARK_PRINT_ALL,
  BENCHMARK_PRINT_TOTALS,
} BenchmarkPrintType;

// A big array, full of events
BENCHMARK_EVENTS_LOCATION static BenchmarkEvent benchmarkEvents[MAX_BENCHMARK_EVENTS];

// The index of the next place to record a benchmark event
static int numBenchmarkEvents;

BENCHMARK_EVENTS_LOCATION static const char * eventNames[MAX_BENCHMARK_EVENTS];
static volatile int numEventNames;

BENCHMARK_EVENTS_LOCATION static double totalTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static double minTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static double maxTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static unsigned int numEvents[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static int lastBeginIndex[MAX_BENCHMARK_EVENTS];

staticInline void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type);

static void PrintBenchmarkResults(const BenchmarkPrintType printType);

void InitBenchmarking(void)
{
  numBenchmarkEvents = 0;
}

staticInline unsigned long long GetBenchmarkTime(void)
{
#if defined(_MSC_VER)
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  return counter.QuadPart;
#elif defined(__APPLE_CC__)
  return 0; // TODO: implement
#elif defined (__EDG__)  // MDK-ARM
  return XXX_HACK_FOR_PETE();
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

staticInline void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type)
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

static int GetNameIndex(const char * const name, unsigned int * index)
{
  int i;
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

static unsigned int AddName(const char * const name)
{
  unsigned int index = 0;

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
  int i;

#if defined(_MSC_VER)
  LARGE_INTEGER frequency;
  double freqencyDouble;
  QueryPerformanceFrequency(&frequency);
  freqencyDouble = (double)(frequency.QuadPart);
#endif

  numEventNames = 0;

  for(i=0; i<MAX_BENCHMARK_EVENTS; i++) {
    totalTimes[i] = 0.0;
    minTimes[i] = (double)(0x7FFFFFFFFFFFFFFFLL);
    maxTimes[i] = (double)(-0x7FFFFFFFFFFFFFFFLL);
    numEvents[i] = 0;
    lastBeginIndex[i] = -1;
  }

  for(i=0; i<numBenchmarkEvents; i++) {
    const unsigned int index = AddName(benchmarkEvents[i].name);
    if(benchmarkEvents[i].type == BENCHMARK_EVENT_BEGIN) {
      lastBeginIndex[index] = i;
    } else { // BENCHMARK_EVENT_END
      if(lastBeginIndex[index] < 0) {
        printf("Benchmark parse error: Perhaps BeginBenchmark() and EndBenchmark() were nested, or there were more than %d benchmark events.\n", MAX_BENCHMARK_EVENTS);
        continue;
      }

      {
        const unsigned long long rawElapsedTime = benchmarkEvents[i].time - benchmarkEvents[lastBeginIndex[index]].time;
        //#pragma unused (rawElapsedTime) // may or may not get used depending on #ifs below

#if defined(_MSC_VER)
        const double elapsedTime = (double)rawElapsedTime / freqencyDouble;
#elif defined(__APPLE_CC__)
        const double elapsedTime = 0.0;
#elif defined(__EDG__)  // MDK-ARM
        const double elapsedTime = (double)rawElapsedTime / 1000000.0;
#else
        const double elapsedTime = (double)rawElapsedTime / 1000000000.0;
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
        (s32)Round(1000000*totalTimes[i]/(double)numEvents[i]), (s32)Round(1000000*minTimes[i]), (s32)Round(1000000*maxTimes[i]), (s32)Round(1000000*totalTimes[i]), (s32)numEvents[i]);
    } else if (printType == BENCHMARK_PRINT_TOTALS) {
      printf(": Total:%dus\n",
        (s32)Round(1000000*totalTimes[i]));
    }
  } // for(i=0; i<numEventNames; i++)
} // void PrintBenchmarkResults()
