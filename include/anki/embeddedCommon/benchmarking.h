#ifndef _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
#define _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_

#if defined(_MSC_VER)
#include <windows.h>
#elif defined(USING_MOVIDIUS_GCC_COMPILER)
// TODO: implement
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
// TODO: implement
#else
#include <sys/time.h>
#endif

#define NUM_BENCHMARK_EVENTS 0xFFFF

#if defined(_MSC_VER)
#ifndef inline
#define inline __forceinline
#endif
#endif

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

  inline void InitBenchmarking();

  // Warning: name must be in globally available memory
  // Warning: nesting BeginBenchmark() and EndBenchmark() events won't work
  inline void BeginBenchmark(const char *name);
  inline void EndBenchmark(const char *name);
  inline void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type);

  void PrintBenchmarkResults();

#pragma mark --- Implementations ---

  inline void InitBenchmarking()
  {
    currentBenchmarkEvent = 0;
  }

  inline unsigned long long GetBenchmarkTime()
  {
#if defined(_MSC_VER)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return counter.QuadPart;
#elif defined(__APPLE_CC__)
    return 0; // TODO: implement
#elif defined(USING_MOVIDIUS_GCC_COMPILER)
    return 0; // TODO: implement
#elif defined(USING_MOVIDIUS_SHAVE_COMPILER)
    return 0; // TODO: implement
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
#endif
  }

  inline void AddBenchmarkEvent(const char *name, unsigned long long time, BenchmarkEventType type)
  {
    benchmarkEvents[currentBenchmarkEvent].name = name;
    benchmarkEvents[currentBenchmarkEvent].time = time;
    benchmarkEvents[currentBenchmarkEvent].type = type;

    currentBenchmarkEvent++;

    // If we run out of space, just keep overwriting the last event
    if(currentBenchmarkEvent >= NUM_BENCHMARK_EVENTS)
      currentBenchmarkEvent = NUM_BENCHMARK_EVENTS-1;
  }

  inline void BeginBenchmark(const char *name)
  {
    AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_BEGIN);
  } // inline void startBenchmark(const char *name)

  inline void EndBenchmark(const char *name)
  {
    AddBenchmarkEvent(name, GetBenchmarkTime(), BENCHMARK_EVENT_END);
  } // inline void endBenchmark(const char *name)

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_