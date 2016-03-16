#include "gtest/gtest.h"
#include <opencv2/highgui.hpp>
#include <vector>

#include "anki/common/types.h"
#include "anki/common/basestation/math/rect_impl.h"
#include "anki/common/basestation/array2d_impl.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/vision/basestation/image.h"

#define SHOW_IMAGES 0

// Helper for testing whether two images are exactly equal, pixelwise
static bool AreImagesEqual(Anki::Vision::Image& img1, Anki::Vision::Image& img2)
{
  assert(img1.IsContinuous());
  assert(img2.IsContinuous());
  bool areEqual = (img1.GetNumRows() == img2.GetNumRows()) && (img1.GetNumCols() == img2.GetNumCols());
  
  u8* imgData1 = img1.GetDataPointer();
  u8* imgData2 = img2.GetDataPointer();
  
  s32 i=0;
  while(areEqual && i<img1.GetNumElements())
  {
    areEqual = imgData1[i] == imgData2[i];
    ++i;
  }
  
  return areEqual;
} // AreImagesEqual()

// Test to see if images pushed through the RLE compressor come back as expected
// when uncompressed using the helper above
TEST(FaceAnimationManager, TestCompressRLE)
{
  Anki::Vision::Image img(Anki::Cozmo::FaceAnimationManager::IMAGE_HEIGHT,
                          Anki::Cozmo::FaceAnimationManager::IMAGE_WIDTH);
  
  Anki::Vision::Image testImg(Anki::Cozmo::FaceAnimationManager::IMAGE_HEIGHT,
                              Anki::Cozmo::FaceAnimationManager::IMAGE_WIDTH);
  
  // Test with empty image:
  img.FillWith(0);
  
  std::vector<u8> rleData;
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  Anki::Cozmo::FaceAnimationManager::DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("EmptyTestFace", img);
  cv::imshow("EmptyTestFace_Compare", testImg);
# endif
  
  ASSERT_TRUE(img.IsContinuous());
  ASSERT_TRUE(testImg.IsContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
  // Test with filled image:
  img.FillWith(255);
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  Anki::Cozmo::FaceAnimationManager::DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("FullTestFace", img);
  cv::imshow("FullTestFace_Compare", testImg);
# endif
  
  ASSERT_TRUE(img.IsContinuous());
  ASSERT_TRUE(testImg.IsContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
  // Test with two simple interlaced eyes:
  Anki::Vision::Image eyeROI;
  img.FillWith(0);
  Anki::Rectangle<s32> eyeRectL(24,24,16,16);
  Anki::Rectangle<s32> eyeRectR(88,24,16,16);
  eyeROI = img.GetROI(eyeRectL);
  for(s32 i=0; i<eyeROI.GetNumRows(); i+=2) {
    u8* eyeROI_row = eyeROI.GetRow(i);
    memset(eyeROI_row, 255, eyeROI.GetNumCols());
  }
  
  eyeROI = img.GetROI(eyeRectR);
  for(s32 i=0; i<eyeROI.GetNumRows(); i+=2) {
    u8* eyeROI_row = eyeROI.GetRow(i);
    memset(eyeROI_row, 255, eyeROI.GetNumCols());
  }
  
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  Anki::Cozmo::FaceAnimationManager::DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("TwoEyesTestFace", img);
  cv::imshow("TwoEyesTestFace_Compare", testImg);
  cv::waitKey(10);
# endif
  
  ASSERT_TRUE(img.IsContinuous());
  ASSERT_TRUE(testImg.IsContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));

  // Test with two simple solid eyes touching top of image
  Anki::Vision::Image topEyeROI;
  img.FillWith(0);
  Anki::Rectangle<s32> topEyeRectL(24,0,16,16);
  Anki::Rectangle<s32> topEyeRectR(88,0,16,16);
  topEyeROI = img.GetROI(topEyeRectL);
  for(s32 i=0; i<topEyeROI.GetNumRows(); ++i) {
    u8* topEyeROI_row = topEyeROI.GetRow(i);
    memset(topEyeROI_row, 255, eyeROI.GetNumCols());
  }
  
  topEyeROI = img.GetROI(topEyeRectR);
  for(s32 i=0; i<topEyeROI.GetNumRows(); ++i) {
    u8* topEyeROI_row = topEyeROI.GetRow(i);
    memset(topEyeROI_row, 255, eyeROI.GetNumCols());
  }
  
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  Anki::Cozmo::FaceAnimationManager::DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("TwoEyesAtTopTestFace", img);
  cv::imshow("TwoEyesAtTopTestFace_Compare", testImg);
  cv::waitKey(10);
# endif
  
  ASSERT_TRUE(img.IsContinuous());
  ASSERT_TRUE(testImg.IsContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
} // FaceAnimationManager.TestCompressRLE
