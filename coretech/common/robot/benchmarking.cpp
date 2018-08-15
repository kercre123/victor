/**
File: benchmarking.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/utilities.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "util/helpers/ankiDefines.h"

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(__EDG__)
#include "anki/cozmo/robot/hal.h"
#elif defined(__GNUC__)
#if defined(__ARM_ARCH_7A__)
#include <time.h>
#else
#include <sys/time.h>
#endif
#endif

#if ANKI_EMBEDDED_BENCHMARK
typedef enum
{
  BENCHMARK_EVENT_BEGIN,
  BENCHMARK_EVENT_END
} BenchmarkEventType;

typedef struct
{
  const char * name; // WARNING: name must be in globally available memory
  u32 time;
  BenchmarkEventType type;
} BenchmarkEvent;

// A big array, full of events
OFFCHIP static BenchmarkEvent g_benchmarkEvents[Anki::Embedded::MAX_BENCHMARK_EVENTS];

// The index of the next place to record a benchmark event
static s32 g_numBenchmarkEvents;

//static const s32 benchmarkPrintBufferLength = 512 + Anki::Embedded::MAX_BENCHMARK_EVENTS*350;
//OFFCHIP static char benchmarkPrintBuffer[benchmarkPrintBufferLength];

staticInline void AddBenchmarkEvent(const char *name, u32 time, BenchmarkEventType type);

// For use instead of a BenchmarkElement for intermediate processing
typedef struct BasicBenchmarkElement
{
  // All times in microseconds, on all platforms

  // Inclusive includes all the time for all sub-benchmarks
  u32 inclusive_total;
  u32 inclusive_min;
  u32 inclusive_max;

  // Exclusive does not include sub-benchmarks
  u32 exclusive_total;
  u32 exclusive_min;
  u32 exclusive_max;

  // How many times was this element's name benchmarked?
  u32 numEvents;

  const char * name;

  BasicBenchmarkElement(const char * name)
  : inclusive_total(0), inclusive_min(std::numeric_limits<u32>::max()), inclusive_max(0), exclusive_total(0), exclusive_min(std::numeric_limits<u32>::max()), exclusive_max(0), numEvents(0), name(name)
  {
  }
} BasicBenchmarkElement;

staticInline u32 GetBenchmarkTime()
{
#if defined(_MSC_VER)
  static LONGLONG startCounter = 0;

  LARGE_INTEGER counter;

  QueryPerformanceCounter(&counter);

  // Subtract startCounter, so the floating point number has reasonable precision
  if(startCounter == 0) {
    startCounter = counter.QuadPart;
  }

  return static_cast<u32>((counter.QuadPart - startCounter) & 0xFFFFFFFF);
#elif defined(__APPLE_CC__)
  struct timeval time;
#if !defined(ANKI_PLATFORM_IOS)
  // TODO: Fix build error when using this in an iOS build for arm architectures
  gettimeofday(&time, NULL);
#endif
  // Subtract startSeconds, so the floating point number has reasonable precision
  static long startSeconds = 0;
  if(startSeconds == 0) {
    startSeconds = time.tv_sec;
  }

  return (u32)(time.tv_sec-startSeconds)*1000000 + (u32)time.tv_usec;
#elif defined (__EDG__)  // MDK-ARM
  return Anki::Vector::HAL::GetMicroCounter();
#else
  timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u32)ts.tv_sec * 1000000 + (u32)(ts.tv_nsec/1000);
#endif
}

staticInline void AddBenchmarkEvent(const char *name, const u32 time, BenchmarkEventType type)
{
  AnkiAssert(strlen(name) < Anki::Embedded::BenchmarkElement::NAME_LENGTH);

  g_benchmarkEvents[g_numBenchmarkEvents].name = name;
  g_benchmarkEvents[g_numBenchmarkEvents].time = time;
  g_benchmarkEvents[g_numBenchmarkEvents].type = type;

  g_numBenchmarkEvents++;

  // If we run out of space, just keep overwriting the last event
  if(g_numBenchmarkEvents >= Anki::Embedded::MAX_BENCHMARK_EVENTS)
    g_numBenchmarkEvents = Anki::Embedded::MAX_BENCHMARK_EVENTS-1;
}
#endif // ANKI_EMBEDDED_BENCHMARK

namespace Anki
{
  namespace Embedded
  {
#if ANKI_EMBEDDED_BENCHMARK
    ShowBenchmarkParameters::ShowBenchmarkParameters(const char * name, const bool showExclusiveTime, const u8 *color)
    {
      strncpy(this->name, name, BenchmarkElement::NAME_LENGTH);
      this->showExclusiveTime = showExclusiveTime;
      this->red = color[0];
      this->green = color[1];
      this->blue = color[2];
    }

    void InitBenchmarking()
    {
      g_numBenchmarkEvents = 0;
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
      : inclusive_mean(0), inclusive_min(std::numeric_limits<u32>::max()), inclusive_max(0), inclusive_total(0), exclusive_mean(0), exclusive_min(std::numeric_limits<u32>::max()), exclusive_max(0), exclusive_total(0), numEvents(0)
    {
      snprintf(this->name, BenchmarkElement::NAME_LENGTH, "%s", name);
    }

    s32 SnprintElement(char * buffer, const s32 bufferLength, const char * prefix, const s32 number, const char * suffix, const s32 minNumberCharacterToPrint)
    {
      const s32 internalBufferLength = 128;
      char numberBuffer[internalBufferLength];

      SnprintfCommasS32(numberBuffer, internalBufferLength, number);

      s32 numPrinted = 0;

      numPrinted += snprintf(buffer, bufferLength, "%s%s%s", prefix, numberBuffer, suffix);

      const s32 numNumberCharactersPrinted = static_cast<s32>(strlen(numberBuffer));

      for(s32 i=numNumberCharactersPrinted;  i<minNumberCharacterToPrint; i++) {
        if(numPrinted < (bufferLength-1)) {
          buffer[numPrinted] = ' ';
          numPrinted++;
        }
      }

      buffer[numPrinted] = '\0';

      return numPrinted;
    }

    s32 BenchmarkElement::Snprint(char * buffer, const s32 bufferLength, const bool verbose, const bool microseconds, const FixedLengthList<s32> * minCharacterToPrint) const
    {
      s32 numPrintedTotal = 0;
      numPrintedTotal += snprintf(buffer + numPrintedTotal, bufferLength - numPrintedTotal, "%s: ", this->name);

      if(minCharacterToPrint && minCharacterToPrint->get_size() > 0) {
        const s32 numPrinted = static_cast<s32>(strlen(buffer));
        for(s32 i=numPrinted;  i<((*minCharacterToPrint)[0]+2); i++) {
          buffer[numPrintedTotal] = ' ';
          numPrintedTotal++;
        }
      }

      const char * suffix;
      f32 multiplier;
      if(microseconds) {
        suffix = "us ";
        multiplier = 1.0f;
      } else {
        suffix = "ms ";
        multiplier = 1.0f / 1000.0f;
      }

      numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "ExcTot:", Round<s32>(this->exclusive_total*multiplier), suffix, (*minCharacterToPrint)[1]);
      numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "IncTot:", Round<s32>(this->inclusive_total*multiplier), suffix, (*minCharacterToPrint)[2]);
      numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "NEvents:", static_cast<s32>(this->numEvents), " ", (*minCharacterToPrint)[3]);

      if(verbose) {
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "ExcMean:", Round<s32>(this->exclusive_mean*multiplier), suffix, (*minCharacterToPrint)[4]);
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "ExcMin:", Round<s32>(this->exclusive_min*multiplier), suffix, (*minCharacterToPrint)[5]);
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "ExcMax:", Round<s32>(this->exclusive_max*multiplier), suffix, (*minCharacterToPrint)[6]);
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "IncMean:", Round<s32>(this->inclusive_mean*multiplier), suffix, (*minCharacterToPrint)[7]);
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "IncMin:", Round<s32>(this->inclusive_min*multiplier), suffix, (*minCharacterToPrint)[8]);
        numPrintedTotal += SnprintElement(&buffer[numPrintedTotal], bufferLength - numPrintedTotal, "IncMax:", Round<s32>(this->inclusive_max*multiplier), suffix, (*minCharacterToPrint)[9]);
      }

      return numPrintedTotal;
    }

    void BenchmarkElement::Print(const bool verbose, const bool microseconds, const FixedLengthList<s32> * minCharacterToPrint) const
    {
      const s32 bufferLength = 1024;
      char buffer[bufferLength];

      Snprint(buffer, bufferLength, verbose, microseconds, minCharacterToPrint);

      CoreTechPrint("%s", buffer);
    }

    class CompileBenchmarkResults
    {
    public:

      static FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(const s32 numBenchmarkEvents, const BenchmarkEvent * benchmarkEvents, MemoryStack &memory)
      {
        if(numBenchmarkEvents == 0) {
          return FixedLengthList<BenchmarkElement>(0, memory, Flags::Buffer(false, false, false));
        }

        AnkiConditionalErrorAndReturnValue(numBenchmarkEvents > 0 && numBenchmarkEvents < MAX_BENCHMARK_EVENTS,
          FixedLengthList<BenchmarkElement>(), "ComputeBenchmarkResults", "Invalid numBenchmarkEvents");

        FixedLengthList<BenchmarkElement> outputResults(numBenchmarkEvents/2, memory, Flags::Buffer(false, false, false));

#if defined(_MSC_VER)
        LARGE_INTEGER frequency;
        f64 frequencyF64;
        QueryPerformanceFrequency(&frequency);
        frequencyF64 = (f64)(frequency.QuadPart);
#endif

        {
          PUSH_MEMORY_STACK(memory);

          FixedLengthList<BasicBenchmarkElement> basicOutputResults(numBenchmarkEvents/2, memory, Flags::Buffer(false, false, false));
          FixedLengthList<BenchmarkInstance> fullList(numBenchmarkEvents/2, memory, Flags::Buffer(false, false, false));
          FixedLengthList<BenchmarkInstance> parseStack(numBenchmarkEvents/2, memory, Flags::Buffer(false, false, false));

          AnkiConditionalErrorAndReturnValue(AreValid(outputResults, basicOutputResults, fullList, parseStack),
            FixedLengthList<BenchmarkElement>(), "ComputeBenchmarkResults", "Out of memory");

          BenchmarkInstance * pParseStack = parseStack.Pointer(0);

          // First, parse each event individually, and add it to the fullList
          for(s32 iEvent=0; iEvent<numBenchmarkEvents; iEvent++) {
#if defined(_MSC_VER)
            const u32 timeU32 = static_cast<u32>(1000000.0 * (static_cast<f64>(benchmarkEvents[iEvent].time) / frequencyF64));
#elif defined(__APPLE_CC__)
            const u32 timeU32 = benchmarkEvents[iEvent].time;
#elif defined(__EDG__)  // MDK-ARM
            const u32 timeU32 = benchmarkEvents[iEvent].time;
#else
            const u32 timeU32 = benchmarkEvents[iEvent].time;
#endif

            if(benchmarkEvents[iEvent].type == BENCHMARK_EVENT_BEGIN) {
              const s32 startLevel = parseStack.get_size();

              parseStack.PushBack(BenchmarkInstance(benchmarkEvents[iEvent].name, timeU32, 0, 0, startLevel));
            } else {
              // BENCHMARK_EVENT_END

              const BenchmarkInstance startInstance = parseStack.PopBack();

              const s32 endLevel = parseStack.get_size();

              AnkiConditionalErrorAndReturnValue(strcmp(startInstance.name, benchmarkEvents[iEvent].name) == 0 && startInstance.level == endLevel,
                FixedLengthList<BenchmarkElement>(), "ComputeBenchmarkResults", "Benchmark parse error: Perhaps BeginBenchmark() and EndBenchmark() were nested, or there were more than %d benchmark events, or InitBenchmarking() was called in between a StartBenchmarking() and EndBenchmarking() pair, or some other non-supported thing listed in the comments.", MAX_BENCHMARK_EVENTS);

              const u32 elapsedTime = timeU32 - startInstance.startTime;

              // Remove the time for this event from the exclusive time for all other events on the stack
              //const s32 numParents = endLevel;
              if(endLevel > 0) {
                pParseStack[endLevel-1].exclusiveTimeElapsed -= elapsedTime;
              }

              fullList.PushBack(BenchmarkInstance(benchmarkEvents[iEvent].name, startInstance.startTime, elapsedTime, elapsedTime + startInstance.exclusiveTimeElapsed, endLevel));
            }
          } // for(s32 iEvent=0; iEvent<numBenchmarkEvents; iEvent++)

          // Second, combine the events in fullList to create the total outputResults
          const s32 numFullList = fullList.get_size();
          const BenchmarkInstance * pFullList = fullList.Pointer(0);
          BasicBenchmarkElement * pBasicOutputResults = basicOutputResults.Pointer(0);

          //fullList.Print("fullList");

          for(s32 iInstance=0; iInstance<numFullList; iInstance++) {
            //for(s32 iInstance=numFullList-1; iInstance<numFullList; iInstance++) {
            const char * const curName = pFullList[iInstance].name;
            const u32 curInclusiveTimeElapsed = pFullList[iInstance].inclusiveTimeElapsed;
            const u32 curExclusiveTimeElapsed = pFullList[iInstance].exclusiveTimeElapsed;

            s32 index = CompileBenchmarkResults::GetNameIndex(curName, basicOutputResults);

            // If this name hasn't been used yet, add it
            //if(index < 0 && iInstance == (numFullList-1)) {
            if(index < 0) {
              basicOutputResults.PushBack(curName);
              index = basicOutputResults.get_size() - 1;
              AnkiAssert(index == CompileBenchmarkResults::GetNameIndex(curName, basicOutputResults));
            }

            pBasicOutputResults[index].inclusive_min = MIN(pBasicOutputResults[index].inclusive_min, curInclusiveTimeElapsed);
            pBasicOutputResults[index].inclusive_max = MAX(pBasicOutputResults[index].inclusive_max, curInclusiveTimeElapsed);
            pBasicOutputResults[index].inclusive_total += curInclusiveTimeElapsed;

            pBasicOutputResults[index].exclusive_min = MIN(pBasicOutputResults[index].exclusive_min, curExclusiveTimeElapsed);
            pBasicOutputResults[index].exclusive_max = MAX(pBasicOutputResults[index].exclusive_max, curExclusiveTimeElapsed);
            pBasicOutputResults[index].exclusive_total += curExclusiveTimeElapsed;

            pBasicOutputResults[index].numEvents++;
          } // for(s32 iInstance=0; iInstance<numFullList; iInstance++)

          const s32 numEvents = basicOutputResults.get_size();
          outputResults.set_size(numEvents);

          BenchmarkElement * pOutputResults = outputResults.Pointer(0);

          for(s32 iEvent=0; iEvent<numEvents; iEvent++) {
            pOutputResults[iEvent].inclusive_min = pBasicOutputResults[iEvent].inclusive_min;
            pOutputResults[iEvent].inclusive_max = pBasicOutputResults[iEvent].inclusive_max;
            pOutputResults[iEvent].inclusive_total = pBasicOutputResults[iEvent].inclusive_total;
            pOutputResults[iEvent].exclusive_min = pBasicOutputResults[iEvent].exclusive_min;
            pOutputResults[iEvent].exclusive_max = pBasicOutputResults[iEvent].exclusive_max;
            pOutputResults[iEvent].exclusive_total = pBasicOutputResults[iEvent].exclusive_total;
            pOutputResults[iEvent].numEvents = pBasicOutputResults[iEvent].numEvents;

            snprintf(pOutputResults[iEvent].name, BenchmarkElement::NAME_LENGTH, "%s", pBasicOutputResults[iEvent].name);

            if(pOutputResults[iEvent].numEvents == 1) {
              pOutputResults[iEvent].inclusive_mean = pOutputResults[iEvent].inclusive_total;
              pOutputResults[iEvent].exclusive_mean = pOutputResults[iEvent].exclusive_total;
            } else {
              pOutputResults[iEvent].inclusive_mean = pOutputResults[iEvent].inclusive_total / pOutputResults[iEvent].numEvents;
              pOutputResults[iEvent].exclusive_mean = pOutputResults[iEvent].exclusive_total / pOutputResults[iEvent].numEvents;
            }
          } // for(s32 iEvent=0; iEvent<numEvents; iEvent++)
        } // PUSH_MEMORY_STACK(memory);

        outputResults.Resize(outputResults.get_size(), memory);

        return outputResults;
      } // ComputeBenchmarkResults()

      // Returns the index of "name", or -1 if it isn't found
      // compares with strcmp()
      static s32 GetNameIndex(const char * name, const FixedLengthList<BenchmarkElement> &outputResults)
      {
        const BenchmarkElement * pOutputResults = outputResults.Pointer(0);
        const s32 numOutputResults = outputResults.get_size();

        for(s32 i=numOutputResults-1; i>=0; i--) {
          if(strcmp(name, pOutputResults[i].name) == 0) {
            return i;
          }
        } // for(s32 i=0; i<numOutputResults; i++)

        return -1;
      } // GetNameIndex()

      // Returns the index of "name", or -1 if it isn't found
      // compares the raw pointer addresses
      static s32 GetNameIndex(const char * name, const FixedLengthList<BasicBenchmarkElement> &basicOutputResults)
      {
        const BasicBenchmarkElement * pOutputResults = basicOutputResults.Pointer(0);
        const s32 numBasicOutputResults = basicOutputResults.get_size();

        for(s32 i=numBasicOutputResults-1; i>=0; i--) {
          if(reinterpret_cast<size_t>(name) == reinterpret_cast<size_t>(pOutputResults[i].name)) {
            return i;
          }
        } // for(s32 i=0; i<numOutputResults; i++)

        return -1;
      } // GetNameIndex()

    protected:
      typedef struct BenchmarkInstance
      {
        u32 startTime; // A standard timestamp for the parseStack
        u32 inclusiveTimeElapsed; // An elapsed inclusive time for the fullList
        u32 exclusiveTimeElapsed; //< May be negative while the algorithm is still parsing
        s32 level; //< Level 0 is the base level. Every sub-benchmark adds another level.

        const char * name;

        BenchmarkInstance(const char * name, const u32 startTime, const u32 inclusiveTimeElapsed, u32 exclusiveTimeElapsed, const s32 level)
          : startTime(startTime), inclusiveTimeElapsed(inclusiveTimeElapsed), exclusiveTimeElapsed(exclusiveTimeElapsed), level(level), name(name)
        {
        }

        void Print() const
        {
          const s32 bufferLength = 128;
          char buffer[bufferLength];

          //CoreTechPrint("Exclusive Mean:%fs Exclusive Min:%fs Exclusive Max:%fs Inclusive Mean:%fs Inclusive Min:%fs Inclusive Max:%fs (Exclusive Total:%fs Inclusive Total:%fs NumEvents:%d)\n",

          CoreTechPrint("%s: ", this->name);

          SnprintfCommasS32(buffer, bufferLength, this->exclusiveTimeElapsed);
          CoreTechPrint("exclusiveTimeElapsed:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, this->inclusiveTimeElapsed);
          CoreTechPrint("inclusiveTimeElapsed:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, this->startTime);
          CoreTechPrint("startTime:%sus ", buffer);

          SnprintfCommasS32(buffer, bufferLength, this->level);
          CoreTechPrint("NumEvents:%s\n", buffer);
        }
      } BenchmarkInstance;
    }; // class CompileBenchmarkResults

    FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory)
    {
      return CompileBenchmarkResults::ComputeBenchmarkResults(g_numBenchmarkEvents, g_benchmarkEvents, memory);
    } // FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory)

    Result PrintBenchmarkResults(const FixedLengthList<BenchmarkElement> &results, const bool verbose, const bool microseconds)
    {
      if(results.get_size() == 0) {
        return RESULT_OK;
      }

      const s32 scratchBufferLength = 128;
      char scratchBuffer[scratchBufferLength];

      const s32 stringBufferLength = 128;
      char stringBuffer[stringBufferLength];

      AnkiConditionalErrorAndReturnValue(AreValid(results),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Invalid objects");

      MemoryStack scratch(scratchBuffer, scratchBufferLength);

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Out of memory");

      FixedLengthList<s32> minCharacterToPrint(10, scratch, Flags::Buffer(true, false, true));

      AnkiConditionalErrorAndReturnValue(minCharacterToPrint.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "PrintBenchmarkResults", "Out of memory");

      CoreTechPrint("Benchmark Results: (Exc=Exclusive Inc=Inclusive Tot=Total N=NumberOf)\n");

      const s32 numResults = results.get_size();
      const BenchmarkElement * const pResults = results.Pointer(0);

      f32 multiplier;
      if(microseconds) {
        multiplier = 1.0f;
      } else {
        multiplier = 1.0f / 1000.0f;
      }

      for(s32 x=0; x<numResults; x++) {
        const s32 values[] = {
          Round<s32>(pResults[x].exclusive_total*multiplier),
          Round<s32>(pResults[x].inclusive_total*multiplier),
          static_cast<s32>(pResults[x].numEvents),
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
        CoreTechPrint("\n");
      }

      CoreTechPrint("\n");

      return RESULT_OK;
    }

    Result ComputeAndPrintBenchmarkResults(const bool verbose, const bool microseconds, MemoryStack scratch)
    {
      FixedLengthList<BenchmarkElement> results = ComputeBenchmarkResults(scratch);

      AnkiConditionalErrorAndReturnValue(AreValid(scratch, results),
        RESULT_FAIL_INVALID_OBJECT, "PrintBenchmarkResults", "Invalid objects");

      return PrintBenchmarkResults(results, verbose, microseconds);
    }

    Result ShowBenchmarkResults(
      const FixedLengthList<BenchmarkElement> &results,
      const FixedLengthList<ShowBenchmarkParameters> &namesToDisplay,
      const f32 pixelsPerMillisecond,
      const s32 imageHeight,
      const s32 imageWidth)
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      static cv::Mat toShowImage;
      static s32 toShowImageColumn;
      static bool blinkOn = true;

      const s32 blackWidth = 10;
      //const s32 displayGridEveryNMilliseconds = 50;
      const s32 blinkerWidth = 7;

      const s32 totalTimeIndex = CompileBenchmarkResults::GetNameIndex("TotalTime", results);

      AnkiConditionalErrorAndReturnValue(totalTimeIndex >= 0,
        RESULT_FAIL, "ShowBenchmarkResults", "Requires a \"TotalTime\" benchmark event");

      AnkiConditionalErrorAndReturnValue(namesToDisplay.get_size() > 0,
        RESULT_FAIL_INVALID_PARAMETER, "ShowBenchmarkResults", "namesToDisplay must be have 1 or more names");

      if(toShowImage.rows != imageHeight || toShowImage.cols != imageWidth) {
        toShowImage = cv::Mat(imageHeight, imageWidth, CV_8UC3);
        toShowImage.setTo(0);
        toShowImageColumn = 0;
      }

      for(s32 iX=0; iX<blackWidth; iX++) {
        //const s32 x = MIN((toShowImageColumn + iX), imageWidth-1);
        //cv::rectangle(toShowImage, cv::Rect(x1,0,blackWidth,imageHeight-1), cv::Scalar(0,0,0), CV_FILLED);

        const s32 x = (toShowImageColumn + iX) % imageWidth;
        cv::line(
          toShowImage,
          cv::Point(x, 0),
          cv::Point(x, imageHeight - 1),
          cv::Scalar(0,0,0),
          1,
          4);
      }

      // Draw the total time as gray
      const s32 numPixelsTotal = Round<s32>(pixelsPerMillisecond * results[totalTimeIndex].inclusive_total / 1000.0f);
      cv::line(
        toShowImage,
        cv::Point(toShowImageColumn, imageHeight - 1),
        cv::Point(toShowImageColumn, MAX(0, imageHeight - numPixelsTotal)),
        cv::Scalar(128, 128, 128),
        1,
        4);

      // Draw the specific benchmarks as colors
      s32 curY = imageHeight - 1;
      for(s32 iName=0; iName<namesToDisplay.get_size(); iName++) {
        const s32 index = CompileBenchmarkResults::GetNameIndex(namesToDisplay[iName].name, results);

        if(index < 0)
          continue;

        const u32 timeElapsed = namesToDisplay[iName].showExclusiveTime ? results[index].exclusive_total : results[index].inclusive_total;

        const s32 numPixels = Round<s32>(pixelsPerMillisecond * timeElapsed / 1000.0f);

        cv::line(
          toShowImage,
          cv::Point(toShowImageColumn, curY),
          cv::Point(toShowImageColumn, MAX(0, curY-numPixels+1)),
          cv::Scalar(namesToDisplay[iName].blue, namesToDisplay[iName].green, namesToDisplay[iName].red),
          1,
          4);

        curY -= numPixels;
      } // for(s32 iName=0; iName<namesToDisplay.get_size(); iName++)

      const s32 numGridlines = static_cast<s32>( floor(imageHeight / (50.0f * pixelsPerMillisecond)) ) + 1;
      for(s32 i=0; i<numGridlines; i++) {
        const s32 curY = imageHeight - 1 - Round<s32>(50.0f * pixelsPerMillisecond * i);
        cv::line(
          toShowImage,
          cv::Point(0, curY),
          cv::Point(imageWidth-1, curY),
          cv::Scalar(196,196,196),
          1,
          4);
      }

      cv::imshow("Benchmarks", toShowImage);
      cv::moveWindow("Benchmarks", 150, 0);

      toShowImageColumn++;

      if(toShowImageColumn >= imageWidth)
        toShowImageColumn = 0;

      // Display all the benchmarks ( copied from Print() )
      {
        const s32 textHeightInPixels = 20;
        const f32 textFontSize = 1.0f;
        const bool microseconds = false;
        const bool verbose = false;

        const s32 textImageHeight = (results.get_size()+1)*textHeightInPixels + 10;
        const s32 textImageWidth = 1400;
        cv::Mat textImage(textImageHeight, textImageWidth, CV_8UC3);
        textImage.setTo(0);

        if(blinkOn) {
          //Draw a blinky rectangle at the upper right

          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=textImageWidth-blinkerWidth; x<textImageWidth; x++) {
              textImage.at<u8>(y,x*3) = 0;
              textImage.at<u8>(y,x*3+1) = 0;
              textImage.at<u8>(y,x*3+2) = 0;
            }
          }

          for(s32 y=1; y<blinkerWidth-1; y++) {
            for(s32 x=textImageWidth+1-blinkerWidth; x<(textImageWidth-1); x++) {
              textImage.at<u8>(y,x*3) = 255;
              textImage.at<u8>(y,x*3+1) = 255;
              textImage.at<u8>(y,x*3+2) = 255;
            }
          }
        } else { // if(blinkOn)
          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=textImageWidth-blinkerWidth; x<textImageWidth; x++) {
              textImage.at<u8>(y,x*3) = 0;
              textImage.at<u8>(y,x*3+1) = 0;
              textImage.at<u8>(y,x*3+2) = 0;
            }
          }
        } // if(blinkOn) ... else

        s32 curY = 15;

        const s32 stringBufferLength = 256;
        char stringBuffer[stringBufferLength];

        const s32 scratchBufferLength = 128;
        char scratchBuffer[scratchBufferLength];

        MemoryStack scratch(scratchBuffer, scratchBufferLength);

        AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "ShowBenchmarkResults", "Out of memory");

        FixedLengthList<s32> minCharacterToPrint(10, scratch, Flags::Buffer(true, false, true));

        AnkiConditionalErrorAndReturnValue(minCharacterToPrint.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "ShowBenchmarkResults", "Out of memory");

        snprintf(stringBuffer, stringBufferLength, "Benchmark Results: (Exc=Exclusive Inc=Inclusive Tot=Total N=NumberOf)\n");
        cv::putText(textImage, stringBuffer, cv::Point(5,curY), cv::FONT_HERSHEY_PLAIN, textFontSize, cv::Scalar(255, 255, 255));
        curY += textHeightInPixels;

        const s32 numResults = results.get_size();
        const BenchmarkElement * const pResults = results.Pointer(0);

        f32 multiplier;
        if(microseconds) {
          multiplier = 1.0f;
        } else {
          multiplier = 1.0f / 1000.0f;
        }

        for(s32 x=0; x<numResults; x++) {
          const s32 values[] = {
            Round<s32>(pResults[x].exclusive_total*multiplier),
            Round<s32>(pResults[x].inclusive_total*multiplier),
            static_cast<s32>(pResults[x].numEvents),
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
          pResults[x].Snprint(stringBuffer, stringBufferLength, verbose, microseconds, &minCharacterToPrint);
          CvPutTextFixedWidth(textImage, stringBuffer, cv::Point(5,curY), cv::FONT_HERSHEY_PLAIN, textFontSize, cv::Scalar(255, 255, 255));
          curY += textHeightInPixels;
        }

        cv::imshow("All Benchmarks", textImage);
        //cv::moveWindow("All Benchmarks", 850, 675);
      }

      // Display the key
      {
        const s32 textHeightInPixels = 20;
        const f32 textFontSize = 1.0f;

        const s32 keyImageHeight = (namesToDisplay.get_size()+1)*textHeightInPixels;
        const s32 keyImageWidth = 640;
        cv::Mat keyImage(keyImageHeight, keyImageWidth, CV_8UC3);
        keyImage.setTo(0);

        if(blinkOn) {
          //Draw a blinky rectangle at the upper right

          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=keyImageWidth-blinkerWidth; x<keyImageWidth; x++) {
              keyImage.at<u8>(y,x*3) = 0;
              keyImage.at<u8>(y,x*3+1) = 0;
              keyImage.at<u8>(y,x*3+2) = 0;
            }
          }

          for(s32 y=1; y<blinkerWidth-1; y++) {
            for(s32 x=keyImageWidth+1-blinkerWidth; x<(keyImageWidth-1); x++) {
              keyImage.at<u8>(y,x*3) = 255;
              keyImage.at<u8>(y,x*3+1) = 255;
              keyImage.at<u8>(y,x*3+2) = 255;
            }
          }
        } else { // if(blinkOn)
          for(s32 y=0; y<blinkerWidth; y++) {
            for(s32 x=keyImageWidth-blinkerWidth; x<keyImageWidth; x++) {
              keyImage.at<u8>(y,x*3) = 0;
              keyImage.at<u8>(y,x*3+1) = 0;
              keyImage.at<u8>(y,x*3+2) = 0;
            }
          }
        } // if(blinkOn) ... else

        s32 curY = 15;

        const s32 textBufferLength = 256;
        char textBuffer[textBufferLength];

        snprintf(textBuffer, textBufferLength, "Total: %dms", Round<s32>(results[totalTimeIndex].inclusive_total / 1000.0f));
        cv::putText(keyImage, textBuffer, cv::Point(5,curY), cv::FONT_HERSHEY_PLAIN, textFontSize, cv::Scalar(128, 128, 128));
        curY += textHeightInPixels;

        for(s32 iKey=namesToDisplay.get_size() - 1; iKey>=0; iKey--) {
          const s32 index = CompileBenchmarkResults::GetNameIndex(namesToDisplay[iKey].name, results);

          u32 time;
          if(index == -1) {
            time = 0;
          } else {
            time = namesToDisplay[iKey].showExclusiveTime ? results[index].exclusive_total : results[index].inclusive_total;
          }

          snprintf(textBuffer, textBufferLength,  "%s: %dms", namesToDisplay[iKey].name, Round<s32>(time/1000.0f));
          cv::putText(keyImage, textBuffer, cv::Point(5,curY), cv::FONT_HERSHEY_PLAIN, textFontSize, cv::Scalar(namesToDisplay[iKey].blue, namesToDisplay[iKey].green, namesToDisplay[iKey].red));
          curY += textHeightInPixels;
        }

        cv::imshow("Benchmarks Key", keyImage);
        cv::moveWindow("Benchmarks Key", 850, 540);
      }

      blinkOn = !blinkOn;

      return RESULT_OK;
#else // #if ANKICORETECH_EMBEDDED_USE_OPENCV
      return RESULT_FAIL;
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV ... #else
    }

    s32 GetNameIndex(const char * name, const FixedLengthList<BenchmarkElement> &outputResults)
    {
      return CompileBenchmarkResults::GetNameIndex(name, outputResults);
    }

    Result PrintPercentileBenchmark(const FixedLengthList<FixedLengthList<BenchmarkElement> > &benchmarkElements, const s32 numRuns, const f32 elementPercentile, MemoryStack scratch)
    {
      // Check that all the lists have the same number and type of benchmarks
      const s32 numElements = benchmarkElements[0].get_size();
      for(s32 iRun=1; iRun<numRuns; iRun++) {
        if(benchmarkElements[0].get_size() != benchmarkElements[iRun].get_size()) { AnkiError("PrintPercentileBenchmark", "number failure %d!=%d", benchmarkElements[0].get_size(), benchmarkElements[iRun].get_size()); return RESULT_FAIL; }
        for(s32 i=0; i<numElements; i++) {
          if(benchmarkElements[0][i].numEvents != benchmarkElements[iRun][i].numEvents) { AnkiError("PrintPercentileBenchmark", "number failure %d!=%d (%s,%s)", benchmarkElements[0][i].numEvents, benchmarkElements[iRun][i].numEvents, benchmarkElements[0][i].name, benchmarkElements[iRun][i].name); return RESULT_FAIL; }
          if(strcmp(benchmarkElements[0][i].name, benchmarkElements[iRun][i].name) != 0) { AnkiError("PrintPercentileBenchmark", "name failure %s!=%s", benchmarkElements[0][i].name, benchmarkElements[iRun][i].name); return RESULT_FAIL; }
        }
      }

      // Compute the medians
      FixedLengthList<BenchmarkElement> medianBenchmarkElements(benchmarkElements[0].get_size(), scratch, Flags::Buffer(true, false, true));
      Array<u32> sortedElements(1, numRuns, scratch);
      u32 * pSortedElements = sortedElements.Pointer(0,0);
      for(s32 i=0; i<numElements; i++) {
        strncpy(medianBenchmarkElements[i].name, benchmarkElements[0][i].name, BenchmarkElement::NAME_LENGTH - 1);
        medianBenchmarkElements[i].numEvents = benchmarkElements[0][i].numEvents;

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].inclusive_mean; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].inclusive_mean = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].inclusive_min; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].inclusive_min = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].inclusive_max; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].inclusive_max = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].inclusive_total; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].inclusive_total = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].exclusive_mean; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].exclusive_mean = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].exclusive_min; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].exclusive_min = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].exclusive_max; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].exclusive_max = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];

        for(s32 iRun=0; iRun<numRuns; iRun++) { pSortedElements[iRun] = benchmarkElements[iRun][i].exclusive_total; }
        if(Matrix::InsertionSort<u32>(sortedElements, 1) != RESULT_OK) { AnkiError("PrintPercentileBenchmark", "sort failure"); return RESULT_FAIL; }
        medianBenchmarkElements[i].exclusive_total = pSortedElements[saturate_cast<s32>(numRuns*elementPercentile)];
      } // for(s32 i=0; i<numElements; i++)

      PrintBenchmarkResults(medianBenchmarkElements, true, true);

      return RESULT_OK;
    } // CompueMedianBenchmark()
#else
#endif // ANKI_EMBEDDED_BENCHMARK
  } // namespace Embedded
} // namespace Anki
