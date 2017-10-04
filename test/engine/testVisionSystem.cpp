/**
 * File: testVisionSystem.cpp
 *
 * Author:  Andrew Stein
 * Created: 08/08/2016
 *
 * Description: Unit/regression tests for Cozmo's VisionSystem
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=VisionSystem*
 **/


#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/types.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/visionSystem.h"
#include "engine/vision/laserPointDetector.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/vision/basestation/imageCache.h"
#include "anki/vision/MarkerCodeDefinitions.h"

#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"
#include "json/json.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;


TEST(VisionSystem, MarkerDetectionTests)
{
# define DISABLED        0
# define ENABLED         1
# define ENABLE_AND_SAVE 2

# define DEBUG_DISPLAY DISABLED

  using namespace Anki;

  // Construct a vision system
  Cozmo::VisionSystem visionSystem(cozmoContext);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(RESULT_OK, result);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  auto calib = std::make_shared<Vision::CameraCalibration>(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);

  // Turn on _only_ marker detection
  result = visionSystem.SetNextMode(Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(RESULT_OK, result);

  result = visionSystem.SetNextMode(Cozmo::VisionMode::DetectingMarkers, true);
  ASSERT_EQ(RESULT_OK, result);

  // Make sure we run on every frame
  result = visionSystem.PushNextModeSchedule(Cozmo::AllVisionModesSchedule({{Cozmo::VisionMode::DetectingMarkers, Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(RESULT_OK, result);

  // Grab all the test images from "resources/test/lowLightMarkerDetectionTests"
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/markerDetectionTests");

  struct TestDefinition
  {
    std::string subDir;
    f32         expectedFailureRate;
    std::function<bool(size_t numMarkers)> didSucceedFcn;
  };

  const std::vector<TestDefinition> testDefinitions = {

    TestDefinition{
      .subDir = "BacklitStack",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers >= 2;
      }
    },

    TestDefinition{
      .subDir = "LowLight",
      .expectedFailureRate = .02f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers > 0;
      }
    },

    TestDefinition{
      .subDir = "NoMarkers",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers == 0;
      }
    },

  };


  Vision::ImageCache imageCache;

  for(auto & testDefinition : testDefinitions)
  {
    const std::string& subDir = testDefinition.subDir;

    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");

    u32 numFailures = 0;
    for(auto & filename : testFiles)
    {
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);

      imageCache.Reset(img);

      Cozmo::VisionPoseData robotState; // not needed just to detect markers
      robotState.cameraPose.SetParent(robotState.histState.GetPose()); // just so we don't trigger an assert
      result = visionSystem.Update(robotState, imageCache);
      ASSERT_EQ(RESULT_OK, result);

      Cozmo::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);

      // For now, the measure of "success" for an image is if we detected at least
      // one marker in it. We are not checking whether the marker's type or position
      // is correct.
      if(!testDefinition.didSucceedFcn(processingResult.observedMarkers.size()))
      {
        ++numFailures;
      }

      if(DEBUG_DISPLAY != DISABLED)
      {
        for(auto const& debugImg : processingResult.debugImageRGBs)
        {
          debugImg.second.Display(debugImg.first.c_str());
        }

        Vision::ImageRGB dispImg(img);
        for(auto const& marker : processingResult.observedMarkers)
        {
          dispImg.DrawQuad(marker.GetImageCorners(), NamedColors::RED, 2);
          dispImg.DrawLine(marker.GetImageCorners().GetTopLeft(),
                           marker.GetImageCorners().GetTopRight(),
                           NamedColors::GREEN, 3);
          std::string markerName(Vision::MarkerTypeStrings[marker.GetCode()]);
          markerName = markerName.substr(strlen("MARKER_"), std::string::npos);

          // Use leftmost corner to anchor text
          Point2f textPoint(std::numeric_limits<f32>::max(), 0.f);
          for(auto const& corner : marker.GetImageCorners())
          {
            if(corner.x() < textPoint.x())
            {
              textPoint = corner;
            }
          }
          dispImg.DrawText(textPoint, markerName, NamedColors::YELLOW, 0.5f, true);
        }
        auto meanVal = cv::mean(img.get_CvMat_());
        dispImg.DrawText(Point2f(1,9), "mean: " +  std::to_string((u8)meanVal[0]), NamedColors::RED, 0.4f, true);
        dispImg.Display(filename.c_str(), 0);

        if(DEBUG_DISPLAY == ENABLE_AND_SAVE)
        {
          dispImg.Save("temp/markerDetectionTests/" + filename + ".png");
        }
      }

    }

    const f32 failureRate = (f32) numFailures / (f32) testFiles.size();

    PRINT_NAMED_INFO("VisionSystem.MarkerDetectionTests",
                     "%s: %.1f%% failures (%u of %zu)",
                     subDir.c_str(), 100.f*failureRate, numFailures, testFiles.size());

    // Note that we're not expecting perfection here
    EXPECT_LE(failureRate, testDefinition.expectedFailureRate);
  }

# undef DEBUG_DISPLAY
# undef ENABLED
# undef DISABLED
# undef ENABLE_AND_SAVE

} // MarkerDetectionTests


// Makes sure image quality matches the subdirectory name for all images in
// test/imageQualityTests
TEST(VisionSystem, ImageQuality)
{
  using namespace Anki;

  // Construct a vision system
  Cozmo::VisionSystem visionSystem(cozmoContext);

  // Make sure we are running every frame so we don't skip test images!
  Json::Value config = cozmoContext->GetDataLoader()->GetRobotVisionConfig();

  Result result = visionSystem.Init(config);
  ASSERT_EQ(RESULT_OK, result);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  auto calib = std::make_shared<Vision::CameraCalibration>(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);

  // Turn on _only_ image quality check
  result = visionSystem.SetNextMode(Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(RESULT_OK, result);

  result = visionSystem.SetNextMode(Cozmo::VisionMode::CheckingQuality, true);
  ASSERT_EQ(RESULT_OK, result);

  result = visionSystem.PushNextModeSchedule(Cozmo::AllVisionModesSchedule({{Cozmo::VisionMode::CheckingQuality, Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(RESULT_OK, result);

  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/imageQualityTests");

  const std::vector<std::string> testSubDirs = {
    "Good", "TooBright", "TooDark"
  };

  Vision::ImageCache imageCache;

  // Fake the exposure parameters so that we are always against the extremes in order
  // to trigger TooDark and TooBright
  const Cozmo::VisionSystem::GammaCurve gammaCurve{};
  result = visionSystem.SetCameraExposureParams(1, 1, 1, 2.f, 2.f, 2.f, gammaCurve);
  ASSERT_EQ(RESULT_OK, result);

  for(auto & subDir : testSubDirs)
  {
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");

    for(auto & filename : testFiles)
    {
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);

      imageCache.Reset(img);

      Cozmo::VisionPoseData robotState; // not needed for image quality check
      robotState.cameraPose.SetParent(robotState.histState.GetPose()); // just so we don't trigger an assert
      result = visionSystem.Update(robotState, imageCache);
      ASSERT_EQ(RESULT_OK, result);

      Cozmo::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);

      // Make sure the detected quality is as expected for this subdir
      EXPECT_EQ(subDir, EnumToString(processingResult.imageQuality));

      PRINT_NAMED_INFO("VisionSystem.ImageQuality", "%s = %s",
                       filename.c_str(), EnumToString(processingResult.imageQuality));

#     define DISPLAY_IMAGES 0
      if(DISPLAY_IMAGES)
      {
        for(auto const& debugImg : processingResult.debugImageRGBs)
        {
          debugImg.second.Display(debugImg.first.c_str());
        }
        img.Display("TestImage", 0);
      }
    }
  }

} // ImageQuality

GTEST_TEST(LaserPointDetector, LaserDetect)
{

  // TODO Make this test more comprehensive, see below:
  // This unit test does not test the "real" Detect function, only the one that doesn't use the robot pose.
  // This function is only used by this test. The important change here is to get the robot's head angle and
  // the corresponding homography and test the full Detect metod.

#define DEBUG_LASER_DISPLAY false

  using namespace Anki;

  auto doTest = [](const std::string& imageName, int expectedPoints)
  {

    PRINT_NAMED_INFO("LaserPointDetector.LaserDetect", "Testing image %s", imageName.c_str());

    Vision::Image testImg;
    Vision::ImageCache imageCache;
    Result result = testImg.Load(imageName);
    ASSERT_EQ(RESULT_OK, result);
    imageCache.Reset(testImg);

    // Create LaserPointDetector and test on image
    Cozmo::DebugImageList<Anki::Vision::ImageRGB> debugImageList;
    std::list<Cozmo::ExternalInterface::RobotObservedLaserPoint> points;

    Cozmo::LaserPointDetector detector(nullptr);
    result = detector.Detect(imageCache, false, points, debugImageList);
    ASSERT_EQ(RESULT_OK, result);
    EXPECT_EQ(expectedPoints, points.size()) << "Image: "<<imageName;

    if (DEBUG_LASER_DISPLAY) {
      for (auto image: debugImageList) {
        image.second.Display(imageName.c_str(), 0);
      }
    }

  };

  // True positives
  {
    // TODO Check if the found point is the correct one, not just one
    const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                     "test/LaserPointDetectionTests/true_positives");
    const std::vector<std::string> &files = Util::FileUtils::FilesInDirectory(testImageDir, true, ".jpg", false);
    for (const auto& filename: files) {
      doTest(filename, 1);
    }
  }

  // True negatives
  {
    const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                     "test/LaserPointDetectionTests/true_negatives");
    const std::vector<std::string> &files = Util::FileUtils::FilesInDirectory(testImageDir, true, ".jpg", false);
    for (const auto& filename: files) {
      doTest(filename, 0);
    }
  }

} // LaserPointDetector


