#ifndef _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_
#define _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_

#if !defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__==2 && __GNUC_PATCHLEVEL__==1 //hack to detect the movidius compiler
#warning Using GNUC 4.2.1
#define USING_MOVIDIUS_SHAVE_COMPILER
#define USING_MOVIDIUS_COMPILER
#endif

#if !defined(__APPLE_CC__) && defined(__GNUC__) && __GNUC__==4 && __GNUC_MINOR__==4 && __GNUC_PATCHLEVEL__==2 //hack to detect the movidius compiler
#warning Using GNUC 4.4.2
#define USING_MOVIDIUS_GCC_COMPILER
#define USING_MOVIDIUS_COMPILER
#endif

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

  void explicitPrintf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // _ANKICORETECHEMBEDDED_COMMON_BENCHMARKING_H_