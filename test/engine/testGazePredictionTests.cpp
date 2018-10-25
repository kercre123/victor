/**
 * File: testGazePredictionTests.cpp
 *
 * Author: Robert Cosgriff
 * Created: 2/2/2018
 *
 * Description: Tests for gaze prediction, and eye contact
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/eyeContact.h"
#include "coretech/vision/engine/faceNormalDirectedAtRobot3d.h"

#include "coretech/common/shared/types.h"

#include "engine/components/visionComponent.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/visionSystem.h"

#include <fstream>

extern Anki::Vector::CozmoContext* cozmoContext;

static const std::vector<const char*> imageFileExtensions = {"jpg", "jpeg", "png"};

// Console vars we want to modify for the tests below
namespace Anki {
namespace Vector {
  extern bool kIgnoreFacesBelowRobot;
  extern f32 kBodyTurnSpeedThreshFace_degs;
  extern f32 kHeadTurnSpeedThreshFace_degs;
}
namespace Vision {
  extern s32 kFaceRecognitionThreshold;
  extern f32 kTimeBetweenFaceEnrollmentUpdates_sec;
  extern bool kGetEnrollmentTimeFromImageTimestamp;
  extern bool kFaceRecognitionExtraDebug;
}
}


using namespace Anki::Vision;
using namespace Anki;

TEST(EyeContact, GazeEstimationInterface)
{
  Camera camera(1);
  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
                                              -0.2198782133395603,
                                              0.001435740245449692,
                                              0.001523365725052927,
                                              0.1341471670512819,
                                              0, 0, 0}};
  auto calib = std::make_shared<CameraCalibration>(360,640,
      362.8773099149878, 366.7347434532929, 302.2888225643724,
      200.012543449327, 0, distortionCoeffs);
  camera.SetCalibration(calib);

  Json::Value config;
  Json::Value faceDetection;
  faceDetection["DetectionMode"] = "video";
  config["FaceDetection"] = faceDetection;

  Json::Value faceRecognition;
  faceRecognition["RunMode"] = "synchronous";
  config["FaceRecognition"] = faceRecognition;

  Json::Value initialVisionModes;
  initialVisionModes["DetectingFaces"] = true;
  config["InitialVisionModes"] = initialVisionModes;
  config["FaceAlbum"] = "robot";

  const std::string path = "foo";
  FaceTracker faceTracker(camera, path, config);
  faceTracker.EnableGazeDetection(true);
  faceTracker.EnableBlinkDetection(true);

  Result lastResult = RESULT_OK;

  const std::string pathToImages = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/gazeEstimationTests");
  std::vector<std::string> imageFiles = 
    Anki::Util::FileUtils::FilesInDirectory(pathToImages, true, ".jpg");

  int frameNumber = 0;
  for (auto const& filename : imageFiles) {
    Vision::Image image;
    lastResult = image.Load(filename);

    // Do the gaze estimation
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    const float cropFactor = 1.f;
    lastResult = faceTracker.Update(image, cropFactor, faces, updatedIDs);
    // We don't detect a face in the first frame (even though
    // there is one present) but should find one face in the
    // rest of the frames
    if (frameNumber > 0 ) {
      ASSERT_GE(faces.size(), 1);
      auto const& face = faces.back();

      auto const& gaze = face.GetGaze();
      ASSERT_LT(std::abs(gaze.leftRight_deg), 40);
      ASSERT_LT(std::abs(gaze.upDown_deg), 30);

      auto const& blink = face.GetBlinkAmount();
      ASSERT_LT(blink.blinkAmountLeft, 1.f);
      ASSERT_GT(blink.blinkAmountLeft, 0.f);
      ASSERT_LT(blink.blinkAmountRight, 1.f);
      ASSERT_GT(blink.blinkAmountRight, 0.f);
    }
    frameNumber += 1;
  } // imageFiles
}

TEST(EyeContact, EyeContactInterface)
{
  Json::Value gazeData;
  
  const std::string base = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                           "test/gazeEstimationTests");
  std::string inputPath = Util::FileUtils::FullFilePath({base, "gazePoints.json"});
  std::ifstream gazeDataFile(inputPath, std::ifstream::binary);
  gazeDataFile >> gazeData;

  std::map <uint , std::pair<int, int>> gazePoints;
  for (Json::Value::iterator it = gazeData.begin();
        it != gazeData.end(); it++) {
    auto key = it.key().asString();
    uint frameNumber = static_cast<uint>(std::stoi(key));

    auto value = (*it);
    auto gazeX = value[0].asInt();
    auto gazeY = value[1].asInt();

    std::pair<int, int> gazePoint(gazeX, gazeY);
    gazePoints[frameNumber] = std::move(gazePoint);
  }

  EyeContact eyeContact;
  TrackedFace face;
  TimeStamp_t timeStamp;

  for (const auto& kv: gazePoints) {
    auto frameNumber = kv.first;
    auto gazeX = kv.second.first;
    auto gazeY = kv.second.second;
    face.SetGaze(gazeX, gazeY);
    timeStamp = frameNumber;

    eyeContact.Update(face, timeStamp);
    ASSERT_FALSE(eyeContact.GetExpired(timeStamp));
    auto makingEyeContact = eyeContact.IsMakingEyeContact();

    if (frameNumber < 6) {
      ASSERT_FALSE(makingEyeContact);
    } else if (frameNumber >= 6 && frameNumber < 12) { 
      ASSERT_TRUE(makingEyeContact);
    } else {
      return;
    }
  }
}

TEST(FaceDirection, LookingAtPhone)
{
  Camera camera(1);
  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
                                              -0.2198782133395603,
                                              0.001435740245449692,
                                              0.001523365725052927,
                                              0.1341471670512819,
                                              0, 0, 0}};
  auto calib = std::make_shared<CameraCalibration>(360,640,
      362.8773099149878, 366.7347434532929, 302.2888225643724,
      200.012543449327, 0, distortionCoeffs);
  camera.SetCalibration(calib);

  Json::Value config;
  Json::Value faceDetection;
  faceDetection["DetectionMode"] = "video";
  config["FaceDetection"] = faceDetection;

  Json::Value faceRecognition;
  faceRecognition["RunMode"] = "synchronous";
  config["FaceRecognition"] = faceRecognition;

  Json::Value initialVisionModes;
  initialVisionModes["DetectingFaces"] = true;
  config["InitialVisionModes"] = initialVisionModes;
  config["FaceAlbum"] = "robot";

  const std::string path = "foo";
  FaceTracker faceTracker(camera, path, config);
  faceTracker.EnableGazeDetection(true);
  faceTracker.EnableBlinkDetection(true);

  Result lastResult = RESULT_OK;

  // const std::string pathToImages = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
  //                                                                                  "test/gazeEstimationTests");

  const std::string pathToImages = "/var/tmp/face_direction/interact_with_phone";
  std::vector<std::string> imageFiles = 
    Anki::Util::FileUtils::FilesInDirectory(pathToImages, true, ".jpg");

  int frameNumber = 0;
  for (auto const& filename : imageFiles) {
    std::cout << "Frame number " << frameNumber << std::endl;
    Vision::ImageRGB image;
    lastResult = image.Load(filename);

    // Do the gaze estimation
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    lastResult = faceTracker.Update(image.ToGray(), faces, updatedIDs);
    // We don't detect a face in the first frame (even though
    // there is one present) but should find one face in the
    // rest of the frames
    if (frameNumber > 0 ) {
      ASSERT_GE(faces.size(), 1);
      auto const& face = faces.back();

      const auto roll = face.GetHeadRoll();
      const auto pitch = face.GetHeadPitch();
      const auto yaw = face.GetHeadYaw();

      Point2f locationRollText(50, 50);
      std::string rollText = "roll: " + std::to_string(RAD_TO_DEG(roll.ToFloat()));
      image.DrawText(locationRollText, rollText, NamedColors::RED);

      Point2f locationPitchText(50, 70);
      std::string pitchText = "pitch: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      image.DrawText(locationPitchText, pitchText, NamedColors::RED);

      Point2f locationYawText(50, 90);
      std::string yawText = "yaw: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      image.DrawText(locationYawText, yawText, NamedColors::RED);

      std::ostringstream localOSS;  
      localOSS << "/tmp/interact_with_phone_" << std::setw(10) << std::setfill('0') << frameNumber << ".jpg";  
      std::string outputFilename = localOSS.str();  
      image.Save(outputFilename);

      // Probably worth saving this out on the image
      std::cout << "Roll: " << RAD_TO_DEG(roll.ToFloat()) << " Pitch: " << RAD_TO_DEG(pitch.ToFloat())
                << " Yaw: " << RAD_TO_DEG(yaw.ToFloat()) << std::endl;
    }
    frameNumber += 1;
  } // imageFiles
}

TEST(FaceDirection, LookingAtPhoneVisionSystem)
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
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(RESULT_OK, result);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  auto calib = std::make_shared<Vision::CameraCalibration>(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);

  // Turn on _only_ marker detection
  result = visionSystem.SetNextMode(Vector::VisionMode::Idle, true);
  ASSERT_EQ(RESULT_OK, result);

  result = visionSystem.SetNextMode(Vector::VisionMode::DetectingFaces, true);
  ASSERT_EQ(RESULT_OK, result);
  
  result = visionSystem.SetNextMode(Vector::VisionMode::DetectingGaze, true);
  ASSERT_EQ(RESULT_OK, result);

  // Make sure we run on every frame
  result = visionSystem.PushNextModeSchedule(Vector::AllVisionModesSchedule({
    {Vector::VisionMode::DetectingFaces, Vector::VisionModeSchedule(1)},
    {Vector::VisionMode::DetectingGaze, Vector::VisionModeSchedule(1)},
  }));
  ASSERT_EQ(RESULT_OK, result);

  /*
  // Grab all the test images from "resources/test/lowLightMarkerDetectionTests"
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/markerDetectionTests");
  */
  const std::string pathToImages = "/var/tmp/face_direction/interact_with_phone";

  Vision::ImageCache imageCache;


  std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(pathToImages, false, ".jpg");

  EXPECT_FALSE(testFiles.empty());
 
  unsigned int frameNumber = 0; 
  for(auto & filename : testFiles)
  {
    Vision::ImageRGB image;
    result = image.Load(Util::FileUtils::FullFilePath({pathToImages, filename}));
    ASSERT_EQ(RESULT_OK, result);

    imageCache.Reset(image);

    Vector::VisionPoseData robotState; // not needed just to detect markers
    robotState.cameraPose.SetParent(robotState.histState.GetPose()); // just so we don't trigger an assert
    result = visionSystem.Update(robotState, imageCache);
    ASSERT_EQ(RESULT_OK, result);

    Vector::VisionProcessingResult processingResult;
    bool resultAvailable = visionSystem.CheckMailbox(processingResult);
    EXPECT_TRUE(resultAvailable);


    // We don't detect a face in the first frame (even though
    // there is one present) but should find one face in the
    // rest of the frames
    if (frameNumber > 0 ) {
      ASSERT_GT(processingResult.faces.size(), 0);
      const auto& faces = processingResult.faces;
      ASSERT_GE(faces.size(), 1);
      auto const& face = faces.back();

      const auto roll = face.GetHeadRoll();
      const auto pitch = face.GetHeadPitch();
      const auto yaw = face.GetHeadYaw();

      Point2f locationRollText(50, 50);
      std::string rollText = "roll: " + std::to_string(RAD_TO_DEG(roll.ToFloat()));
      image.DrawText(locationRollText, rollText, NamedColors::RED);

      Point2f locationPitchText(50, 70);
      std::string pitchText = "pitch: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      image.DrawText(locationPitchText, pitchText, NamedColors::RED);

      Point2f locationYawText(50, 90);
      std::string yawText = "yaw: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      image.DrawText(locationYawText, yawText, NamedColors::RED);

      std::ostringstream localOSS;  
      localOSS << "/tmp/interact_with_phone_" << std::setw(10) << std::setfill('0') << frameNumber << ".jpg";  
      std::string outputFilename = localOSS.str();  
      image.Save(outputFilename);

      // Probably worth saving this out on the image
      std::cout << "Roll: " << RAD_TO_DEG(roll.ToFloat()) << " Pitch: " << RAD_TO_DEG(pitch.ToFloat())
                << " Yaw: " << RAD_TO_DEG(yaw.ToFloat()) << std::endl;
    }

    frameNumber += 1;

    if(DEBUG_DISPLAY != DISABLED && ((DEBUG_DISPLAY != ENABLE_ON_FAIL)))
    {
      for(auto const& debugImg : processingResult.debugImageRGBs)
      {
        debugImg.second.Display(debugImg.first.c_str());
      }
    }
  }

# undef DEBUG_DISPLAY
# undef ENABLED
# undef DISABLED
# undef ENABLE_AND_SAVE

} // MarkerDetectionTests

static void ProcessImage(Anki::Vector::Robot& robot, RobotTimeStamp_t timestamp, Anki::Vector::RobotState& stateMsg, Vision::Image& img,
                         const std::string& filename)
{
  std::cout << "Processing images!" << std::endl;
  std::cout << "Frame period: " << robot.GetVisionComponent().GetFramePeriod_ms() <<
    " processing period: " << robot.GetVisionComponent().GetProcessingPeriod_ms() << std::endl;
  Result lastResult = RESULT_OK;
  
  stateMsg.timestamp = (TimeStamp_t)timestamp;
  std::cout << "About to update robot state with time stamp " << timestamp << std::endl;
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  if(filename.empty())
  {
    DEV_ASSERT(!img.IsEmpty(), "FaceRecognitionTests.Recognize.EmptyImage");
    img.FillWith(128);
  }
  else
  {
    std::cout << "The file I'm about to open is " << filename << std::endl;
    lastResult = img.Load(filename);
    ASSERT_EQ(RESULT_OK, lastResult);
  }
  
  img.SetTimestamp((TimeStamp_t)timestamp);

  std::cout << "Image is rows " << img.GetNumRows() << " and cols " << img.GetNumCols() << std::endl;;
  
  Vision::ImageRGB imgRGB(img);
  std::cout << "Image is rows " << imgRGB.GetNumRows() << " and cols " << imgRGB.GetNumCols() << std::endl;;
  Vision::ImageBuffer buffer(reinterpret_cast<u8*>(imgRGB.GetDataPointer()),
                             imgRGB.GetNumRows(),
                             imgRGB.GetNumCols(),
                             Vision::ImageEncoding::RawRGB,
                             (TimeStamp_t)timestamp,
                             0);
  if (buffer.GetFormat() == Vision::ImageEncoding::RawRGB) {
    std::cout << "Looks like format is set to what i would expect " << std::endl;
  }
  std::cout << "Setting next image!" << std::endl;
  lastResult = robot.GetVisionComponent().SetNextImage(buffer);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  std::cout << "Updating all the vision results" << std::endl;
  lastResult = robot.GetVisionComponent().UpdateAllResults();
  ASSERT_TRUE(RESULT_OK == lastResult);

  if(1)
  {
    Vision::ImageRGB dispImg(img);

    auto faceIDs = robot.GetFaceWorld().GetFaceIDs(timestamp);
    std::list<Vision::TrackedFace> faces;
    for(auto faceID : faceIDs)
    {
      faces.push_back(*robot.GetFaceWorld().GetFace(faceID));
    }

    if (faces.size() > 0) {
      std::cout << "We have faces " << faces.size()  << " for our test!" << std::endl;
    } else {
      std::cout << "No faces!" << std::endl;
    }
    
    for(auto & face : faces) {
      //PRINT_NAMED_INFO("FaceRecognition.PairwiseComparison.Recognize",
      //                 "FaceTracker observed face ID %llu at t=%d",
      //                 face.GetID(), img.GetTimestamp());
      
      
      // ColorRGBA drawColor = NamedColors::GREEN; // Unidentified

      const auto roll = face.GetHeadRoll();
      const auto pitch = face.GetHeadPitch();
      const auto yaw = face.GetHeadYaw();

      /*
      Point2f locationRollText(50, 50);
      std::string rollText = "roll: " + std::to_string(RAD_TO_DEG(roll.ToFloat()));
      img.DrawText(locationRollText, rollText, NamedColors::RED);

      Point2f locationPitchText(50, 70);
      std::string pitchText = "pitch: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      img.DrawText(locationPitchText, pitchText, NamedColors::RED);

      Point2f locationYawText(50, 90);
      std::string yawText = "yaw: " + std::to_string(RAD_TO_DEG(pitch.ToFloat()));
      img.DrawText(locationYawText, yawText, NamedColors::RED);
      */

      std::cout << "Roll: " << RAD_TO_DEG(roll.ToFloat()) << " Pitch: " << RAD_TO_DEG(pitch.ToFloat())
                << " Yaw: " << RAD_TO_DEG(yaw.ToFloat()) << std::endl;

      /*      
      dispImg.DrawRect(face.GetRect(), drawColor, 2);
      Point2f leftEye, rightEye;
      if(face.GetEyeCenters(leftEye, rightEye)) {
        dispImg.DrawCircle(leftEye,  drawColor, 2);
        dispImg.DrawCircle(rightEye, drawColor, 2);
      }
      
      std::string label = face.GetName();
      if(label.empty()) {
        label = std::to_string(face.GetID());
      }
      if(face.IsBeingTracked()) {
        label += "*";
      }
      dispImg.DrawText(face.GetRect().GetTopLeft(), label, drawColor, 1.25f);

      // Draw features
      for(s32 iFeature=0; iFeature<(s32)Vision::TrackedFace::NumFeatures; ++iFeature)
      {
        Vision::TrackedFace::FeatureName featureName = (Vision::TrackedFace::FeatureName)iFeature;
        const Vision::TrackedFace::Feature& feature = face.GetFeature(featureName);
        
        }
      }
      */
    }
  
    dispImg.DrawText(Point2f(1.f, dispImg.GetNumRows()-1), std::to_string(img.GetTimestamp()),
                     NamedColors::RED, 0.6f, true);
    // TODO need a better name
    // dispImg.Display("Image");
    
  } // if(SHOW_IMAGES)

} // Recognize()

TEST(FaceDirection, FaceDirectionVisionComponent)
{

  std::cout << "FaceDirection and vision " << std::endl;

  // TODO do i really need this?
  Result lastResult = RESULT_OK;

  // TODO this needs to be the correct directory 
  // const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "test/faceRecVideoTests/");
  const std::string dataPath = "/var/tmp/face_direction/interact_with_phone";
  
  Vision::Image img;
  
  // Find all directories of face images, one per person
  std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(dataPath, false, ".jpg");

  // Get default config and make it use synchronous mode for face recognition
  Json::Value& config = cozmoContext->GetDataLoader()->GetRobotVisionConfigUpdatableRef();
  config["FaceRecognition"]["RunMode"] = "synchronous";
  
  const u16 HEAD_CAM_CALIB_WIDTH  = 320;
  const u16 HEAD_CAM_CALIB_HEIGHT = 240;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_X = 290.f;
  const f32 HEAD_CAM_CALIB_FOCAL_LENGTH_Y = 290.f;
  const f32 HEAD_CAM_CALIB_CENTER_X       = 160.f;
  const f32 HEAD_CAM_CALIB_CENTER_Y       = 120.f;
  
  auto camCalib = std::make_shared<Vision::CameraCalibration>(HEAD_CAM_CALIB_HEIGHT, HEAD_CAM_CALIB_WIDTH,
                                                              HEAD_CAM_CALIB_FOCAL_LENGTH_X, HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
                                                              HEAD_CAM_CALIB_CENTER_X, HEAD_CAM_CALIB_CENTER_Y);
  
  // Since we're processing faster than real time (using saved images)
  Vision::kTimeBetweenFaceEnrollmentUpdates_sec = 1.f;
  Vision::kGetEnrollmentTimeFromImageTimestamp = true;
  
  // Since the fake robot is in arbitrary pose
  Anki::Vector::kIgnoreFacesBelowRobot = false;
  
  // So we don't have to fake IMU data, disable WasMovingTooFast checks
  Anki::Vector::kBodyTurnSpeedThreshFace_degs = -1.f;
  Anki::Vector::kHeadTurnSpeedThreshFace_degs = -1.f;
  
  Vision::kFaceRecognitionExtraDebug = true;
  
  RobotTimeStamp_t fakeTime = 100000;
  
  s32 totalFalsePositives = 0;
  
  DependencyManagedEntity<Anki::Vector::RobotComponentID> dependentComps;
  dependentComps.AddDependentComponent(Anki::Vector::RobotComponentID::CozmoContextWrapper, new Anki::Vector::ContextWrapper(cozmoContext));

  // All-new robot, face tracker, and face world for each person for this test
  Anki::Vector::Robot robot(1, cozmoContext);
  robot.FakeSyncRobotAck();

  // Fake a state message update for robot
  Anki::Vector::RobotState stateMsg( Anki::Vector::Robot::GetDefaultRobotState() );
  
  robot.GetVisionComponent().SetIsSynchronous(true);
  robot.GetVisionComponent().InitDependent(&robot, dependentComps);
  
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  robot.GetVisionComponent().EnableMode(Anki::Vector::VisionMode::Idle, true);
  robot.GetVisionComponent().EnableMode(Anki::Vector::VisionMode::DetectingFaces, true);
  robot.GetVisionComponent().EnableMode(Anki::Vector::VisionMode::DetectingGaze, true);
  robot.GetVisionComponent().Enable(true);
  
  // Enroll each person marked for training
  const TimeStamp_t kEnrollTimeInc = 1500; // Pretend there's this much time b/w each image
  for(auto const& test : testFiles)
  {
    const std::string fullFilename = Util::FileUtils::FullFilePath({dataPath, test});
    ProcessImage(robot, fakeTime, stateMsg, img, fullFilename);
    fakeTime += kEnrollTimeInc;
    ASSERT_EQ(RESULT_OK, lastResult);
  }
  
  // Allow session-only enrollment
  robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
  
  ASSERT_EQ(0, totalFalsePositives);

  // Clean up after ourselves
  Util::FileUtils::RemoveDirectory("testAlbum");
  
  ASSERT_TRUE(RESULT_OK == lastResult);
}
