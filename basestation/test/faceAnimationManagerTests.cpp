#include "gtest/gtest.h"
#include <opencv2/highgui/highgui.hpp>
#include <vector>

#include "anki/common/types.h"
#include "anki/cozmo/basestation/faceAnimationManager.h"

#define SHOW_IMAGES 0

// NOTE: This code for "uncompressing" the RLE format into an image for display was
// copied from sim_hal.cpp
static void DrawFaceRLE(const std::vector<u8>& rleData,
                        cv::Mat_<u8>& outImg)
{
  using namespace Anki;
  using namespace Cozmo;
  
  const s32 NUM_DISPLAY_PIXELS = (FaceAnimationManager::IMAGE_HEIGHT *
                                  FaceAnimationManager::IMAGE_WIDTH);
  
  u32 pixelCounter = 0;
  bool draw = false;
  
  // Clear the display
  outImg.setTo(0);
  
  // Draw pixels
  auto rleIter = rleData.begin();
  while (rleIter != rleData.end() && *rleIter != 0) {
    u8 run = *rleIter;
    ++rleIter;
    
    u32 numPixels = run < 64 ? run * FaceAnimationManager::IMAGE_WIDTH : run - 64;
    
    // Adjust number of pixels this byte if it exceeds screen total
    if (pixelCounter + numPixels > NUM_DISPLAY_PIXELS) {
      numPixels = NUM_DISPLAY_PIXELS - pixelCounter;
    }
    
    // Set pixels
    if (draw) {
      for (u32 i=0; i<numPixels; ++i) {
        u32 y = pixelCounter / FaceAnimationManager::IMAGE_WIDTH;
        u32 x = pixelCounter - (y * FaceAnimationManager::IMAGE_WIDTH);
        ++pixelCounter;
        
        outImg(y,x) = 255;
      }
    } else {
      // Update total pixels drawn thus far
      pixelCounter += numPixels;
    }
    
    // Toggle whether we're drawing 0s or 1s
    if (run >= 64) {
      draw = !draw;
    }
    
  }
} // DrawFaceRLE()

// Helper for testing whether two images are exactly equal, pixelwise
static bool AreImagesEqual(cv::Mat_<u8>& img1, cv::Mat_<u8>& img2)
{
  assert(img1.isContinuous());
  assert(img2.isContinuous());
  bool areEqual = (img1.rows == img2.rows) && (img1.cols == img2.cols);
  
  u8* imgData1 = img1[0];
  u8* imgData2 = img2[0];
  
  s32 i=0;
  while(areEqual && i<img1.rows*img2.cols)
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
  cv::Mat_<u8> img(Anki::Cozmo::FaceAnimationManager::IMAGE_HEIGHT,
                   Anki::Cozmo::FaceAnimationManager::IMAGE_WIDTH);
  
  cv::Mat_<u8> testImg(Anki::Cozmo::FaceAnimationManager::IMAGE_HEIGHT,
                       Anki::Cozmo::FaceAnimationManager::IMAGE_WIDTH);
  
  // Test with empty image:
  img.setTo(0);
  
  std::vector<u8> rleData;
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("EmptyTestFace", img);
  cv::imshow("EmptyTestFace_Compare", testImg);
# endif
  
  ASSERT_TRUE(img.isContinuous());
  ASSERT_TRUE(testImg.isContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
  // Test with filled image:
  img.setTo(255);
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("FullTestFace", img);
  cv::imshow("FullTestFace_Compare", testImg);
# endif
  
  ASSERT_TRUE(img.isContinuous());
  ASSERT_TRUE(testImg.isContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
  // Test with two simple interlaced eyes:
  cv::Mat_<u8> eyeROI;
  img.setTo(0);
  cv::Rect eyeRectL(24,24,16,16);
  cv::Rect eyeRectR(88,24,16,16);
  eyeROI = img(eyeRectL);
  for(s32 i=0; i<eyeROI.rows; i+=2) {
    cv::Mat_<u8> eyeROI_row = eyeROI.row(i);
    eyeROI_row.setTo(255);
  }
  
  eyeROI = img(eyeRectR);
  for(s32 i=0; i<eyeROI.rows; i+=2) {
    cv::Mat_<u8> eyeROI_row = eyeROI.row(i);
    eyeROI_row.setTo(255);
  }
  
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("TwoEyesTestFace", img);
  cv::imshow("TwoEyesTestFace_Compare", testImg);
  cv::waitKey(10);
# endif
  
  ASSERT_TRUE(img.isContinuous());
  ASSERT_TRUE(testImg.isContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));

  // Test with two simple solid eyes touching top of image
  cv::Mat_<u8> topEyeROI;
  img.setTo(0);
  cv::Rect topEyeRectL(24,0,16,16);
  cv::Rect topEyeRectR(88,0,16,16);
  topEyeROI = img(topEyeRectL);
  for(s32 i=0; i<topEyeROI.rows; ++i) {
    cv::Mat_<u8> topEyeROI_row = topEyeROI.row(i);
    topEyeROI_row.setTo(255);
  }
  
  topEyeROI = img(topEyeRectR);
  for(s32 i=0; i<topEyeROI.rows; ++i) {
    cv::Mat_<u8> topEyeROI_row = topEyeROI.row(i);
    topEyeROI_row.setTo(255);
  }
  
  Anki::Cozmo::FaceAnimationManager::CompressRLE(img, rleData);
  
  DrawFaceRLE(rleData, testImg);
  
# if SHOW_IMAGES
  cv::imshow("TwoEyesAtTopTestFace", img);
  cv::imshow("TwoEyesAtTopTestFace_Compare", testImg);
  cv::waitKey(10);
# endif
  
  ASSERT_TRUE(img.isContinuous());
  ASSERT_TRUE(testImg.isContinuous());
  EXPECT_TRUE(AreImagesEqual(img, testImg));
  
} // FaceAnimationManager.TestCompressRLE
