//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"
#include "visionBenchmarks.h"

#include <string.h>

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECH_USE_MATLAB)
Anki::Embedded::Matlab matlab(false);
#endif

#define CHECK_FOR_ERRORS

#define USE_L2_CACHE
#define MAX_BYTES 5000000

#ifdef _MSC_VER
static char buffer[MAX_BYTES];
#else

#ifdef USE_L2_CACHE
static char buffer[MAX_BYTES] __attribute__((section(".ddr.bss"))); // With L2 cache
#else
static char buffer[MAX_BYTES] __attribute__((section(".ddr_direct.bss"))); // No L2 cache
#endif

#endif // #ifdef USING_MOVIDIUS_COMPILER

int BenchmarkBinomialFilter()
{
  const s32 width = 640;
  const s32 height = 480;
  const s32 numBytes = MIN(MAX_BYTES, 5000000);

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
  const s32 width = 640;
  const s32 height = 480;
  const s32 numBytes = MIN(MAX_BYTES, 5000000);

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
  const s32 width = 640;
  const s32 height = 480;
  const s32 numBytes = MIN(MAX_BYTES, 5000000);

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
  const s32 width = 640;
  const s32 height = 480;
  const s32 numBytes = MIN(MAX_BYTES, 5000000);

  Anki::Embedded::MemoryStack ms(buffer, numBytes);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Anki::Embedded::Array_u8 binaryImg(height, width, ms);
  const Anki::Embedded::Point_s16 startPoint(3,2);
  const Anki::Embedded::BoundaryDirection initialDirection = Anki::Embedded::BOUNDARY_N;
  Anki::Embedded::FixedLengthList_Point_s16 boundary(Anki::Embedded::MAX_BOUNDARY_LENGTH, ms);

#ifdef CHECK_FOR_ERRORS
  DASConditionalErrorAndReturnValue(binaryImg.IsValid(), -2, "binaryImg.IsValid()", "");
  DASConditionalErrorAndReturnValue(boundary.IsValid(), -3, "boundary.IsValid()", "");
#endif

  // This is a thin, wide, hollow rectangle in the middle of the image
  {
    for(s32 y=196; y<=206; y++) {
      memset(binaryImg.Pointer(y, 0), 0, binaryImg.get_stride());
    }

    // Top edge
    for(s32 y=198; y<=199; y++) {
      u8 * restrict rowPointer = binaryImg.Pointer(y, 0);
      for(s32 x=3; x<(width-3); x++) {
        rowPointer[x] = 1;
      }
    }

    //Bottom edge
    for(s32 y=202; y<=203; y++) {
      u8 * restrict rowPointer = binaryImg.Pointer(y, 0);
      for(s32 x=3; x<(width-3); x++) {
        rowPointer[x] = 1;
      }
    }

    //Sides
    for(s32 y=200; y<=201; y++) {
      u8 * restrict rowPointer = binaryImg.Pointer(y, 0);
      for(s32 x=3; x<=4; x++) {
        rowPointer[x] = 1;
      }

      for(s32 x=width-4; x<=(width-3); x++) {
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