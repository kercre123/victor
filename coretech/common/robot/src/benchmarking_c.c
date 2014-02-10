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
#elif defined(USING_MOVIDIUS_GCC_COMPILER)
// Included at the top-level config
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
// Included at the top-level config
#else
#include <sys/time.h>
#endif

#ifdef USING_MOVIDIUS_GCC_COMPILER
#define BENCHMARK_EVENTS_LOCATION __attribute__((section(".ddr.bss")))
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
BENCHMARK_EVENTS_LOCATION BenchmarkEvent benchmarkEvents[MAX_BENCHMARK_EVENTS];

// The index of the next place to record a benchmark event
int numBenchmarkEvent;

BENCHMARK_EVENTS_LOCATION static const char * eventNames[MAX_BENCHMARK_EVENTS];
static volatile int numEventNames;

BENCHMARK_EVENTS_LOCATION static double totalTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static double minTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static double maxTimes[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static unsigned int numEvents[MAX_BENCHMARK_EVENTS];
BENCHMARK_EVENTS_LOCATION static int lastBeginIndex[MAX_BENCHMARK_EVENTS];

void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type);

void PrintBenchmarkResults_All();
void PrintBenchmarkResults_OnlyTotals();
void PrintBenchmarkResults(const BenchmarkPrintType printType);

void InitBenchmarking()
{
  numBenchmarkEvent = 0;
}

unsigned long long GetBenchmarkTime()
{
#if defined(_MSC_VER)
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);

  return counter.QuadPart;
#elif defined(__APPLE_CC__)
  return 0; // TODO: implement
#elif defined(USING_MOVIDIUS_GCC_COMPILER)
  return DrvTimerGetSysTicks64();
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
  return 0; // TODO: implement
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
}

void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type)
{
  benchmarkEvents[numBenchmarkEvent].name = name;
  benchmarkEvents[numBenchmarkEvent].time = time;
  benchmarkEvents[numBenchmarkEvent].type = type;

  numBenchmarkEvent++;

  // If we run out of space, just keep overwriting the last event
  if(numBenchmarkEvent >= MAX_BENCHMARK_EVENTS)
    numBenchmarkEvent = MAX_BENCHMARK_EVENTS-1;
}

void BeginBenchmark(const char *name)
{
  AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_BEGIN);
} // void startBenchmark(const char *name)

void EndBenchmark(const char *name)
{
  AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_END);
} // void endBenchmark(const char *name)

int GetNameIndex(const char * const name, unsigned int * index)
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

unsigned int AddName(const char * const name)
{
  unsigned int index = 0;

  if(GetNameIndex(name, &index)) {
    eventNames[numEventNames++] = name;
    return numEventNames-1;
  } else {
    return index;
  }
}

void PrintBenchmarkResults_All()
{
  PrintBenchmarkResults(BENCHMARK_PRINT_ALL);
}

void PrintBenchmarkResults_OnlyTotals()
{
  PrintBenchmarkResults(BENCHMARK_PRINT_TOTALS);
}

void PrintBenchmarkResults(const BenchmarkPrintType printType)
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

  for(i=0; i<numBenchmarkEvent; i++) {
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
#pragma unused (rawElapsedTime) // may or may not get used depending on #ifs below

#if defined(_MSC_VER)
        const double elapsedTime = (double)rawElapsedTime / freqencyDouble;
#elif defined(__APPLE_CC__)
        const double elapsedTime = 0.0;
#elif defined(USING_MOVIDIUS_GCC_COMPILER)
        const double elapsedTime = (1.0 / 1000.0) * DrvTimerTicksToMs(rawElapsedTime);
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
        const double elapsedTime = 0.0;
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
  } // for(i=0; i<numBenchmarkEvent; i++)

  for(i=0; i<numEventNames; i++) {
    printf("Event ");
    printf(eventNames[i]);
    if(printType == BENCHMARK_PRINT_ALL) {
      printf(": Mean:%fs Min:%fs Max:%fs Total:%fs NumEvents:%d\n",
        totalTimes[i]/(double)numEvents[i], minTimes[i], maxTimes[i], totalTimes[i], numEvents[i]);
    } else if (printType == BENCHMARK_PRINT_TOTALS) {
      printf(": Total:%fs\n",
        totalTimes[i]);
    }
  } // for(i=0; i<numEventNames; i++)
} // void PrintBenchmarkResults()