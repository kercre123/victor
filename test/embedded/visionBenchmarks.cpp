//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"
#include "visionBenchmarks.h"

using namespace Anki::Embedded;

#if(defined(_MSC_VER) || defined(__APPLE__))
#include <string.h>
#else
static void memset(void * dst, int value, size_t size)
{
  size_t i;
  for(i=0; i<size; i++)
  {
    ((char*)dst)[i] = value;
  }
}
#endif

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECH_USE_MATLAB)
Matlab matlab(false);
#endif

#define CHECK_FOR_ERRORS

// If both are commented, it is buffer in DDR without L2
// NOTE: Cannot use both CMX and L2 Cache
//#define BUFFER_IN_DDR_WITH_L2
//#define BUFFER_IN_CMX

#if defined(BUFFER_IN_DDR_WITH_L2) && defined(BUFFER_IN_CMX)
You cannot use both CMX and L2 Cache;
#endif

//#define MAX_BYTES 5000000
#define MAX_BYTES 100000

#ifdef _MSC_VER
static char buffer[MAX_BYTES];
#else

#ifdef BUFFER_IN_CMX
static char buffer[MAX_BYTES] __attribute__((section(".cmx.bss,CMX")));
#else // #ifdef BUFFER_IN_CMX

#ifdef BUFFER_IN_DDR_WITH_L2
static char buffer[MAX_BYTES] __attribute__((section(".ddr.bss,DDR"))); // With L2 cache
#else
static char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss,DDR_DIRECT"))); // No L2 cache
#endif

#endif // #ifdef BUFFER_IN_CMX ... #else

//static char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss"))); // No L2 cache

#endif // #ifdef USING_MOVIDIUS_COMPILER

static const s32 width = 640;
//static const s32 height = 480;
static const s32 height = 10;
static const s32 numBytes = MIN(MAX_BYTES, 5000000);

int BenchmarkBinomialFilter()
{
  MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms, false);
  Array<u8> imageFiltered(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
  DASConditionalErrorAndReturnValue(imageFiltered.IsValid(), -3, "imageFiltered.IsValid()", "");
#endif

  Result result = BinomialFilter(image, imageFiltered, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkDownsampleByFactor()
{
  const s32 downsampleFactor = 2;

  MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms);
  Array<u8> imageDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
  DASConditionalErrorAndReturnValue(imageDownsampled.IsValid(), -3, "imageDownsampled.IsValid()", "");
#endif

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = DownsampleByFactor(image, downsampleFactor, imageDownsampled);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkComputeCharacteristicScale()
{
  const s32 numPyramidLevels = 6;

  MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
#endif

  Array<u32> scaleImage(height, width, ms);
#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(scaleImage.IsValid(), -2, "scaleImage.IsValid()", "");
#endif

  Result result = ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkTraceBoundary()
{
  MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> binaryImage(height, width, ms);
  const Point<s16> startPoint(3,3);
  const BoundaryDirection initialDirection = BOUNDARY_N;
  FixedLengthList<Point<s16> > boundary(MAX_BOUNDARY_LENGTH, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(binaryImage.IsValid(), -2, "binaryImage.IsValid()", "");
  DASConditionalErrorAndReturnValue(boundary.IsValid(), -3, "boundary.IsValid()", "");
#endif

  // This is a thin, wide, hollow rectangle in the middle of the image
  // TODO: check if this initialization takes a long time, relative to the tracing kernel
  {
    // Clear out a black area
    for(s32 y=0; y<=8; y++) {
      memset(binaryImage.Pointer(y, 0), 0, binaryImage.get_stride());
    }

    // Top edge
    for(s32 y=2; y<=3; y++) {
      memset(binaryImage.Pointer(y, 0) + 3, 1, width-6);
    }

    //Bottom edge
    for(s32 y=6; y<=7; y++) {
      memset(binaryImage.Pointer(y, 0) + 3, 1, width-6);
    }

    //Sides
    for(s32 y=4; y<=5; y++) {
      u8 * restrict rowPointer = binaryImage.Pointer(y, 0);
      for(s32 x=3; x<=4; x++) {
        rowPointer[x] = 1;
      }

      for(s32 x=width-5; x<(width-3); x++) {
        rowPointer[x] = 1;
      }
    }
  }

#if defined(ANKI_DEBUG_HIGH) && defined(ANKICORETECH_USE_MATLAB)
  Matlab matlab;

  matlab.PutArray<u8>(binaryImage, "binaryImage");
#endif // #if defined(ANKI_DEBUG_HIGH) && defined(ANKICORETECH_USE_MATLAB)

  Result result = TraceBoundary(binaryImage, startPoint, initialDirection, boundary);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}