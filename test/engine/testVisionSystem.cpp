
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


#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/rotation.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/visionSystem.h"
#include "engine/vision/laserPointDetector.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/neuralNetRunner.h"
#include "coretech/vision/shared/MarkerCodeDefinitions.h"

#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"
#include "json/json.h"

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui.hpp"

#include "coretech/common/engine/colorRGBA.h"


extern Anki::Vector::CozmoContext* cozmoContext;

TEST(VisionSystem, DISABLED_CameraCalibrationTarget_InvertedBox)
{
  NativeAnkiUtilConsoleSetValueWithString("CalibTargetType",
                                          std::to_string(Anki::Vector::CameraCalibrator::INVERTED_BOX).c_str());
  
  Anki::Vector::VisionSystem* visionSystem = new Anki::Vector::VisionSystem(cozmoContext);
  Anki::Result result = visionSystem->Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  std::shared_ptr<Anki::Vision::CameraCalibration> calib(new Anki::Vision::CameraCalibration(360,640,
                                                                                             290,290,
                                                                                             320,180,
                                                                                             0.f));
  result = visionSystem->UpdateCameraCalibration(calib);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
    
  Anki::Vision::ImageCache imageCache;
  
  Anki::Vision::ImageRGB img;
  
  std::string testImgPath = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                            "test/markerDetectionTests/CalibrationTarget/inverted_box.jpg");
  
  result = img.Load(testImgPath);
  
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  imageCache.Reset(img);

  Anki::Vector::VisionSystemInput input;
  input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::DetectingMarkers, true);
  input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::ComputingCalibration, true);
  input.imageBuffer = imageCache.GetBuffer();
  
  result = visionSystem->Update(input);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  Anki::Vector::VisionProcessingResult processingResult;
  bool resultAvailable = visionSystem->CheckMailbox(processingResult);
  EXPECT_TRUE(resultAvailable);
  
  // 1 is the default value of focal length
  ASSERT_EQ(processingResult.cameraCalibration.size(), 1);
  ASSERT_EQ(processingResult.cameraCalibration.front().GetFocalLength_x() != 1.f, true);
  
  const std::vector<f32> distortionCoeffs = {{-0.06456808353003426,
                                              -0.2702951616534774,
                                               0.001409446798192025,
                                               0.001778340478064528,
                                               0.1779613197923669,
                                               0, 0, 0}};
  
  const Anki::Vision::CameraCalibration expectedCalibration(360,640,
                                                            372.328857,
                                                            368.344482,
                                                            306.370270,
                                                            185.576843,
                                                            0,
                                                            distortionCoeffs);
  
  const auto& computedCalibration = processingResult.cameraCalibration.front();
  
  ASSERT_NEAR(computedCalibration.GetCenter_x(), expectedCalibration.GetCenter_x(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetCenter_y(), expectedCalibration.GetCenter_y(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetFocalLength_x(), expectedCalibration.GetFocalLength_x(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetFocalLength_y(), expectedCalibration.GetFocalLength_y(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetNcols(), expectedCalibration.GetNcols(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetNrows(), expectedCalibration.GetNrows(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetSkew(), expectedCalibration.GetSkew(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetDistortionCoeffs().size(), expectedCalibration.GetDistortionCoeffs().size(), 0);
  
  for(int i = 0; i < expectedCalibration.GetDistortionCoeffs().size(); ++i)
  {
    ASSERT_NEAR(computedCalibration.GetDistortionCoeffs()[i], expectedCalibration.GetDistortionCoeffs()[i],
                Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  }
  
  Anki::Util::SafeDelete(visionSystem);
  visionSystem = nullptr;
}

TEST(VisionSystem, DISABLED_CameraCalibrationTarget_Qbert)
{
  NativeAnkiUtilConsoleSetValueWithString("CalibTargetType",
                                          std::to_string(Anki::Vector::CameraCalibrator::QBERT).c_str());

  Anki::Vector::VisionSystem* visionSystem = new Anki::Vector::VisionSystem(cozmoContext);
  Anki::Result result = visionSystem->Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  std::shared_ptr<Anki::Vision::CameraCalibration> calib(new Anki::Vision::CameraCalibration(360,640,
                                                                                             290,290,
                                                                                             320,180,
                                                                                             0.f));
  result = visionSystem->UpdateCameraCalibration(calib);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
    
  Anki::Vision::ImageCache imageCache;
  
  Anki::Vision::ImageRGB img;
  
  std::string testImgPath = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                            "test/markerDetectionTests/CalibrationTarget/qbert.png");
  result = img.Load(testImgPath);
  
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  imageCache.Reset(img);
  
  Anki::Vector::VisionSystemInput input;
  input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::DetectingMarkers, true);
  input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::ComputingCalibration, true);
  input.imageBuffer = imageCache.GetBuffer();
  
  result = visionSystem->Update(input);
    ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  Anki::Vector::VisionProcessingResult processingResult;
  bool resultAvailable = visionSystem->CheckMailbox(processingResult);
  EXPECT_TRUE(resultAvailable);
  
  // 1 is the default value of focal length
  ASSERT_EQ(processingResult.cameraCalibration.size(), 1);
  ASSERT_EQ(processingResult.cameraCalibration.front().GetFocalLength_x() != 1.f, true);

  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
                                              -0.2198782133395603,
                                              0.001435740245449692,
                                              0.001523365725052927,
                                              0.1341471670512819,
                                              0, 0, 0}};
  
  const Anki::Vision::CameraCalibration expectedCalibration(360,640,
                                                            362.8773099149878,
                                                            366.7347434532929,
                                                            302.2888225643724,
                                                            200.012543449327,
                                                            0,
                                                            distortionCoeffs);
  
  const auto& computedCalibration = processingResult.cameraCalibration.front();
  
  ASSERT_NEAR(computedCalibration.GetCenter_x(), expectedCalibration.GetCenter_x(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetCenter_y(), expectedCalibration.GetCenter_y(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetFocalLength_x(), expectedCalibration.GetFocalLength_x(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetFocalLength_y(), expectedCalibration.GetFocalLength_y(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetNcols(), expectedCalibration.GetNcols(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetNrows(), expectedCalibration.GetNrows(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetSkew(), expectedCalibration.GetSkew(),
              Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  
  ASSERT_NEAR(computedCalibration.GetDistortionCoeffs().size(), expectedCalibration.GetDistortionCoeffs().size(), 0);
  
  for(int i = 0; i < expectedCalibration.GetDistortionCoeffs().size(); ++i)
  {
    ASSERT_NEAR(computedCalibration.GetDistortionCoeffs()[i], expectedCalibration.GetDistortionCoeffs()[i],
                Anki::Util::FLOATING_POINT_COMPARISON_TOLERANCE_FLT);
  }
  
  Anki::Util::SafeDelete(visionSystem);
  visionSystem = nullptr;
}

TEST(VisionSystem, MarkerDetectionTests)
{
# define DISABLED        0
# define ENABLED         1
# define ENABLE_AND_SAVE 2
# define ENABLE_ON_FAIL  3

# define DEBUG_DISPLAY DISABLED

  using namespace Anki;

  // Construct a vision system
  // NOTE: We don't just use a MarkerDetector here because the VisionSystem also does CLAHE preprocessing which
  //       is part of this test (e.g. for low light performance)
  Vector::VisionSystem visionSystem(cozmoContext);
  Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(RESULT_OK, result);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  auto calib = std::make_shared<Vision::CameraCalibration>(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);

  // Grab all the test images from "resources/test/lowLightMarkerDetectionTests"
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/markerDetectionTests");

  struct TestDefinition
  {
    std::string subDir;
    f32         expectedFailureRate;
    std::function<bool(const std::list<Vision::ObservedMarker>&, const std::string& filename)> didSucceedFcn;
  };

  // Helper to check success by verifying exactly one marker was detected and its name matches the filename
  // (ignoring trailing number)
  auto matchFilenameFcn = [](const std::list<Vision::ObservedMarker>& markers, const std::string& filename) -> bool
  {
    if(markers.size() != 1)
    {
      return false;
    }
    
    const char* codeName = markers.front().GetCodeName();
    return (strncmp(codeName, filename.c_str(), strlen(codeName)) == 0);
  };
  
  std::vector<TestDefinition> testDefinitions = {
    TestDefinition{
      .subDir = "NoMarkers",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = [](const std::list<Vision::ObservedMarker>& markers, const std::string&) -> bool
      {
        return markers.empty();
      }
    },

    TestDefinition{
      .subDir = "LightOnDark_Circle",
      .expectedFailureRate = 0.08f, // Should be 0 VIC-1165
      .didSucceedFcn = matchFilenameFcn,
    },

    TestDefinition{
      .subDir = "LightOnDark_Square",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = matchFilenameFcn,
    },

    TestDefinition{
      .subDir = "LightOnDark_Charger",
      .expectedFailureRate = 0.05f, // Should be 0 VIC-1165
      .didSucceedFcn = [](const std::list<Vision::ObservedMarker>& markers, const std::string& filename) -> bool
      {
        return (markers.size() == 1) && (strncmp("MARKER_CHARGER_HOME", markers.front().GetCodeName(), 19) == 0);
      }
    },

  };
    
  const std::vector<std::string> markerDirs = {
    "MARKER_CHARGER_HOME", "MARKER_LIGHTCUBE_CIRCLE_BACK", "MARKER_LIGHTCUBE_CIRCLE_BOTTOM",
    "MARKER_LIGHTCUBE_CIRCLE_FRONT", "MARKER_LIGHTCUBE_CIRCLE_LEFT",  "MARKER_LIGHTCUBE_CIRCLE_RIGHT",
    "MARKER_LIGHTCUBE_CIRCLE_TOP"
  };
  
  for(auto const& markerDir : markerDirs)
  {
    testDefinitions.push_back(TestDefinition{
      .subDir = markerDir,
      .expectedFailureRate = 0.1f, // Can we get this to 0? (VIC-1165)
      .didSucceedFcn = [&markerDir](const std::list<Vision::ObservedMarker>& markers, const std::string& filename) -> bool
      {
        return (markers.size() == 1) && (markerDir.compare(0, markerDir.length(), markers.front().GetCodeName()) == 0);
      }
    });
  }
    
  Vision::ImageCache imageCache;

  for(auto & testDefinition : testDefinitions)
  {
    const std::string& subDir = testDefinition.subDir;

    std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");
    const std::vector<std::string> testFiles_png = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".png");
    std::copy(testFiles_png.begin(), testFiles_png.end(), std::back_inserter(testFiles));

    EXPECT_FALSE(testFiles.empty());
    
    u32 numFailures = 0;
    for(auto & filename : testFiles)
    {
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);

      imageCache.Reset(img);

      Anki::Vector::VisionSystemInput input;
      input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::DetectingMarkers, true);
      input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::FullFrameMarkerDetection, true);
      input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::MarkerDetectionWhileRotatingFast, true);
      input.imageBuffer = imageCache.GetBuffer();
  
      Vector::VisionPoseData robotState; // not needed just to detect markers
      robotState.cameraPose.SetParent(robotState.histState.GetPose()); // just so we don't trigger an assert
      input.poseData = robotState;
      
      result = visionSystem.Update(input);
      ASSERT_EQ(RESULT_OK, result);

      Vector::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);

      // For now, the measure of "success" for an image is if we detected at least
      // one marker in it. We are not checking whether the marker's type or position
      // is correct.
      const bool success = testDefinition.didSucceedFcn(processingResult.observedMarkers, filename);
      if(!success)
      {
        ++numFailures;
      }

      if(DEBUG_DISPLAY != DISABLED && ((DEBUG_DISPLAY != ENABLE_ON_FAIL) || !success))
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
  Vector::VisionSystem visionSystem(cozmoContext);

  // Make sure we are running every frame so we don't skip test images!
  Json::Value config = cozmoContext->GetDataLoader()->GetRobotVisionConfig();

  Result result = visionSystem.Init(config);
  ASSERT_EQ(RESULT_OK, result);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  auto calib = std::make_shared<Vision::CameraCalibration>(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);

  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/imageQualityTests");

  const std::vector<std::string> testSubDirs = {
    "Good", "TooBright", "TooDark"
  };
  
  struct Test {
    std::string subDir;
    s32 exposureTime_ms;
    f32 exposureGain;
  };
  
  const std::vector<Test> tests = {
    {
      .subDir = "Good",
      .exposureTime_ms = 31,
      .exposureGain = 2.f
    },
    {
      .subDir = "TooBright",
      .exposureTime_ms = visionSystem.GetMinCameraExposureTime_ms(),
      .exposureGain = visionSystem.GetMinCameraGain(),
    },
    {
      .subDir = "TooDark",
      .exposureTime_ms = visionSystem.GetMaxCameraExposureTime_ms(),
      .exposureGain = visionSystem.GetMaxCameraGain()
    },
  };

  Vision::ImageCache imageCache;

  for(auto & test : tests)
  {
    const std::string fullFileName = Util::FileUtils::FullFilePath({testImageDir, test.subDir});
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(fullFileName, false, ".jpg");

    for(auto & filename : testFiles)
    {
      // Fake the exposure parameters so that we are always against the extremes in order
      // to trigger TooDark and TooBright
      const Vector::VisionSystem::GammaCurve gammaCurve{};
      result = visionSystem.SetCameraExposureParams(test.exposureTime_ms, test.exposureGain, gammaCurve);
      ASSERT_EQ(RESULT_OK, result);
      
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, test.subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);

      imageCache.Reset(img);

      Anki::Vector::VisionSystemInput input;
      input.modesToProcess.SetBitFlag(Anki::Vector::VisionMode::AutoExposure, true);
      input.imageBuffer = imageCache.GetBuffer();
      
      Vector::VisionPoseData robotState; // not needed for image quality check
      robotState.cameraPose.SetParent(robotState.histState.GetPose()); // just so we don't trigger an assert
      input.poseData = robotState;

      result = visionSystem.Update(input);
      ASSERT_EQ(RESULT_OK, result);

      Vector::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);

      // Make sure the detected quality is as expected for this subdir
      EXPECT_EQ(test.subDir, EnumToString(processingResult.imageQuality));

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
    Vector::DebugImageList<Anki::Vision::ImageRGB> debugImageList;
    std::list<Vector::ExternalInterface::RobotObservedLaserPoint> points;

    Vector::LaserPointDetector detector(nullptr);
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

// This test is meant to avoid checking in a vision_config.json or code changes that break the ability to load
// the neural net model(s) successfully.
GTEST_TEST(NeuralNets, InitFromConfig)
{
  using namespace Anki;
  
  // Load vision_config.json file and get NeuralNets section
  const Json::Value& config = cozmoContext->GetDataLoader()->GetRobotVisionConfig();
  ASSERT_TRUE(config.isMember("NeuralNets"));
  const Json::Value& neuralNetConfig = config["NeuralNets"];
  
  const std::string visionConfigPath = Util::FileUtils::FullFilePath({"config", "engine", "vision"});
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                               visionConfigPath);
  const std::string cachePath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Cache, "vision");
  const std::string modelPath = Util::FileUtils::FullFilePath({dataPath, "dnn_models"});
  const std::string dnnCachePath = Util::FileUtils::FullFilePath({cachePath, "neural_nets"});
  
  // Make sure "NeuralNetRunner" load succeeds, given all the current params in vision_config.json
  Vision::NeuralNetRunner neuralNetRunner;
  const Result loadRunnerResult = neuralNetRunner.Init(modelPath, dnnCachePath, neuralNetConfig);
  ASSERT_EQ(RESULT_OK, loadRunnerResult);
  
  ASSERT_TRUE(neuralNetConfig.isMember("graphFile"));
  const std::string modelFileName = neuralNetConfig["graphFile"].asString();
  
  // Make sure we have correct extension
  const size_t extIndex = modelFileName.find_last_of(".");
  ASSERT_NE(std::string::npos, extIndex); // must have extension
  const std::string extension = modelFileName.substr(extIndex, std::string::npos);
  ASSERT_EQ(".tflite", extension); // TODO: make this depend on which neural net platform we're building with?
  
  // Make sure model file exists
  const std::string fullModelPath = Util::FileUtils::FullFilePath({modelPath, modelFileName});
  ASSERT_TRUE(Util::FileUtils::FileExists(fullModelPath));
}

