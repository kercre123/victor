#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "anki/common/types.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"

#include "anki/vision/basestation/faceTracker.h"
#include <dirent.h>
#include <regex>
#include <thread>

// TODO: Remove this once we sort build failures (See COZMO-797)
#define SKIP_FACE_RECOGNITION_TESTS 1

#define SHOW_IMAGES 0

// Increase this (e.g. to max int) in order to run longer tests using all the available video frames
// The limit is here to keep the test time shorter for general use.
static const s32 MaxImagesPerDir = 50;


// Make sure we detected/recognized a high enough percentage of faces and that
// we didn't add excessive IDs to the album in the process. These are checked
// per user-directory of video frames
static const f32 ExpectedDetectionPercent   = 90.f;
static const f32 ExpectedRecognitionPercent = 80.f;
static const s32 ExpectedNumFaceIDsLimit    = 10;
static const f32 ExpectedFalsePositivePercent = 1.f;

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Cozmo;

namespace Anki {
namespace Vision {
  // Console vars we want to modify for the tests below
  extern s32 kFaceRecognitionThreshold;
  extern s32 kFaceMergeThreshold;
  extern f32 kTimeBetweenInitialFaceEnrollmentUpdates_sec;
}
}

static const std::vector<const char*> imageFileExtensions = {"jpg", "jpeg", "png"};

// Helper lambda to pass an image file into the vision component and
// then update FaceWorld with any resulting detections
static void Recognize(Robot& robot, Vision::Image& img, Vision::FaceTracker& faceTracker,
                      const std::string& filename, const char *dispName, bool shouldBeOwner)
{
  Result lastResult = RESULT_OK;
  static TimeStamp_t timestamp = 0;
  
  timestamp += 30; // fake time
  img.SetTimestamp(timestamp);
  
  if(filename.empty()) {
    ASSERT_NAMED(!img.IsEmpty(), "Expected non-empty image by now");
    img.FillWith(128);
  } else {
    lastResult = img.Load(filename);
    ASSERT_TRUE(RESULT_OK == lastResult);
  }
  
# if SHOW_IMAGES
  Vision::ImageRGB dispImg(img);
# endif
  
  std::list<Vision::TrackedFace> faces;
  std::list<Vision::UpdatedFaceID> updatedIDs;
  lastResult = faceTracker.Update(img, faces, updatedIDs);
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
  for(auto & updateID : updatedIDs)
  {
    Result changeResult = robot.GetFaceWorld().ChangeFaceID(updateID.oldID, updateID.newID);
    ASSERT_EQ(changeResult, RESULT_OK);
  }
  
  for(auto & face : faces) {
    //PRINT_NAMED_INFO("FaceRecognition.PairwiseComparison.Recognize",
    //                 "FaceTracker observed face ID %llu at t=%d",
    //                 face.GetID(), img.GetTimestamp());
    
    // Do some bogus pose parent setting to keep face world from complaining
    Pose3d headPose = face.GetHeadPose();
    headPose.SetParent(robot.GetWorldOrigin());
    face.SetHeadPose(headPose);
    
    lastResult = robot.GetFaceWorld().AddOrUpdateFace(face);
    ASSERT_TRUE(RESULT_OK == lastResult);
    
#   if SHOW_IMAGES
    const ColorRGBA& drawColor = ((shouldBeOwner && face.GetID()==1) || (!shouldBeOwner && face.GetID()!=1) ?
                                  NamedColors::GREEN : NamedColors::RED);
    dispImg.DrawRect(face.GetRect(), drawColor, 2);
    Point2f leftEye, rightEye;
    if(face.GetEyeCenters(leftEye, rightEye)) {
      dispImg.DrawPoint(leftEye,  drawColor, 2);
      dispImg.DrawPoint(rightEye, drawColor, 2);
    }
    
    std::string label = face.GetName();
    if(label.empty()) {
      label = std::to_string(face.GetID());
    }
    if(face.IsBeingTracked()) {
      label += "*";
    }
    dispImg.DrawText(face.GetRect().GetTopLeft(), label, drawColor, 1.25f);
#   endif
  }
  
# if SHOW_IMAGES
  if(nullptr != dispName) {
    dispImg.Display(dispName, 30);
  }
# endif
  
  lastResult = robot.GetFaceWorld().Update();
  ASSERT_TRUE(RESULT_OK == lastResult);
} // Recognize()


TEST(FaceRecognition, VideoRecognitionAndTracking)
{
  if(SKIP_FACE_RECOGNITION_TESTS) {
    return;
  }

  Result lastResult = RESULT_OK;
  
  Robot robot(1, cozmoContext);
  
  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_WARNING("FaceRecognition.VideoRecognitionAndTracking.RecognitionNotSupported",
                        "Skipping test because the compiled face tracker does not support recognition");
    return;
  }
  
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "test/faceRecVideoTests/");
  
  // All-new tracker and face world for each person for this test
  Vision::FaceTracker* faceTracker = nullptr;
  robot.GetFaceWorld().ClearAllFaces();
  
  const s32 NumBlankFramesBetweenPeople = 10;
  
  Vision::Image img;
  
  // Find all directories of face images, one per person
  std::vector<std::string> peopleDirs;
  Util::FileUtils::ListAllDirectories(dataPath, peopleDirs);
  
  std::set<Vision::FaceID_t> allIDs;
  
  struct TestDirData {
    const char* dirName;
    bool isForTraining;
    std::set<std::string> names;
  };
  
  std::vector<TestDirData> testDirData = {
    {.dirName = "andrew",       .isForTraining = true,    .names = {"andrew"}},
    {.dirName = "kevin",        .isForTraining = true,    .names = {"kevin"}},
    {.dirName = "peter",        .isForTraining = true,    .names = {"peter"}},
    {.dirName = "andrew2",      .isForTraining = false,   .names = {"andrew"}},
    {.dirName = "kevin+peter",  .isForTraining = false,   .names = {"kevin", "peter"}},
  };
  
  std::set<std::string> allNames;
  for(auto & test : testDirData) {
    allNames.insert(test.names.begin(), test.names.end());
  }
  
  Json::Value config;
  config["FaceDetection"]["DetectionMode"] = "video";
  config["FaceRecognition"]["RunMode"] = "synchronous";
  
  Vision::kFaceRecognitionThreshold = 600;
  Vision::kFaceMergeThreshold = 500;
  
  // Since we're processing faster than real time (using saved images)
  Vision::kTimeBetweenInitialFaceEnrollmentUpdates_sec = 0.01;

  
  for(s32 iReload=0; iReload<2; ++iReload)
  {
    faceTracker = new Vision::FaceTracker(cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                          "/config/basestation/vision"),
                                          config);
    
    if(iReload > 0) {
      std::list<Vision::FaceNameAndID> loadedNames;
      Result loadResult = faceTracker->LoadAlbum("testAlbum", loadedNames);
      ASSERT_EQ(loadResult, RESULT_OK);
      ASSERT_EQ(loadedNames.size(), allNames.size());
      
      // All loaded names should be in the all names set
      for(auto & nameAndID : loadedNames) {
        ASSERT_TRUE(allNames.count(nameAndID.name) > 0);
      }
      
      for(auto & test : testDirData)
      {
        test.isForTraining = false;
      }
    }
    
    for(auto & test : testDirData)
    {
      if(test.isForTraining) {
        faceTracker->SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::LookingStraight);
      } else {
        faceTracker->SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::Disabled);
      }
      
      const char* testDir = test.dirName;
      bool isNameSet = !test.isForTraining;
      
      auto testFiles = Util::FileUtils::FilesInDirectory(dataPath+testDir, true, imageFileExtensions);
      ASSERT_FALSE(testFiles.empty());
      
      struct {
        s32 totalFrames = 0;
        s32 facesDetected = 0;
        s32 facesRecognized = 0;
        s32 falsePositives = 0;
        s32 numFaceIDs = 0;
      } stats;
      
      //for(auto & testFile : testFiles)
      const s32 numFiles = std::min(MaxImagesPerDir, (s32)testFiles.size());
      for(s32 iFile=0; iFile < numFiles; ++iFile)
      {
        const std::string& testFile = testFiles[iFile];
        
        Recognize(robot, img, *faceTracker, testFile, "TestImage", true);
        stats.totalFrames++;
        
        // Get the faces observed in the current image
        auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());

        if(observedFaceIDs.size() != test.names.size()) {
          PRINT_NAMED_WARNING("FaceRecogntion.VideoRecognitionAndTracking.WrongNumFacesDetected",
                              "Detected %lu faces instead of %lu. File: %s",
                              observedFaceIDs.size(), test.names.size(), testFile.c_str());
        }
        
        stats.facesDetected += observedFaceIDs.size();
        
        for(auto observedID : observedFaceIDs)
        {
          if(observedID != Vision::UnknownFaceID)
          {
            allIDs.insert(observedID);
          }
          
          if(isNameSet)
          {
            auto observedFace = robot.GetFaceWorld().GetFace(observedID);
            ASSERT_NE(observedFace, nullptr);
            
            const std::string& observedName = observedFace->GetName();
            
            if(test.names.count(observedName) > 0) {
              //PRINT_NAMED_INFO("FaceRecognition.VideoRecognitionAndTracking.RecognizedFace",
              //                 "Correctly found %s", observedName.c_str());
              stats.facesRecognized++;
            } else if(observedID != Vision::UnknownFaceID && allNames.count(observedName) > 0) {
              stats.falsePositives++;
            }
          } else if(observedID >= 0) {
            // Recognized, not just tracked
            stats.facesRecognized++;
            faceTracker->AssignNameToID(observedID, testDir, Vision::UnknownFaceID);
            PRINT_NAMED_INFO("FaceRecogntion.VideoRecognitionAndTracking.AddingName",
                             "Registering '%s' to ID %d", testDir, observedID);
            isNameSet = true;
          }
        } // for each observed face
        
      } // for each testFile
      
      ASSERT_TRUE(isNameSet);
      
      stats.numFaceIDs = (s32)allIDs.size();
      
      const f32 detectionPercent = (f32)(stats.facesDetected*100)/(f32)(stats.totalFrames * test.names.size());
      const f32 recogPercent     = (f32)(stats.facesRecognized*100)/(f32)stats.facesDetected;
      const f32 falsePosPercent  = (f32)(stats.falsePositives*100)/(f32)stats.facesDetected;
      
      PRINT_NAMED_INFO("FaceRecognition.VideoRecognitionAndTracking.Stats",
                       "%s: %d images, %d faces detected (%.1f%%), %d faces recognized (%.1f%%), %d false positives (%.1f%%), %d total IDs assigned",
                       testDir,
                       stats.totalFrames, stats.facesDetected, detectionPercent,
                       stats.facesRecognized, recogPercent,
                       stats.falsePositives, falsePosPercent,
                       stats.numFaceIDs);
      
      EXPECT_GE(detectionPercent, ExpectedDetectionPercent);
      EXPECT_GE(recogPercent,     ExpectedRecognitionPercent);
      EXPECT_LE(stats.numFaceIDs, ExpectedNumFaceIDsLimit);
      EXPECT_LE(falsePosPercent,  ExpectedFalsePositivePercent);
      
      // Show the system blank frames before next video so it stops tracking
      for(s32 iBlank=0; iBlank<NumBlankFramesBetweenPeople; ++iBlank) {
        Recognize(robot, img, *faceTracker, "", "TestImage", true);
        
        // Since we won't detect anything in these blank frames, we could
        // move through them too quickly and cause the tracker to persist
        // across people (??)
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        
        // We should not detect faces in any frames past the "lost" count
        if(iBlank >= 2) {
          auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
          EXPECT_TRUE(observedFaceIDs.empty());
        }
      }
    } // for each testDir
   
    Result saveResult = faceTracker->SaveAlbum("testAlbum");
    ASSERT_EQ(saveResult, RESULT_OK);
    
    delete faceTracker;
    
  } // for reload

  // Clean up after ourselves
  Util::FileUtils::RemoveDirectory("testAlbum");
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
} // FaceRecognition.VideoRecognitionAndTracking

