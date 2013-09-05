//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"
#include "visionBenchmarks.h"

#ifdef _MSC_VER
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
Anki::Embedded::Matlab matlab(false);
#endif

#define CHECK_FOR_ERRORS

// If both are commented, it is buffer in DDR without L2
// NOTE: Cannot use both CMX and L2 Cache
#define BUFFER_IN_DDR_WITH_L2
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
static char buffer[MAX_BYTES] __attribute__((section(".cmx.bss")));
#else // #ifdef BUFFER_IN_CMX

#ifdef BUFFER_IN_DDR_WITH_L2
static char buffer[MAX_BYTES] __attribute__((section(".ddr.bss"))); // With L2 cache
#else
static char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss"))); // No L2 cache
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
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 img(height, width, ms, false);
  Anki::Embedded::Array_u8 imgFiltered(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(img.IsValid(), -2, "img.IsValid()", "");
  DASConditionalErrorAndReturnValue(imgFiltered.IsValid(), -3, "imgFiltered.IsValid()", "");
#endif

  Anki::Embedded::Result result = Anki::Embedded::BinomialFilter(img, imgFiltered, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == Anki::Embedded::RESULT_OK, -4, "result == Anki::Embedded::RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkDownsampleByFactor()
{
  const s32 downsampleFactor = 2;

  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 img(height, width, ms);
  Anki::Embedded::Array_u8 imgDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(img.IsValid(), -2, "img.IsValid()", "");
  DASConditionalErrorAndReturnValue(imgDownsampled.IsValid(), -3, "imgDownsampled.IsValid()", "");
#endif

  for(s32 x=0; x<width; x++) {
    *img.Pointer(2,x) = static_cast<u8>(x);
  }

  Anki::Embedded::Result result = Anki::Embedded::DownsampleByFactor(img, downsampleFactor, imgDownsampled);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == Anki::Embedded::RESULT_OK, -4, "result == Anki::Embedded::RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkComputeCharacteristicScale()
{
  const s32 numLevels = 6;

  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 img(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(img.IsValid(), -2, "img.IsValid()", "");
#endif

  Anki::Embedded::Array_u32 scaleImage(height, width, ms);
#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(scaleImage.IsValid(), -2, "scaleImage.IsValid()", "");
#endif

  Anki::Embedded::Result result = ComputeCharacteristicScaleImage(img, numLevels, scaleImage, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == Anki::Embedded::RESULT_OK, -4, "result == Anki::Embedded::RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkTraceBoundary()
{
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 binaryImg(height, width, ms);
  const Anki::Embedded::Point_s16 startPoint(3,200);
  const Anki::Embedded::BoundaryDirection initialDirection = Anki::Embedded::BOUNDARY_N;
  Anki::Embedded::FixedLengthList_Point_s16 boundary(Anki::Embedded::MAX_BOUNDARY_LENGTH, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(binaryImg.IsValid(), -2, "binaryImg.IsValid()", "");
  DASConditionalErrorAndReturnValue(boundary.IsValid(), -3, "boundary.IsValid()", "");
#endif

  // This is a thin, wide, hollow rectangle in the middle of the image
  // TODO: check if this initialization takes a long time, relative to the tracing kernel
  {
    // Clear out a black area
    for(s32 y=0; y<=8; y++) {
      memset(binaryImg.Pointer(y, 0), 0, binaryImg.get_stride());
    }

    // Top edge
    for(s32 y=2; y<=3; y++) {
      memset(binaryImg.Pointer(y, 0) + 3, 1, width-6);
    }

    //Bottom edge
    for(s32 y=6; y<=7; y++) {
      memset(binaryImg.Pointer(y, 0) + 3, 1, width-6);
    }

    //Sides
    for(s32 y=4; y<=5; y++) {
      u8 * restrict rowPointer = binaryImg.Pointer(y, 0);
      for(s32 x=3; x<=4; x++) {
        rowPointer[x] = 1;
      }

      for(s32 x=width-5; x<(width-3); x++) {
        rowPointer[x] = 1;
      }
    }
  }

#if defined(ANKI_DEBUG_HIGH) && defined(ANKICORETECH_USE_MATLAB)
  Anki::Embedded::Matlab matlab;

  matlab.PutArray_u8(binaryImg, "binaryImg");
#endif // #if defined(ANKI_DEBUG_HIGH) && defined(ANKICORETECH_USE_MATLAB)

  Anki::Embedded::Result result = Anki::Embedded::TraceBoundary(binaryImg, startPoint, initialDirection, boundary);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(result == Anki::Embedded::RESULT_OK, -4, "result == Anki::Embedded::RESULT_OK", "");
#endif

  return 0;
}