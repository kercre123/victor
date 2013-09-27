#include "anki/embeddedCommon/benchmarking.h"

#include <stdio.h>
#include <string.h>

#ifndef MAX
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef MIN
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

BenchmarkEvent benchmarkEvents[NUM_BENCHMARK_EVENTS];
int currentBenchmarkEvent;

static const char * eventNames[NUM_BENCHMARK_EVENTS];
static volatile int numEventNames;

static double totalTimes[NUM_BENCHMARK_EVENTS];
static double minTimes[NUM_BENCHMARK_EVENTS];
static double maxTimes[NUM_BENCHMARK_EVENTS];
static unsigned int numEvents[NUM_BENCHMARK_EVENTS];
static int lastBeginIndex[NUM_BENCHMARK_EVENTS];

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

void PrintBenchmarkResults()
{
  int i;

#if defined(_MSC_VER)
  LARGE_INTEGER frequency;
  double freqencyDouble;
  QueryPerformanceFrequency(&frequency);
  freqencyDouble = (double)(frequency.QuadPart);
#endif

  numEventNames = 0;

  for(i=0; i<NUM_BENCHMARK_EVENTS; i++) {
    totalTimes[i] = 0.0;
    minTimes[i] = (double)(0x7FFFFFFFFFFFFFFFL);
    maxTimes[i] = (double)(-0x7FFFFFFFFFFFFFFFL);
    numEvents[i] = 0;
    lastBeginIndex[i] = -1;
  }

  for(i=0; i<currentBenchmarkEvent; i++) {
    const unsigned int index = AddName(benchmarkEvents[i].name);
    if(benchmarkEvents[i].type == BENCHMARK_EVENT_BEGIN) {
      lastBeginIndex[index] = i;
    } else { // BENCHMARK_EVENT_END
      if(lastBeginIndex[index] < 0) {
        printf("Benchmark parse error: Perhaps BeginBenchmark() and EndBenchmark() were nested, or there were more than %d benchmark events.\n", NUM_BENCHMARK_EVENTS);
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
        const double elapsedTime = 0.0;
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
        const double elapsedTime = 0.0;
#else
        const double elapsedTime = (double)rawElapsedTime / 1000000000.0;
#endif

        minTimes[index] = MIN(minTimes[index], elapsedTime);
        maxTimes[index] = MAX(maxTimes[index], elapsedTime);;

        totalTimes[index] += elapsedTime;

        numEvents[index]++;

        lastBeginIndex[index] = -1;
      }
    }
  } // for(i=0; i<currentBenchmarkEvent; i++)

  for(i=0; i<numEventNames; i++) {
#if defined(USING_MOVIDIUS_GCC_COMPILER)
    printf("Event %s: MeanTime:%dms MinTime:%dms MaxTime:%dms NumEvents:%d TotalTime:%dms\n",
      eventNames[i], 1000*(int)(totalTimes[i]/(double)numEvents[i]), 1000*(int)(minTimes[i]), 1000*(int)(maxTimes[i]), numEvents[i], 1000*(int)(totalTimes[i]));
#else
    printf("Event %s: MeanTime:%fs MinTime:%fs MaxTime:%fs NumEvents:%d TotalTime:%fs\n",
      eventNames[i], totalTimes[i]/(double)numEvents[i], minTimes[i], maxTimes[i], numEvents[i], totalTimes[i]);
#endif
  } // for(i=0; i<numEventNames; i++)
} // void PrintBenchmarkResults()