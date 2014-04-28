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

    // A big array, full of events
    OFFCHIP static BenchmarkEvent benchmarkEvents[MAX_BENCHMARK_EVENTS];

    static const s32 benchmarkPrintBufferLength = 512 + MAX_BENCHMARK_EVENTS*350;
    OFFCHIP static char benchmarkPrintBuffer[benchmarkPrintBufferLength];

    // The index of the next place to record a benchmark event
    static s32 numBenchmarkEvents;

    staticInline void AddBenchmarkEvent(const char *name, u64 time, BenchmarkEventType type);

    void InitBenchmarking()
    {
      numBenchmarkEvents = 0;
    }

    staticInline u64 GetBenchmarkTime()
    {
#if defined(_MSC_VER)
      static LONGLONG startCounter = 0;

      LARGE_INTEGER counter;

      QueryPerformanceCounter(&counter);

      // Subtract startSeconds, so the floating point number has reasonable precision
      if(startCounter == 0) {
        startCounter = counter.QuadPart;
      }

      return counter.QuadPart - startCounter;
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

    BenchmarkElement::BenchmarkElement(const char * name)
      : inclusive_mean(0), inclusive_min(DBL_MAX), inclusive_max(DBL_MIN), inclusive_total(0), exclusive_mean(0), exclusive_min(DBL_MAX), exclusive_max(DBL_MIN), exclusive_total(0), numEvents(0)
    {
      snprintf(this->name, BenchmarkElement::NAME_LENGTH, "%s", name);
    }

    s32 PrintElement(const char * prefix, const s32 number, const char * suffix, const s32 minNumberCharacterToPrint)
    {
      const s32 bufferLength = 128;
      char numberBuffer[bufferLength];
      char totalBuffer[bufferLength];

      SnprintfCommasS32(numberBuffer, bufferLength, number);

      snprintf(totalBuffer, bufferLength, "%s%s%s", prefix, numberBuffer, suffix);
      printf(totalBuffer);

      const s32 numNumberCharactersPrinted = strlen(numberBuffer);

      for(s32 i=numNumberCharactersPrinted;  i<minNumberCharacterToPrint; i++)
        printf(" ");

      return (minNumberCharacterToPrint - numNumberCharactersPrinted) + strlen(totalBuffer);
    }

    void BenchmarkElement::Print(const bool verbose, const bool microseconds, const FixedLengthList<s32> * minCharacterToPrint) const
    {
      const s32 bufferLength = 128;
      char buffer[bufferLength];

      //printf("Exclusive Mean:%fs Exclusive Min:%fs Exclusive Max:%fs Inclusive Mean:%fs Inclusive Min:%fs Inclusive Max:%fs (Exclusive Total:%fs Inclusive Total:%fs NumEvents:%d)\n",

      snprintf(buffer, bufferLength, "%s: ", this->name);
      printf(buffer);

      if(minCharacterToPrint && minCharacterToPrint->get_size() > 0) {
        const s32 numPrinted = strlen(buffer);
        for(s32 i=numPrinted;  i<((*minCharacterToPrint)[0]+2); i++) printf(" ");
      }

      const char * suffix;
      f64 multiplier;
      if(microseconds) {
        suffix = "us ";
        multiplier = 1000000.0;
      } else {
        suffix = "ms ";
        multiplier = 1000.0;
      }

      PrintElement("ExcTot:", Round<s32>(this->exclusive_total*multiplier), suffix, (*minCharacterToPrint)[1]);
      PrintElement("IncTot:", Round<s32>(this->inclusive_total*multiplier), suffix, (*minCharacterToPrint)[2]);
      PrintElement("NEvents:", this->numEvents, " ", (*minCharacterToPrint)[3]);

      if(verbose) {
        PrintElement("ExcMean:", Round<s32>(this->exclusive_mean*multiplier), suffix, (*minCharacterToPrint)[4]);
        PrintElement("ExcMin:", Round<s32>(this->exclusive_min*multiplier), suffix, (*minCharacterToPrint)[5]);
        PrintElement("ExcMax:", Round<s32>(this->exclusive_max*multiplier), suffix, (*minCharacterToPrint)[6]);
        PrintElement("IncMean:", Round<s32>(this->inclusive_mean*multiplier), suffix, (*minCharacterToPrint)[7]);
        PrintElement("IncMin:", Round<s32>(this->inclusive_min*multiplier), suffix, (*minCharacterToPrint)[8]);
        PrintElement("IncMax:", Round<s32>(this->inclusive_max*multiplier), suffix, (*minCharacterToPrint)[9]);
      }

      printf("\n");
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

          AnkiConditionalErrorAndReturnValue(outputResults.IsValid() && fullList.IsValid() && parseStack.IsValid(),
            outputResults, "ComputeBenchmarkResults", "Out of memory");

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
              //const s32 numParents = endLevel;
              if(endLevel > 0) {
                pParseStack[endLevel-1].exclusiveTimeElapsed -= elapsedTime;
              }

              /*for(s32 iParent=0; iParent<numParents; iParent++) {
              pParseStack[iParent].exclusiveTimeElapsed -= elapsedTime;
              }*/

              fullList.PushBack(BenchmarkInstance(benchmarkEvents[iEvent].name, startInstance.startTime, elapsedTime, elapsedTime + startInstance.exclusiveTimeElapsed, endLevel));
            }
          } // for(s32 iEvent=0; iEvent<numBenchmarkEvents; iEvent++)

          // Second, combine the events in fullList to create the total outputResults
          const s32 numFullList = fullList.get_size();
          const BenchmarkInstance * pFullList = fullList.Pointer(0);
          BenchmarkElement * pOutputResults = outputResults.Pointer(0);

          //fullList.Print("fullList");

          for(s32 iInstance=0; iInstance<numFullList; iInstance++) {
            const char * const curName = pFullList[iInstance].name;
            const f64 curInclusiveTimeElapsed = pFullList[iInstance].inclusiveTimeElapsed;
            const f64 curExclusiveTimeElapsed = pFullList[iInstance].exclusiveTimeElapsed;

            s32 index = GetNameIndex(curName, outputResults);

            // If this name hasn't been used yet, add it
            if(index < 0) {
              BenchmarkElement newElement(curName);
              outputResults.PushBack(curName);
              index = GetNameIndex(curName, outputResults);
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

        void Print() const
        {
          const s32 bufferLength = 128;
          char buffer[bufferLength];

          //printf("Exclusive Mean:%fs Exclusive Min:%fs Exclusive Max:%fs Inclusive Mean:%fs Inclusive Min:%fs Inclusive Max:%fs (Exclusive Total:%fs Inclusive Total:%fs NumEvents:%d)\n",

          printf("%s: ", this->name);

          SnprintfCommasS32(buffer, bufferLength, Round<s32>(this->exclusiveTimeElapsed*1000000.0));
          printf("exclusiveTimeElapsed:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, Round<s32>(this->inclusiveTimeElapsed*1000000.0));
          printf("inclusiveTimeElapsed:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, Round<s32>(this->startTime*1000000.0));
          printf("startTime:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, this->level);
          printf("NumEvents:%s\n", buffer);
        }
      } BenchmarkInstance;

      // Returns the index of "name", or -1 if it isn't found
      static s32 GetNameIndex(const char * name, const FixedLengthList<BenchmarkElement> &outputResults)
      {
        const BenchmarkElement * pOutputResults = outputResults.Pointer(0);
        const s32 numOutputResults = outputResults.get_size();

        for(s32 i=0; i<numOutputResults; i++) {
          if(strcmp(name, pOutputResults[i].name) == 0) {
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

    Result PrintBenchmarkResults(const FixedLengthList<BenchmarkElement> &results, const bool verbose, const bool microseconds)
    {
      const s32 scratchBufferLength = 128;
      char scratchBuffer[scratchBufferLength];

      const s32 stringBufferLength = 128;
      char stringBuffer[stringBufferLength];

      MemoryStack scratch(scratchBuffer, scratchBufferLength);

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Out of memory");

      FixedLengthList<s32> minCharacterToPrint(10, scratch, Flags::Buffer(true, false, true));

      AnkiConditionalErrorAndReturnValue(minCharacterToPrint.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Out of memory");

      printf("Benchmark Results: (Exc=Exclusive Inc=Inclusive Tot=Total N=NumberOf)\n");

      const s32 numResults = results.get_size();
      const BenchmarkElement * const pResults = results.Pointer(0);

      f64 multiplier;
      if(microseconds) {
        multiplier = 1000000.0;
      } else {
        multiplier = 1000.0;
      }

      for(s32 x=0; x<numResults; x++) {
        const s32 values[] = {
          Round<s32>(pResults[x].exclusive_total*multiplier),
          Round<s32>(pResults[x].inclusive_total*multiplier),
          pResults[x].numEvents,
          Round<s32>(pResults[x].exclusive_mean*multiplier),
          Round<s32>(pResults[x].exclusive_min*multiplier),
          Round<s32>(pResults[x].exclusive_max*multiplier),
          Round<s32>(pResults[x].inclusive_mean*multiplier),
          Round<s32>(pResults[x].inclusive_min*multiplier),
          Round<s32>(pResults[x].inclusive_max*multiplier)};

        minCharacterToPrint[0] = MAX(minCharacterToPrint[0], static_cast<s32>(strlen(pResults[x].name)));

        for(s32 iVal=0; iVal<=8; iVal++) {
          SnprintfCommasS32(stringBuffer, stringBufferLength, values[iVal]);
          minCharacterToPrint[iVal+1] = MAX(minCharacterToPrint[iVal+1], static_cast<s32>(strlen(stringBuffer)));
        }
      }

      for(s32 x=0; x<numResults; x++) {
        pResults[x].Print(verbose, microseconds, &minCharacterToPrint);
      }

      printf("\n");

      return RESULT_OK;
    }

    Result ComputeAndPrintBenchmarkResults(const bool verbose, const bool microseconds)
    {
      MemoryStack scratch(benchmarkPrintBuffer, benchmarkPrintBufferLength);

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Out of memory");

      FixedLengthList<BenchmarkElement> results = ComputeBenchmarkResults(scratch);

      AnkiConditionalErrorAndReturnValue(results.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "PrintBenchmarkResults", "results is not valid");

      return PrintBenchmarkResults(results, verbose, microseconds);
    }
  } // namespace Embedded
} // namespace Anki
