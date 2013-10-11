#ifndef __EMBEDDED_VISION_BENCHMARKS_H_
#define __EMBEDDED_VISION_BENCHMARKS_H_

#if defined(__cplusplus) && !defined(_MSC_VER)
extern "C" {
#endif

  int BenchmarkSimpleDetector_Steps12345_realImage(int numIterations);
  int BenchmarkBinomialFilter();
  int BenchmarkDownsampleByFactor();
  int BenchmarkComputeCharacteristicScale();
  int BenchmarkTraceInteriorBoundary();

#if defined(__cplusplus) && !defined(_MSC_VER)
}
#endif

#endif // __EMBEDDED_VISION_BENCHMARKS_H_
