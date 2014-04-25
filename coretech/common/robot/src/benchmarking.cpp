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

namespace Anki
{
  namespace Embedded
  {
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

    void InitBenchmarking()
    {
      numBenchmarkEvents = 0;
    }

    staticInline u64 GetBenchmarkTime()
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

    void PrintBenchmarkResults_All()
    {
      PrintBenchmarkResults(BENCHMARK_PRINT_ALL);
    }

    void PrintBenchmarkResults_OnlyTotals()
    {
      PrintBenchmarkResults(BENCHMARK_PRINT_TOTALS);
    }

    static void PrintBenchmarkResults(const BenchmarkPrintType printType)
    {
      s32 i;

#if defined(_MSC_VER)
      LARGE_INTEGER frequency;
      f64 frequencyF64;
      QueryPerformanceFrequency(&frequency);
      frequencyF64 = (f64)(frequency.QuadPart);
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
            const f64 elapsedTime = (f64)rawElapsedTime / frequencyF64;
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

    BenchmarkElement::BenchmarkElement(const char * name)
      : inclusive_mean(0), inclusive_min(DBL_MAX), inclusive_max(DBL_MIN), inclusive_total(0), exclusive_mean(0), exclusive_min(DBL_MAX), exclusive_max(DBL_MIN), exclusive_total(0), numEvents(0)
    {
      snprintf(this->name, BenchmarkElement::NAME_LENGTH, "%s", name);
    }

    class CompileBenchmarkResults
    {
    public:

      static FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory)
      {
        FixedLengthList<BenchmarkElement> outputResults(MAX_BENCHMARK_EVENTS, memory);

        AnkiConditionalErrorAndReturnValue(numBenchmarkEvents > 0 && numBenchmarkEvents < MAX_BENCHMARK_EVENTS,
          outputResults, "ComputeBenchmarkResults", "Invalid numBenchmarkEvents");

#if defined(_MSC_VER)
        LARGE_INTEGER frequency;
        f64 frequencyF64;
        QueryPerformanceFrequency(&frequency);
        frequencyF64 = (f64)(frequency.QuadPart);
#endif

        {
          PUSH_MEMORY_STACK(memory);

          FixedLengthList<BenchmarkInstance> fullList(MAX_BENCHMARK_EVENTS, memory);
          FixedLengthList<BenchmarkInstance> parseStack(MAX_BENCHMARK_EVENTS, memory);

          BenchmarkInstance * pParseStack = parseStack.Pointer(0);

          // First, parse each event individually, and add it to the fullList
          for(s32 iEvent=0; iEvent<numBenchmarkEvents; iEvent++) {
#if defined(_MSC_VER)
            const f64 timeF64 = static_cast<f64>(benchmarkEvents[iEvent].time) / frequencyF64;
#elif defined(__APPLE_CC__)
            const f64 timeF64 = static_cast<f64>(benchmarkEvents[iEvent].time) / 1000000.0;
#elif defined(__EDG__)  // MDK-ARM
            const f64 timeF64 = static_cast<f64>(benchmarkEvents[iEvent].time) / 1000000.0;
#else
            const f64 timeF64 = static_cast<f64>(benchmarkEvents[iEvent].time) / 1000000000.0;
#endif

            if(benchmarkEvents[iEvent].type == BENCHMARK_EVENT_BEGIN) {
              const s32 startLevel = parseStack.get_size();

              parseStack.PushBack(BenchmarkInstance(benchmarkEvents[iEvent].name, timeF64, 0, 0, startLevel));
            } else {
              // BENCHMARK_EVENT_END

              const BenchmarkInstance startInstance = parseStack.PopBack();

              const s32 endLevel = parseStack.get_size();

              AnkiConditionalErrorAndReturnValue(strcmp(startInstance.name, benchmarkEvents[iEvent].name) == 0 && startInstance.level == endLevel,
                outputResults, "ComputeBenchmarkResults", "Benchmark parse error: Perhaps BeginBenchmark() and EndBenchmark() were nested, or there were more than %d benchmark events, or some other non-supported thing listed in the comments.\n", MAX_BENCHMARK_EVENTS);

              const f64 elapsedTime = timeF64 - startInstance.startTime;

              // Remove the time for this event from the exclusive time for all other events on the stack
              const s32 numParents = endLevel;
              for(s32 iParent=0; iParent<numParents; iParent++) {
                pParseStack[iParent].exclusiveTimeElapsed -= elapsedTime;
              }

              fullList.PushBack(BenchmarkInstance(benchmarkEvents[iEvent].name, startInstance.startTime, elapsedTime, elapsedTime + startInstance.exclusiveTimeElapsed, endLevel));
            }
          } // for(s32 iEvent=0; iEvent<numBenchmarkEvents; iEvent++)

          // Second, combine the events in fullList to create the total outputResults
          const s32 numFullList = fullList.get_size();
          const BenchmarkInstance * pFullList = fullList.Pointer(0);
          BenchmarkElement * pOutputResults = outputResults.Pointer(0);

          for(s32 iInstance=0; iInstance<numFullList; iInstance++) {
            const char * const curName = pFullList[iInstance].name;
            const f64 curInclusiveTimeElapsed = pFullList[iInstance].inclusiveTimeElapsed;
            const f64 curExclusiveTimeElapsed = pFullList[iInstance].exclusiveTimeElapsed;

            s32 index = GetNameIndex(curName, fullList);

            // If this name hasn't been used yet, add it
            if(index < 0) {
              BenchmarkElement newElement(curName);
              outputResults.PushBack(curName);
              index = GetNameIndex(curName, fullList);
              AnkiAssert(index >= 0);
            }

            pOutputResults[index].inclusive_min = MIN(pOutputResults[index].inclusive_min, curInclusiveTimeElapsed);
            pOutputResults[index].inclusive_max = MAX(pOutputResults[index].inclusive_max, curInclusiveTimeElapsed);
            pOutputResults[index].inclusive_total += curInclusiveTimeElapsed;

            pOutputResults[index].exclusive_min = MIN(pOutputResults[index].exclusive_min, curExclusiveTimeElapsed);
            pOutputResults[index].exclusive_max = MAX(pOutputResults[index].exclusive_max, curExclusiveTimeElapsed);
            pOutputResults[index].exclusive_total += curExclusiveTimeElapsed;

            pOutputResults[index].numEvents++;
          } // for(s32 iInstance=0; iInstance<numFullList; iInstance++)

          const s32 numEvents = outputResults.get_size();
          for(s32 iEvent=0; iEvent<numEvents; iEvent++) {
            pOutputResults[iEvent].inclusive_mean = pOutputResults[iEvent].inclusive_total / pOutputResults[iEvent].numEvents;
            pOutputResults[iEvent].exclusive_mean = pOutputResults[iEvent].exclusive_total / pOutputResults[iEvent].numEvents;
          } // for(s32 iEvent=0; iEvent<numEvents; iEvent++)
        } // PUSH_MEMORY_STACK(memory);

        // TODO: resize outputResults

        return outputResults;
      } // ComputeBenchmarkResults()

    protected:
      typedef struct BenchmarkInstance
      {
        f64 startTime; // A standard timestamp for the parseStack
        f64 inclusiveTimeElapsed; // An elapsed inclusive time for the fullList
        f64 exclusiveTimeElapsed; //< May be negative while the algorithm is still parsing
        s32 level; //< Level 0 is the base level. Every sub-benchmark adds another level.

        char name[BenchmarkElement::NAME_LENGTH];

        BenchmarkInstance(const char * name, const f64 startTime, const f64 inclusiveTimeElapsed, f64 exclusiveTimeElapsed, const s32 level)
          : startTime(startTime), inclusiveTimeElapsed(inclusiveTimeElapsed), exclusiveTimeElapsed(exclusiveTimeElapsed), level(level)
        {
          snprintf(this->name, BenchmarkElement::NAME_LENGTH, "%s", name);
        }
      } BenchmarkInstance;

      // Returns the index of "name", or -1 if it isn't found
      static s32 GetNameIndex(const char * name, const FixedLengthList<BenchmarkInstance> &fullList)
      {
        const BenchmarkInstance * pFullList = fullList.Pointer(0);
        const s32 numFullList = fullList.get_size();

        for(s32 i=0; i<numFullList; i++) {
          if(strcmp(name, pFullList[i].name) == 0) {
            return i;
          }
        } // for(s32 i=0; i<numOutputResults; i++)

        return -1;
      } // GetNameIndex()
    }; // class CompileBenchmarkResults

    FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory)
    {
      return CompileBenchmarkResults::ComputeBenchmarkResults(memory);
    } // FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory)
  } // namespace Embedded
} // namespace Anki
