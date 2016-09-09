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

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotDataLoader.h"
#include "anki/cozmo/basestation/visionSystem.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"
#include "json/json.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;

namespace Anki {
namespace Cozmo {
  extern u8 kMarkerDetectionFrequency;
}
}


TEST(VisionSystem, MarkerDetectionTests)
{
# define DISABLED        0
# define ENABLED         1
# define ENABLE_AND_SAVE 2
  
# define DEBUG_DISPLAY DISABLED
  
  using namespace Anki;
  
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                               Util::FileUtils::FullFilePath({"config", "basestation", "vision"}));
  
  // Make sure we are running every frame so we don't skip test images!
  // TODO: Should not be necessary once we have COZMO-3218 done (can just indicate via EnableVisionMode)
  if(Cozmo::kMarkerDetectionFrequency > 1)
  {
    PRINT_NAMED_WARNING("VisionSystem.DetectingMarkersInLowLight.AdjustingDetectionFrequency",
                        "Setting frequency to 1 instead of %d for the purposes of testing",
                        Cozmo::kMarkerDetectionFrequency);
    Cozmo::kMarkerDetectionFrequency = 1;
  }
  
  // Construct a vision system
  Cozmo::VisionSystem visionSystem(dataPath, nullptr);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);
  
  // Turn on _only_ marker detection
  visionSystem.EnableMode(Cozmo::VisionMode::Idle, true);
  visionSystem.EnableMode(Cozmo::VisionMode::DetectingMarkers, true);
  
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
  
  Vision::ImageRGB img;
  
  for(auto & testDefinition : testDefinitions)
  {
    const std::string& subDir = testDefinition.subDir;
    
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");
    
    u32 numFailures = 0;
    for(auto & filename : testFiles)
    {
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);
      
      Cozmo::VisionPoseData robotState; // not needed just to detect markers
      result = visionSystem.Update(robotState, img);
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
    ASSERT_LE(failureRate, testDefinition.expectedFailureRate);
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
  
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                               Util::FileUtils::FullFilePath({"config", "basestation", "vision"}));
  
  // Construct a vision system
  Cozmo::VisionSystem visionSystem(dataPath, nullptr);

  // Make sure we are running every frame so we don't skip test images!
  Json::Value config = cozmoContext->GetDataLoader()->GetRobotVisionConfig();
  config["ImageQuality"]["CheckFrequency"] = 1;
  
  Result result = visionSystem.Init(config);
  ASSERT_EQ(RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);
  
  // Turn on _only_ image quality check
  visionSystem.EnableMode(Cozmo::VisionMode::Idle, true);
  visionSystem.EnableMode(Cozmo::VisionMode::CheckingQuality, true);
  
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/imageQualityTests");
  
  const std::vector<std::string> testSubDirs = {
    "Good", "TooBright", "TooDark"
  };
  
  Vision::ImageRGB img;
  
  for(auto & subDir : testSubDirs)
  {
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");
    
    for(auto & filename : testFiles)
    {
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);
      
      Cozmo::VisionPoseData robotState; // not needed for image quality check
      result = visionSystem.Update(robotState, img);
      ASSERT_EQ(RESULT_OK, result);
      
      Cozmo::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);
      
      // Make sure the detected quality is as expected for this subdir
      EXPECT_EQ(subDir, EnumToString(processingResult.imageQuality));
      
      PRINT_NAMED_INFO("VisionSystem.ImageQuality", "%s = %s",
                       filename.c_str(), EnumToString(processingResult.imageQuality));
    }
  }
  
} // ImageQuality


