/**
File: benchmarkins_c.h
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

#define MAX_BENCHMARK_EVENTS 0xFFFF

#ifdef __cplusplus
extern "C" {
#endif
  // Call this before doing any benchmarking, to clear the buffer of benchmarkEvents.
  // Can be called multiple times.
  void InitBenchmarking();

  // Use these functions to add a new event to the list. These functions are very fast.
  //
  // WARNING: name must be in globally available memory
  //
  // WARNING: nesting BeginBenchmark() and EndBenchmark() events that have the same name won't work.
  // This is okay: BeginBenchmark("a"); BeginBenchmark("b"); EndBenchmark("b"); EndBenchmark("a");
  // This is not okay: BeginBenchmark("a"); BeginBenchmark("a"); EndBenchmark("a"); EndBenchmark("a");
  void BeginBenchmark(const char *name);
  void EndBenchmark(const char *name);

  // Compile and print out all the benchmark events that were recorded
  void PrintBenchmarkResults_All();
  void PrintBenchmarkResults_OnlyTotals();

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_