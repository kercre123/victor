//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"
#include "embeddedVisionBenchmarks.h"

#include "../../blockImages/fiducial105_6ContrastReduced.h"
#include "../../blockImages/blockImage50.h"

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

#define BIG_BUFFER_SIZE 5000000

#if defined(USING_MOVIDIUS_COMPILER)
__attribute__((section(".ddr_direct.rodata")))
#endif
  char bigBuffer0[BIG_BUFFER_SIZE];

#if defined(USING_MOVIDIUS_COMPILER)
__attribute__((section(".ddr_direct.rodata")))
#endif
  char bigBuffer1[BIG_BUFFER_SIZE];

#if defined(USING_MOVIDIUS_COMPILER)
__attribute__((section(".ddr_direct.rodata")))
#endif
  char bigBuffer2[BIG_BUFFER_SIZE];

static const s32 width = 640;
//static const s32 height = 480;
static const s32 height = 10;
//static const s32 numBytes = MIN(MAX_BYTES, 5000000);

int BenchmarkSimpleDetector_Steps12345_realImage(int numIterations)
{
  const s32 scaleImage_numPyramidLevels = 5;

  const s16 component1d_minComponentWidth = 0;
  const s16 component1d_maxSkipDistance = 0;

  const f32 minSideLength = 0.03f*MAX(fiducial105_6_HEIGHT,fiducial105_6_WIDTH);
  const f32 maxSideLength = 0.97f*MIN(fiducial105_6_HEIGHT,fiducial105_6_WIDTH);

  const s32 component_minimumNumPixels = static_cast<s32>(Round(minSideLength*minSideLength - (0.8f*minSideLength)*(0.8f*minSideLength)));
  const s32 component_maximumNumPixels = static_cast<s32>(Round(maxSideLength*maxSideLength - (0.8f*maxSideLength)*(0.8f*maxSideLength)));
  const s32 component_sparseMultiplyThreshold = 1000 << 5;
  const s32 component_solidMultiplyThreshold = 2 << 5;

  const s32 component_percentHorizontal = 1 << 7; // 0.5, in SQ 23.8
  const s32 component_percentVertical = 1 << 7; // 0.5, in SQ 23.8

  const s32 maxExtractedQuads = 1000;
  const s32 quads_minQuadArea = 100;
  const s32 quads_quadSymmetryThreshold = 384;
  const s32 quads_minDistanceFromImageEdge = 2;

  const f32 decode_minContrastRatio = 1.25;

  const s32 maxMarkers = 100;

  MemoryStack scratch0(&bigBuffer0[0], BIG_BUFFER_SIZE);
  MemoryStack scratch1(&bigBuffer1[0], BIG_BUFFER_SIZE);
  MemoryStack scratch2(&bigBuffer2[0], BIG_BUFFER_SIZE);

  const s32 maxConnectedComponentSegments = u16_MAX;
  ConnectedComponents extractedComponents(maxConnectedComponentSegments, scratch0);

  Array<u8> image(blockImage50_HEIGHT, blockImage50_WIDTH, scratch0);
  image.Set(&blockImage50[0], blockImage50_HEIGHT*blockImage50_WIDTH);

  FixedLengthList<BlockMarker> markers(maxMarkers, scratch0);
  FixedLengthList<Array<f64>> homographies(maxMarkers, scratch0);

  markers.set_size(maxMarkers);
  homographies.set_size(maxMarkers);

  for(s32 i=0; i<maxMarkers; i++) {
    Array<f64> newArray(3, 3, scratch0);
    homographies[i] = newArray;
  } // for(s32 i=0; i<maximumSize; i++)

  f64 totalTime = 0;
  for(s32 i=0; i<numIterations; i++) {
    PUSH_MEMORY_STACK(scratch1);
    PUSH_MEMORY_STACK(scratch2);

    const f64 time0 = GetTime();
    SimpleDetector_Steps12345(
      image,
      markers,
      homographies,
      scaleImage_numPyramidLevels,
      component1d_minComponentWidth, component1d_maxSkipDistance,
      component_minimumNumPixels, component_maximumNumPixels,
      component_sparseMultiplyThreshold, component_solidMultiplyThreshold,
      component_percentHorizontal, component_percentVertical,
      quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge,
      decode_minContrastRatio,
      scratch1,
      scratch2);
    const f64 time1 = GetTime();
    totalTime += time1 - time0;
    image[0][0]++;
  }
  printf("totalTime: %f\n", totalTime);

  //ASSERT_TRUE(markers.get_size() == 1);

  //ASSERT_TRUE(markers[0].blockType == 105);
  //ASSERT_TRUE(markers[0].faceType == 6);
  //ASSERT_TRUE(markers[0].orientation == BlockMarker::ORIENTATION_LEFT);
  //ASSERT_TRUE(markers[0].corners[0] == Point<s16>(21,21));
  //ASSERT_TRUE(markers[0].corners[1] == Point<s16>(21,235));
  //ASSERT_TRUE(markers[0].corners[2] == Point<s16>(235,21));
  //ASSERT_TRUE(markers[0].corners[3] == Point<s16>(235,235));

  return 0;
} // int BenchmarkSimpleDetector_Steps12345_realImage()

int BenchmarkBinomialFilter()
{
  MemoryStack ms(buffer, MAX_BYTES);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms, false);
  Array<u8> imageFiltered(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
  AnkiConditionalErrorAndReturnValue(imageFiltered.IsValid(), -3, "imageFiltered.IsValid()", "");
#endif

  Result result = BinomialFilter(image, imageFiltered, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkDownsampleByFactor()
{
  const s32 downsampleFactor = 2;

  MemoryStack ms(buffer, MAX_BYTES);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms);
  Array<u8> imageDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
  AnkiConditionalErrorAndReturnValue(imageDownsampled.IsValid(), -3, "imageDownsampled.IsValid()", "");
#endif

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Result result = DownsampleByFactor(image, downsampleFactor, imageDownsampled);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkComputeCharacteristicScale()
{
  const s32 numPyramidLevels = 6;

  MemoryStack ms(buffer, MAX_BYTES);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> image(height, width, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(image.IsValid(), -2, "image.IsValid()", "");
#endif

  FixedPointArray<u32> scaleImage(height, width, 16, ms);
#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(scaleImage.IsValid(), -2, "scaleImage.IsValid()", "");
#endif

  Result result = ComputeCharacteristicScaleImage(image, numPyramidLevels, scaleImage, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}

int BenchmarkTraceInteriorBoundary()
{
  MemoryStack ms(buffer, MAX_BYTES);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(ms.IsValid(), -1, "ms.IsValid()", "");
#endif

  Array<u8> binaryImage(height, width, ms);
  const Point<s16> startPoint(3,3);
  const BoundaryDirection initialDirection = BOUNDARY_N;
  FixedLengthList<Point<s16> > boundary(MAX_BOUNDARY_LENGTH, ms);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(binaryImage.IsValid(), -2, "binaryImage.IsValid()", "");
  AnkiConditionalErrorAndReturnValue(boundary.IsValid(), -3, "boundary.IsValid()", "");
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

#if defined(ANKI_DEBUG_ALL) && defined(ANKICORETECH_USE_MATLAB)
  Matlab matlab;

  matlab.PutArray<u8>(binaryImage, "binaryImage");
#endif // #if defined(ANKI_DEBUG_ALL) && defined(ANKICORETECH_USE_MATLAB)

  Result result = TraceInteriorBoundary(binaryImage, startPoint, initialDirection, boundary);

#ifdef CHECK_FOR_ERRORS
  AnkiConditionalErrorAndReturnValue(result == RESULT_OK, -4, "result == RESULT_OK", "");
#endif

  return 0;
}