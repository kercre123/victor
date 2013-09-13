//#define USING_MOVIDIUS_COMPILER

#include "anki/embeddedVision.h"

#if defined(ANKICORETECHEMBEDDED_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
Anki::Embedded::Matlab matlab(false);
#endif

#if defined(ANKICORETECHEMBEDDED_USE_GTEST)
#include "gtest/gtest.h"
#endif

//#define BUFFER_IN_DDR_WITH_L2
//#define BUFFER_IN_CMX

#if defined(BUFFER_IN_DDR_WITH_L2) && defined(BUFFER_IN_CMX)
You cannot use both CMX and L2 Cache;
#endif

#define MAX_BYTES 10000

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

#endif // #ifdef USING_MOVIDIUS_COMPILER

//IN_CACHED_DDR GTEST_TEST(CoreTech_Vision, BinomialFilter)
GTEST_TEST(CoreTech_Vision, BinomialFilter)
{
  const s32 width = 10;
  const s32 height = 5;
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  // Allocate memory from the heap, for the memory allocator
  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Anki::Embedded::Array_u8 image(height, width, ms, false);
  image.Set(static_cast<u8>(0));

  Anki::Embedded::Array_u8 imageFiltered(height, width, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageFiltered.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Anki::Embedded::Result result = Anki::Embedded::BinomialFilter(image, imageFiltered, ms);

  //printf("image:\n");
  //image.Print();

  //printf("imageFiltered:\n");
  //imageFiltered.Print();

  ASSERT_TRUE(result == Anki::Embedded::RESULT_OK);

  const u8 correctResults[5][10] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 1, 1, 1, 2, 2, 2, 3}, {0, 0, 0, 0, 1, 1, 1, 1, 1, 2}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

  for(s32 y=0; y<height; y++) {
    for(s32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageFiltered.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageFiltered.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
}

IN_CACHED_DDR GTEST_TEST(CoreTech_Vision, DownsampleByFactor)
{
  const s32 width = 10;
  const s32 height = 4;
  const s32 downsampleFactor = 2;

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 1000);

  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Anki::Embedded::Array_u8 image(height, width, ms);
  Anki::Embedded::Array_u8 imageDownsampled(height/downsampleFactor, width/downsampleFactor, ms);

  ASSERT_TRUE(image.get_rawDataPointer()!= NULL);
  ASSERT_TRUE(imageDownsampled.get_rawDataPointer()!= NULL);

  for(s32 x=0; x<width; x++) {
    *image.Pointer(2,x) = static_cast<u8>(x);
  }

  Anki::Embedded::Result result = Anki::Embedded::DownsampleByFactor(image, downsampleFactor, imageDownsampled);

  ASSERT_TRUE(result == Anki::Embedded::RESULT_OK);

  const u8 correctResults[2][5] = {{0, 0, 0, 0, 0}, {0, 1, 2, 3, 4}};

  for(s32 y=0; y<imageDownsampled.get_size(0); y++) {
    for(s32 x=0; x<imageDownsampled.get_size(1); x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *imageDownsampled.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
}

IN_CACHED_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale)
{
  const s32 width = 16;
  const s32 height = 16;
  const s32 numLevels = 3;
  const char * imageData =
    "0	0	0	107	255	255	255	255	0	0	0	0	0	0	160	89 "
    "0	255	0	251	255	0	0	255	0	255	255	255	255	0	197	38 "
    "0	0	0	77	255	0	0	255	0	255	255	255	255	0	238	149 "
    "18	34	27	179	255	255	255	255	0	255	255	255	255	0	248	67 "
    "226	220	173	170	40	210	108	255	0	255	255	255	255	0	49	11 "
    "25	100	51	137	218	251	24	255	0	0	0	0	0	0	35	193 "
    "255	255	255	255	255	255	255	255	255	255	49	162	65	133	178	62 "
    "255	0	0	0	0	0	0	0	0	255	188	241	156	253	24	113 "
    "255	0	0	0	0	0	0	0	0	255	62	53	148	56	134	175 "
    "255	0	0	0	0	0	0	0	0	255	234	181	138	27	135	92 "
    "255	0	0	0	0	0	0	0	0	255	69	60	222	28	220	188 "
    "255	0	0	0	0	0	0	0	0	255	195	30	68	16	124	101 "
    "255	0	0	0	0	0	0	0	0	255	48	155	81	103	100	174 "
    "255	0	0	0	0	0	0	0	0	255	73	115	30	114	171	180 "
    "255	0	0	0	0	0	0	0	0	255	23	117	240	93	189	113 "
    "255	255	255	255	255	255	255	255	255	255	147	169	165	195	133	5";

  const u32 correctResults[16][16] = {
    {983040, 2097152, 4390912, 8585216, 12124160, 12976128, 12386304, 10158080, 6488064, 4587520, 4849664, 4849664, 4259840, 4784128, 6225920, 6422528},
    {1638400, 4063232, 6029312, 8847360, 9580544, 10285056, 9109504, 9519104, 8896512, 8667136, 10747904, 10747904, 7880704, 7344128, 6959104, 7012352},
    {2293760, 4947968, 6814720, 7451648, 8088576, 8725504, 9028608, 8997888, 8967168, 10043392, 14680064, 14680064, 11599872, 8269824, 7995392, 7471104},
    {4587520, 6164480, 7009280, 7478272, 9654272, 10133504, 8670208, 8709120, 8748032, 10444800, 14680064, 14680064, 9486336, 8364032, 7331840, 6488064},
    {7536640, 7712768, 8425472, 9195520, 10022912, 10346496, 10166272, 9998336, 8528896, 8637440, 10084352, 9650176, 8568832, 6094848, 5373952, 5570560},
    {9699328, 8581120, 8605696, 8863744, 9355264, 9543680, 11403264, 9338880, 8309760, 8192000, 8060928, 7602176, 6356992, 5308416, 5439488, 6160384},
    {11468800, 8769536, 7925760, 9109504, 9764864, 9764864, 9306112, 9109504, 7880704, 8159232, 8222720, 7536640, 7077888, 7393280, 7073792, 7012352},
    {12255232, 8323072, 5242880, 6555648, 6420480, 6285312, 6422528, 6832128, 7241728, 9699328, 9961472, 8458240, 8716288, 8192000, 7426048, 7536640},
    {11796480, 7241728, 1966080, 3457024, 2965504, 2945024, 3395584, 1966080, 6602752, 9371648, 10420224, 8630272, 8097792, 7743488, 7794688, 8257536},
    {11468800, 6520832, 3735552, 2019328, 1372160, 1335296, 1908736, 3289088, 5963776, 9043968, 10223616, 8482816, 7983104, 7208960, 7979008, 8847360},
    {11468800, 6160384, 3244032, 1437696, 741376, 696320, 1302528, 2756608, 5795840, 8912896, 9633792, 8015872, 7565312, 6750208, 7979008, 9109504},
    {11468800, 6205440, 3280896, 1470464, 774144, 724992, 1323008, 2744320, 6098944, 8585216, 8978432, 7766016, 5963776, 6291456, 8036352, 9109504},
    {11468800, 6656000, 3846144, 2117632, 1470464, 1421312, 1970176, 3252224, 6402048, 8126464, 8454144, 6815744, 5963776, 6553600, 8151040, 9502720},
    {11796480, 5898240, 1966080, 3846144, 3280896, 3231744, 3698688, 1966080, 6365184, 7270400, 7620608, 7274496, 7974912, 7974912, 8847360, 9437184},
    {13107200, 9793536, 5898240, 6656000, 6205440, 6156288, 6508544, 7217152, 8282112, 8904704, 9084928, 9056256, 8818688, 9306112, 8781824, 7667712},
    {15073280, 13107200, 11796480, 11468800, 11468800, 11468800, 11468800, 11796480, 12517376, 12255232, 10813440, 10158080, 10551296, 10158080, 7929856, 5046272}};

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 5000);

  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Anki::Embedded::Array_u8 image(height, width, ms);
  ASSERT_TRUE(image.IsValid());
  ASSERT_TRUE(image.Set(imageData) == width*height);

  Anki::Embedded::Array_u32 scaleImage(height, width, ms);
  ASSERT_TRUE(scaleImage.IsValid());

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numLevels, scaleImage, ms) == Anki::Embedded::RESULT_OK);

  // TODO: manually compute results, and check
  //scaleImage.Print();

  for(s32 y=0; y<height; y++) {
    for(s32 x=0; x<width; x++) {
      //printf("(%d,%d) expected:%d actual:%d\n", y, x, correctResults[y][x], *(imageDownsampled.Pointer(y,x)));
      ASSERT_TRUE(correctResults[y][x] == *scaleImage.Pointer(y,x));
    }
  }

  GTEST_RETURN_HERE;
}

#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
IN_CACHED_DDR GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale2)
{
  const s32 width = 640;
  const s32 height = 480;
  const s32 numLevels = 6;

  /*const s32 width = 320;
  const s32 height = 240;
  const s32 numLevels = 5;*/

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = 10000000;
  void *buffer = calloc(numBytes, 1);
  //ASSERT_TRUE(buffer != NULL);
  Anki::Embedded::MemoryStack ms(buffer, numBytes);

  ASSERT_TRUE(ms.IsValid());

  Anki::Embedded::Matlab matlab(false);

  matlab.EvalStringEcho("image = rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png'));");
  //matlab.EvalStringEcho("image = imresize(rgb2gray(imread('Z:/Documents/testSequence/blockWorldTest_front00000.png')), [240,320]);");
  Anki::Embedded::Array_u8 image = matlab.GetArray_u8("image");
  ASSERT_TRUE(image.get_rawDataPointer() != NULL);

  Anki::Embedded::Array_u32 scaleImage(height, width, ms);
  ASSERT_TRUE(scaleImage.get_rawDataPointer() != NULL);

  ASSERT_TRUE(ComputeCharacteristicScaleImage(image, numLevels, scaleImage, ms) == Anki::Embedded::RESULT_OK);

  matlab.PutArray_u32(scaleImage, "scaleImage6_c");

  free(buffer); buffer = NULL;

  GTEST_RETURN_HERE;
}
#endif // #if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

IN_CACHED_DDR GTEST_TEST(CoreTech_Vision, TraceBoundary)
{
  const s32 width = 16;
  const s32 height = 16;
  const char * imageData =
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 0 0 0 0 0 0 0 0 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 0 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 0 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 0 0 0 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 1 1 1 1 0 1 1 1 1 1 "
    " 1 1 0 1 1 1 1 1 1 1 0 1 1 1 1 1 "
    " 1 1 1 0 0 0 0 0 0 0 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 "
    " 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 ";

  const s32 numPoints = 9;
  const Anki::Embedded::Point_s16 groundTruth[9] = {Anki::Embedded::Point_s16(8,6), Anki::Embedded::Point_s16(8,5), Anki::Embedded::Point_s16(7,4), Anki::Embedded::Point_s16(7,3), Anki::Embedded::Point_s16(8,3), Anki::Embedded::Point_s16(9,3), Anki::Embedded::Point_s16(9,4), Anki::Embedded::Point_s16(9,5), Anki::Embedded::Point_s16(9,6)};

  // Allocate memory from the heap, for the memory allocator
  const s32 numBytes = MIN(MAX_BYTES, 10000);

  Anki::Embedded::MemoryStack ms(&buffer[0], numBytes);

  ASSERT_TRUE(ms.IsValid());

  Anki::Embedded::Array_u8 binaryImage(height, width, ms);
  const Anki::Embedded::Point_s16 startPoint(8,6);
  const Anki::Embedded::BoundaryDirection initialDirection = Anki::Embedded::BOUNDARY_N;
  Anki::Embedded::FixedLengthList_Point_s16 boundary(Anki::Embedded::MAX_BOUNDARY_LENGTH, ms);

  ASSERT_TRUE(binaryImage.IsValid());
  ASSERT_TRUE(boundary.IsValid());

  binaryImage.Set(imageData);

  ASSERT_TRUE(Anki::Embedded::TraceBoundary(binaryImage, startPoint, initialDirection, boundary) == Anki::Embedded::RESULT_OK);

  ASSERT_TRUE(boundary.get_size() == numPoints);
  for(s32 iPoint=0; iPoint<numPoints; iPoint++) {
    //printf("%d) (%d,%d) and (%d,%d)\n", iPoint, boundary.Pointer(iPoint)->x, boundary.Pointer(iPoint)->y, groundTruth[iPoint].x, groundTruth[iPoint].y);
    ASSERT_TRUE(*boundary.Pointer(iPoint) == groundTruth[iPoint]);
  }

  GTEST_RETURN_HERE;
}

#if !defined(ANKICORETECHEMBEDDED_USE_GTEST)
void RUN_ALL_TESTS()
{
  s32 numPassedTests = 0;
  s32 numFailedTests = 0;

  CALL_GTEST_TEST(CoreTech_Vision, BinomialFilter);
  CALL_GTEST_TEST(CoreTech_Vision, DownsampleByFactor);
  CALL_GTEST_TEST(CoreTech_Vision, ComputeCharacteristicScale);
  CALL_GTEST_TEST(CoreTech_Vision, TraceBoundary);

  printf("\n========================================================================\nUNIT TEST RESULTS:\nNumber Passed:%d\nNumber Failed:%d\n========================================================================\n", numPassedTests, numFailedTests);
} // void RUN_ALL_TESTS()
#endif // #if !defined(ANKICORETECHEMBEDDED_USE_GTEST)