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
#include "anki/common/basestation/math/rotation.h"

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/vision/basestation/imageCache.h"

#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"
#include "json/json.h"

//#include "opencv2/calib3d/calib3d.hpp"

extern Anki::Cozmo::CozmoContext* cozmoContext;


TEST(VisionSystem, MarkerDetectionCameraCalibrationTests)
{
//  using namespace Anki;
//  auto img = cv::imread("/Users/alchaussee/Desktop/images_24180_0.jpg");
//  auto img = cv::imread("/Users/alchaussee/Desktop/images_22165_12.jpg");
//  std::vector<cv::KeyPoint> keypoints;
//  auto detector = cv::SimpleBlobDetector::create();
//  detector->detect(img, keypoints);
//  
//  std::sort(keypoints.begin(), keypoints.end(), [](cv::KeyPoint x, cv::KeyPoint y){ return x.pt.y<y.pt.y; });
//  
//  
//  cv::Mat out = img;
//  int i = 0;
//  for(auto p : keypoints)
//  {
//    cv::putText(out, std::to_string(i), keypoints[i].pt, cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(0,0,255));
//    i++;
//  }

  /*
  
   _
  | |
   - _
    | |
     - _
      | |
       - ...
   
   
    ^
    R
  
  ^ +y
  |
   -> +x
   +z out
  
   Marker corners are defined relative to center of bottom left cube in target in this orientation (before rotations
   are applied to get cubes to their actual positions)
   FrontFace is the marker that is facing the robot in this orientation. !FrontMarker is the left marker, that will be
   visible when rotation are applied (rotate 45degree on Z and then -30 degree in Y in this origin)
  */

  const Anki::Quad3f originsFrontFace({{12.5, -22, -12.5},
    {-12.5, -22, -12.5},
    {12.5, -22, 12.5},
    {-12.5, -22, 12.5}});
  
  const Anki::Quad3f originsLeftFace({{-22, -12.5, -12.5},
    {-22, 12.5, -12.5},
    {-22, -12.5, 12.5},
    {-22, 12.5, 12.5}});
  
  auto GetCoordsForFace = [&originsLeftFace, &originsFrontFace](bool isFrontFace,
                                 int numCubesRightOfOrigin,
                                 int numCubesAwayRobotFromOrigin,
                                 int numCubesAboveOrigin)
  {
    
    Anki::Quad3f whichFace = (isFrontFace ? originsFrontFace : originsLeftFace);

    Anki::Pose3d p;
    p.SetTranslation({44.f * numCubesRightOfOrigin,
                      44.f * numCubesAwayRobotFromOrigin,
                      44.f * numCubesAboveOrigin});
    
    p.ApplyTo(whichFace, whichFace);
    return whichFace;
  };
  
  std::map<Anki::Vision::MarkerType, Anki::Quad3f> _markerTo3dCoords;
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_RIGHT] = GetCoordsForFace(true, 0, 0, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_LEFT] = GetCoordsForFace(false, 1, -1, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_FRONT] = GetCoordsForFace(true, 1, -1, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_BOTTOM] = GetCoordsForFace(false, 2, -2, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_BACK] = GetCoordsForFace(true, 2, -2, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_TOP] = GetCoordsForFace(false, 3, -3, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_RIGHT] = GetCoordsForFace(true, 3, -3, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_LEFT] = GetCoordsForFace(false, 4, -4, 0);
  
  
  Anki::Cozmo::VisionSystem visionSystem(cozmoContext);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Anki::Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
//  Anki::Vision::CameraCalibration calib(720,1280,290.f,290.f,160.f,120.f,0.f);
//  result = visionSystem.UpdateCameraCalibration(calib);
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//  
//  // Turn on _only_ marker detection
//  result = visionSystem.SetNextMode(Anki::Cozmo::VisionMode::Idle, true);
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//  
//  result = visionSystem.SetNextMode(Anki::Cozmo::VisionMode::DetectingMarkers, true);
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//  
//  // Make sure we run on every frame
//  result = visionSystem.PushNextModeSchedule(Anki::Cozmo::AllVisionModesSchedule({{Anki::Cozmo::VisionMode::DetectingMarkers, Anki::Cozmo::VisionModeSchedule(1)}}));
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//
//  Anki::Vision::ImageCache imageCache;
//  
//  Anki::Vision::ImageRGB img;
//  result = img.Load("/Users/alchaussee/Desktop/images_29515_0.jpg");
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//  
//  imageCache.Reset(img);
//  
//  Anki::Cozmo::VisionPoseData robotState; // not needed just to detect markers
//  result = visionSystem.Update(robotState, imageCache);
//  ASSERT_EQ(Anki::Result::RESULT_OK, result);
//  
//  Anki::Cozmo::VisionProcessingResult processingResult;
//  bool resultAvailable = visionSystem.CheckMailbox(processingResult);
//  EXPECT_TRUE(resultAvailable);
  
  
  
  /*
  std::vector<std::vector<cv::Vec2f>> imgPts = {{
                             {keypoints[0].pt.x, keypoints[0].pt.y},
                             {keypoints[1].pt.x, keypoints[1].pt.y},
                             {keypoints[2].pt.x, keypoints[2].pt.y},
                             {keypoints[3].pt.x, keypoints[3].pt.y},
                             {keypoints[4].pt.x, keypoints[4].pt.y},
                             {keypoints[5].pt.x, keypoints[5].pt.y},
                             {keypoints[6].pt.x, keypoints[6].pt.y},
                             {keypoints[7].pt.x, keypoints[7].pt.y}}};
  
//  float x = 0.005 * cos(DEG_TO_RAD(13.8));
//  float z = 0.005 * sin(DEG_TO_RAD(13.8));
  float x = 0.005 * cos(DEG_TO_RAD(9.2));
  float z = 0.005 * sin(DEG_TO_RAD(9.2));
  std::vector<std::vector<cv::Vec3f>> worldPts = {{
                               {0.1f-x,   0.03, 0.1f-z},
                               {0.1f-x,  -0.03, 0.09f-z},
                               {0.2f-x,   0.03, 0.1f-z},
                               {0.2f-x,  -0.03, 0.08f-z},
                               {0.15f-x,  0.03, 0.06f-z},
                               {0.15f-x, -0.03, 0.05f-z},
                               {0.12f-x,  0,    0.04f-z},
                               {0.16f-x,  0,    0.02f-z}}};
  
  cv::Mat_<double> D = cv::Mat::zeros(8, 1, CV_64F);
  std::vector<cv::Mat_<double>> R2;
  R2.push_back(cv::Mat::zeros(3, 3, CV_64F));
  std::vector<cv::Mat_<double>> T2;
  T2.push_back(cv::Mat::zeros(3,3,CV_64F));
  
  cv::Mat_<double> K2 = (cv::Mat_<double>(3,3) <<
                          100, 0, 160,
                          0, 100, 120,
                          0, 0, 1);
  
  cv::calibrateCamera(worldPts, imgPts, cv::Size(img.cols, img.rows),
                      K2, D, R2, T2,
                      cv::CALIB_USE_INTRINSIC_GUESS);
  
    std::stringstream ss;
    ss << K2 << std::endl;
    PRINT_NAMED_WARNING("K", "%s", ss.str().c_str());
  
  std::stringstream ss1;
  ss1 << D << std::endl;
  PRINT_NAMED_WARNING("D", "%s", ss1.str().c_str());
  */
}

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
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
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
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
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


