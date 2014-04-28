/**
File: benchmarkins.h
Author: Peter Barnum
Created: 2013

Low-overhead benchmarking, based on a list of start and end events.

The basic use of this benchmarking utility is as follows:
1. InitBenchmarking()
2. At the beginning of the section you want to benchmark, put BeginBenchmark("event type");
3. At the end of the section you want to benchmark, put EndBenchmark("event type");
4. When you're done running the program, call PrintBenchmarkResults() to print the results

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
#define _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    const s32 MAX_BENCHMARK_EVENTS = 0xFFF;

    typedef struct BenchmarkElement
    {
      static const s32 NAME_LENGTH = 64;

      // Inclusive includes all the time for all sub-benchmarks
      f64 inclusive_mean;
      f64 inclusive_min;
      f64 inclusive_max;
      f64 inclusive_total;

      // Exclusive does not include sub-benchmarks
      f64 exclusive_mean;
      f64 exclusive_min;
      f64 exclusive_max;
      f64 exclusive_total;

      //// When was BeginBenchmark first called for this name ?
      //f64 firstInstanceStartTime;

      // How many times was this element's name benchmarked?
      s64 numEvents;

      //// Level 0 is the base level. Every sub-benchmark adds another level.
      ////
      //// For example, take BeginBenchmark("a"); BeginBenchmark("b"); EndBenchmark("b"); EndBenchmark("a")
      //// "a" is level 0, "b" is level 1
      //s32 level;

      char name[BenchmarkElement::NAME_LENGTH];

      BenchmarkElement(const char * name);

      void Print(const bool verbose=true, const bool microseconds=true, const FixedLengthList<s32> * minCharacterToPrint=NULL) const;
    } BenchmarkElement;

    // Call this before doing any benchmarking, to clear the buffer of benchmarkEvents.
    // Can be called multiple times.
    void InitBenchmarking();

    // Use these functions to add a new event to the list. These functions are very fast.
    //
    // WARNING: name must be in globally available memory
    //
    // WARNING: the character string must be less than BenchmarkElement::NAME_LENGTH bytes
    //
    // WARNING: Using the same name for different benchmark events
    //
    // WARNING: nesting BeginBenchmark() and EndBenchmark() events that have the same name won't work.
    // This is okay: BeginBenchmark("a"); BeginBenchmark("b"); EndBenchmark("b"); EndBenchmark("a");
    // This is not okay: BeginBenchmark("a"); BeginBenchmark("a"); EndBenchmark("a"); EndBenchmark("a");
    // This is not okay: BeginBenchmark("a"); BeginBenchmark("b"); EndBenchmark("a"); EndBenchmark("b");
    void BeginBenchmark(const char *name);
    void EndBenchmark(const char *name);

    // Compile all the benchmark events that were recorded
    FixedLengthList<BenchmarkElement> ComputeBenchmarkResults(MemoryStack &memory);

    Result PrintBenchmarkResults(const FixedLengthList<BenchmarkElement> &results, const bool verbose=true, const bool microseconds=true);

    // Compile and print out all the benchmark events that were recorded
    Result ComputeAndPrintBenchmarkResults(const bool verbose=true, const bool microseconds=true);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
