/**
File: benchmarkins_c.h
Author: Peter Barnum
Created: 2013

Low-overhead benchmarking, based on a list of start and end events.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
#define _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_

#include "anki/common/robot/config.h"

#define NUM_BENCHMARK_EVENTS 0xFFF

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
    BENCHMARK_EVENT_BEGIN,
    BENCHMARK_EVENT_END
  } BenchmarkEventType;

  typedef struct
  {
    const char * name; // Warning: name must be in globally available memory
    unsigned long long time;
    BenchmarkEventType type;
  } BenchmarkEvent;

  typedef enum
  {
    BENCHMARK_PRINT_ALL,
    BENCHMARK_PRINT_TOTALS,
  } BenchmarkPrintType;

  extern BenchmarkEvent benchmarkEvents[];
  extern int currentBenchmarkEvent;

  void InitBenchmarking();

  // Warning: name must be in globally available memory
  //
  // Warning: nesting BeginBenchmark() and EndBenchmark() events that have the same name won't work.
  // This is okay: BeginBenchmark("a"); BeginBenchmark("b"); EndBenchmark("b"); EndBenchmark("a");
  // This is not okay: BeginBenchmark("a"); BeginBenchmark("a"); EndBenchmark("a"); EndBenchmark("a");
  void BeginBenchmark(const char *name);
  void EndBenchmark(const char *name);
  void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type);

  void PrintBenchmarkResults(const BenchmarkPrintType printType);

#pragma mark --- Implementations ---

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
