#ifndef __VISION_BENCHMARKS_H_
#define __VISION_BENCHMARKS_H_

#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

  int BenchmarkBinomialFilter();
  int BenchmarkDownsampleByFactor();
  int BenchmarkComputeCharacteristicScale();

#if defined(__cplusplus) && !defined(_MSC_VER)
}
#endif

#endif // __VISION_BENCHMARKS_H_
