#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "engine/components/visionComponent.h"
#include "engine/faceWorld.h"
#include "engine/robot.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/visionSystem.h"

#include "coretech/vision/engine/faceTracker.h"

#include "clad/types/enrolledFaceStorage.h"

#include <dirent.h>
#include <regex>
#include <thread>

// TODO: Remove this once we sort build failures (See COZMO-797)
#define SKIP_FACE_RECOGNITION_TESTS 1

#define SHOW_IMAGES 0

// Increase this (e.g. to max int) in order to run longer tests using all the available video frames
// The limit is here to keep the test time shorter for general use.
static const s32 MaxImagesPerDir = 50;

// Number of blank frames to show the tracker to get it to lose track b/w people
static const s32 NumBlankFramesBetweenPeople = 5;

// Make sure we detected/recognized a high enough percentage of faces and that
// we didn't add excessive IDs to the album in the process. These are checked
// per user-directory of video frames
//static const f32 ExpectedDetectionPercent   = 90.f;
//static const f32 ExpectedRecognitionPercent = 80.f;
//static const s32 ExpectedNumFaceIDsLimit    = 10;
//static const f32 ExpectedFalsePositivePercent = 1.f;

extern Anki::Vector::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Vector;

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

static const std::vector<const char*> imageFileExtensions = {"jpg", "jpeg", "png"};

void InitRobot(Robot& robot){
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
  kIgnoreFacesBelowRobot = false;
  
  // So we don't have to fake IMU data, disable WasMovingTooFast checks
  kBodyTurnSpeedThreshFace_degs = -1.f;
  kHeadTurnSpeedThreshFace_degs = -1.f;
  
  Vision::kFaceRecognitionExtraDebug = true;
  
  
  //const TimeStamp_t kTestTimeInc = 65;
  
  //s32 totalFalsePositives = 0;
  
  DependencyManagedEntity<RobotComponentID> dependentComps;
  dependentComps.AddDependentComponent(RobotComponentID::CozmoContextWrapper, new ContextWrapper(cozmoContext));

  robot.FakeSyncRobotAck();
  robot.GetVisionComponent().SetIsSynchronous(true);
  robot.GetVisionComponent().InitDependent(&robot, dependentComps);
  robot.GetVisionComponent().SetCameraCalibration(camCalib);
  robot.GetVisionComponent().EnableMode(VisionMode::Idle, true);
  robot.GetVisionComponent().EnableMode(VisionMode::DetectingFaces, true);
  robot.GetVisionComponent().Enable(true);

  return;
}

static void Recognize(Robot& robot, RobotTimeStamp_t timestamp, RobotState& stateMsg, Vision::Image& img,
                      const std::string& filename, const char *dispName, const std::set<std::string>& namesPresent)
{
  Result lastResult = RESULT_OK;
  
  stateMsg.timestamp = (TimeStamp_t)timestamp;
  lastResult = robot.UpdateFullRobotState(stateMsg);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  if(filename.empty())
  {
    DEV_ASSERT(!img.IsEmpty(), "FaceRecognitionTests.Recognize.EmptyImage");
    img.FillWith(128);
  }
  else
  {
    lastResult = img.Load(filename);
    ASSERT_EQ(RESULT_OK, lastResult);
  }
  
  img.SetTimestamp((TimeStamp_t)timestamp);
  
  Vision::ImageRGB imgRGB(img);
  lastResult = robot.GetVisionComponent().SetNextImage(imgRGB);
  ASSERT_EQ(RESULT_OK, lastResult);
  
  lastResult = robot.GetVisionComponent().UpdateAllResults();
  ASSERT_TRUE(RESULT_OK == lastResult);

  if(SHOW_IMAGES)
  {
    Vision::ImageRGB dispImg(img);

    auto faceIDs = robot.GetFaceWorld().GetFaceIDs(timestamp);
    std::list<Vision::TrackedFace> faces;
    for(auto faceID : faceIDs)
    {
      faces.push_back(*robot.GetFaceWorld().GetFace(faceID));
    }
    
    for(auto & face : faces) {
      //PRINT_NAMED_INFO("FaceRecognition.PairwiseComparison.Recognize",
      //                 "FaceTracker observed face ID %llu at t=%d",
      //                 face.GetID(), img.GetTimestamp());
      
      
      ColorRGBA drawColor = NamedColors::ORANGE; // Unidentified
      if(face.HasName() && !namesPresent.empty())
      {
        if(namesPresent.count(face.GetName())>0)
        {
          drawColor = NamedColors::GREEN; // True positive
        }
        else
        {
          drawColor = NamedColors::RED; // False positive
        }
      }
      
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
        
        for(size_t crntPoint=0, nextPoint=1; nextPoint < feature.size(); ++crntPoint, ++nextPoint) {
          dispImg.DrawLine(feature[crntPoint], feature[nextPoint], drawColor, 1);
        }
      }

    }
  
    dispImg.DrawText(Point2f(1.f, dispImg.GetNumRows()-1), std::to_string(img.GetTimestamp()),
                     NamedColors::RED, 0.6f, true);
    dispImg.Display(dispName);
    
  } // if(SHOW_IMAGES)

} // Recognize()

Result EnrollFace(Robot& robot, RobotTimeStamp_t& fakeTime, RobotState& stateMsg, Vision::Image& img, const std::string& dataPath, const std::string& userDir)
{
  const std::string& enrollSubDir = "enroll";
  
  robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
  
  // Get the images to use for enrollment
  auto enrollFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({dataPath, userDir, enrollSubDir}),
                                                       true, imageFileExtensions);
  if(enrollFiles.empty())
  {
    PRINT_NAMED_ERROR("TestFaceRecognition.Enroll.NoEnrollFiles", "");
    return RESULT_FAIL;
  }
  
  // We will pick the enrollment ID once we see the first face
  Vision::FaceID_t enrollmentID = Vision::UnknownFaceID;
  
  const TimeStamp_t kEnrollTimeInc = 1500; // Pretend there's this much time b/w each image
  
  const s32 numFiles = std::min(MaxImagesPerDir, (s32)enrollFiles.size());
  for(s32 iFile=0; iFile < numFiles; ++iFile, fakeTime+=kEnrollTimeInc)
  {
    const std::string& enrollFile = enrollFiles[iFile];

    Recognize(robot, fakeTime, stateMsg, img, enrollFile, "EnrollImage", {userDir});
    
    // Get the faces observed in the current image
    auto observedFaceIDs = robot.GetFaceWorld().GetFaceIDs(img.GetTimestamp());

    if(observedFaceIDs.empty())
    {
      continue;
    }
    
    if(observedFaceIDs.size() > 1)
    {
      PRINT_NAMED_WARNING("TestFaceRecognition.Enroll.SkippingEnrollImageWithMultipleFaces",
                          "numFacesObserved=%zu", observedFaceIDs.size());
      continue;
    }
    
    const Vision::FaceID_t observedID = *observedFaceIDs.begin();
    
    if(observedID >= 0)
    {
      if(Vision::UnknownFaceID == enrollmentID)
      {
        const Vision::TrackedFace* observedFace = robot.GetFaceWorld().GetFace(observedID);
        if(observedFace->HasName())
        {
          PRINT_NAMED_ERROR("TestFaceRecognition.Enroll.RecognizedFaceToEnroll",
                            "Enrolling '%s', but recognized '%s'(%d)",
                            userDir.c_str(), observedFace->GetName().c_str(), observedID);
          return RESULT_FAIL;
        }
        
        // Recognized, not just tracked: start enrolling the ID
        enrollmentID = observedID;
        robot.GetVisionComponent().SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight,
                                                         enrollmentID,
                                                         (int)Vision::FaceRecognitionConstants::MaxNumEnrollDataPerAlbumEntry);
        
      }
      else
      {
        auto enrollingFace = robot.GetFaceWorld().GetFace(enrollmentID);
        if(nullptr != enrollingFace && enrollingFace->GetNumEnrollments() > 0)
        {
          EXPECT_EQ((int)Vision::FaceRecognitionConstants::MaxNumEnrollDataPerAlbumEntry,
                    enrollingFace->GetNumEnrollments());
          
          robot.GetVisionComponent().AssignNameToFace(enrollmentID, userDir);
          PRINT_NAMED_INFO("FaceRecognition.VideoRecognitionAndTracking.AddingName",
                           "Registering '%s' to ID %d", userDir.c_str(), enrollmentID);

          // Turn "off" enrollment
          enrollmentID = Vision::UnknownFaceID;
          fakeTime += kEnrollTimeInc;
          return RESULT_OK;
        }
      }
    }
  } // for each image file
  
  PRINT_NAMED_ERROR("TestFaceRecognition.Enroll.RanOutOfImages",
                    "Failed to complete enrollment of '%s' after %zu images",
                    userDir.c_str(), enrollFiles.size());
  return RESULT_FAIL;
}

Result ShowBlankFrames(s32 N, Robot& robot, RobotTimeStamp_t& fakeTime, RobotState& stateMsg, Vision::Image& img, const char* dispName)
{
  Result lastResult = RESULT_OK;
  
  const TimeStamp_t kBlankTimeInc = 10;
  
  // Show the system blank frames before next video so it stops tracking
  for(s32 iBlank=0; iBlank<N; ++iBlank, fakeTime+=kBlankTimeInc)
  {
    Recognize(robot, fakeTime, stateMsg, img, "", dispName, std::set<std::string>());
    
    // Since we won't detect anything in these blank frames, we could
    // move through them too quickly and cause the tracker to persist
    // across people (??)
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    
    // We should not detect faces in any frames past the "lost" count
    if(iBlank >= 2) {
      auto observedFaceIDs = robot.GetFaceWorld().GetFaceIDs(img.GetTimestamp());
      if(!observedFaceIDs.empty())
      {
        lastResult = RESULT_FAIL;
      }
    }
  }
  
  fakeTime += kBlankTimeInc;
  
  return lastResult;
}

Result RunFacialRecognizer(Robot &robot){
  //TODO: Implement the code that looks into the faces detected to find if the ones enrolled are recognized
  return RESULT_OK;
}

Result EnrollPerson(const char* person, Robot& robot){
  //TODO: Implement the code that enrolls the given person into the robot
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "test/faceRecognitionAutomation/Enroll");

  RobotTimeStamp_t fakeTime = 100000;
  // Fake a state message update for robot
  RobotState stateMsg( Robot::GetDefaultRobotState());
  Vision::Image img;

  // Get the names of all the people in the Enroll Directory
  std::vector<std::string> peopleDirs;
  Util::FileUtils::ListAllDirectories(dataPath, peopleDirs);

  std::set<std::string> allNames(peopleDirs.begin(), peopleDirs.end());
  //TODO: Check if person is actually in allNames
  // Get default config and make it use synchronous mode for face recognition
  Json::Value& config = cozmoContext->GetDataLoader()->GetRobotVisionConfigUpdatableRef();
  config["FaceRecognition"]["RunMode"] = "synchronous";

  

  Result lastResult = EnrollFace(robot, fakeTime, stateMsg, img, dataPath, person);
  if(lastResult != RESULT_OK){
    PRINT_NAMED_ERROR("FaceRecognition.VideoRecognitionAndTrackingAutomation.EnrollImage",
                      "Failed to enroll %s", person);
    return RESULT_FAIL;
  }
  
  lastResult = ShowBlankFrames(NumBlankFramesBetweenPeople, robot, fakeTime, stateMsg, img, "EnrollImage");
  if(lastResult != RESULT_OK){
    PRINT_NAMED_ERROR("FaceRecognition.VideoRecognitionAndTrackingAutomation.EnrollImage",
                      "Failed at ShowBlankFrames after enrolling %s", person);
    return RESULT_FAIL;
  }

  return RESULT_OK;
}

TEST(FaceRecognition, VideoRecognitionAndTrackingAutomation){

  if(SKIP_FACE_RECOGNITION_TESTS) {
    return;
  }

  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_ERROR("FaceRecognition.VideoRecognitionAndTrackingAutomation.RecognitionNotSupported",
                      "Skipping test because the compiled face tracker does not support recognition");
    return;
  }

  Robot robot(1, cozmoContext);
  InitRobot(robot);

  Result lastResult = RESULT_OK;

  lastResult = EnrollPerson("Andrew", robot);
  ASSERT_EQ(RESULT_OK, lastResult);

  lastResult = RunFacialRecognizer(robot);
  ASSERT_EQ(RESULT_OK, lastResult);
}
