#ifndef _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
#define _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_

#include "anki/embeddedCommon/config.h"

#define NUM_BENCHMARK_EVENTS 0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

  typedef enum
  {
    BENCHMARK_EVENT_BEGIN,
    BENCHMARK_EVENT_END
  } BenchmarkEventType;

  typedef struct {
    const char * name; // Warning: name must be in globally available memory
    unsigned long long time;
    BenchmarkEventType type;
  } BenchmarkEvent;

  extern BenchmarkEvent benchmarkEvents[];
  extern int currentBenchmarkEvent;

  void InitBenchmarking();

  // Warning: name must be in globally available memory
  // Warning: nesting BeginBenchmark() and EndBenchmark() events won't work
  void BeginBenchmark(const char *name);
  void EndBenchmark(const char *name);
  void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type);

  void PrintBenchmarkResults();

#pragma mark --- Implementations ---

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_