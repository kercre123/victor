#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "anki/common/types.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"

#include <dirent.h>
#include <regex>

#define SHOW_IMAGES 0

// Increase this (e.g. to max int) in order to run longer tests using all the available video frames
// The limit is here to keep the test time shorter for general use.
static const s32 MaxImagesPerDir = 50;

// Increase this to do more exhaustive checking in the single person pairwise test.
// This is the number of images to try as the single "training" image. Set to
// max integer to try using every image as the training image.
static const s32 MaxOwnerTrainings = 2;

// Make sure we detected/recognized a high enough percentage of faces and that
// we didn't add excessive IDs to the album in the process. These are checked
// per user-directory of video frames
static const f32 ExpectedDetectionPercent   = 90.f;
static const f32 ExpectedRecognitionPercent = 85.f;
static const s32 ExpectedNumFaceIDsLimit    = 10;
static const f32 ExpectedFalsePositivePercent = 1.f;

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Cozmo;


static const std::vector<const char*> imageFileExtensions = {"jpg", "jpeg", "png"};

// Helper lambda to pass an image file into the vision component and
// then update FaceWorld with any resulting detections
static void Recognize(Robot& robot, Vision::Image& img, Vision::FaceTracker& faceTracker,
                      const std::string& filename, const char *dispName, bool shouldBeOwner,
                      s32 fakeVideoFrames = 1)
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
  
  // Show the same frame N times to fake face being still in video for a few frames
  for(s32 i=0; i<fakeVideoFrames; ++i) {
    lastResult = faceTracker.Update(img);
  }
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
  for(auto & face : faceTracker.GetFaces()) {
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
    dispImg.DrawPoint(face.GetLeftEyeCenter(), drawColor, 2);
    dispImg.DrawPoint(face.GetRightEyeCenter(), drawColor, 2);
    
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


TEST(FaceRecognition, SinglePersonPairwiseComparison)
{
  Result lastResult = RESULT_OK;

  // TODO: Update or remove this test. It is currently not really valid because it is using Single Image mode.
  return;
  
  
  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.RecognitionNotSupported",
                        "Skipping test because the compiled face tracker does not support recognition");
    return;
  }
  
  Robot robot(1, cozmoContext);
  
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "test/faceRecognitionTests/");
  
  Vision::Image img;
  
  // Find all directories of face images, one per person
  std::vector<std::string> peopleDirs;
  Util::FileUtils::ListAllDirectories(dataPath, peopleDirs);

  struct {
    s32 falsePosCount = 0;
    s32 falseNegCount = 0;
    s32 totalCount = 0;
  } stats;
  
  for(auto & ownerDir : peopleDirs)
  {
    auto ownerTrainFiles = Util::FileUtils::FilesInDirectory(dataPath+ownerDir, true, imageFileExtensions);
    ASSERT_FALSE(ownerTrainFiles.empty());
    
    s32 numOwnerTrainings = 0;
    for(auto & ownerTrainFile : ownerTrainFiles)
    {
      if(numOwnerTrainings >= MaxOwnerTrainings) {
        break;
      }
      
      // Create a new face tracker for each owner train file, to start from
      // scratch
      Vision::FaceTracker faceTracker(cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                      "/config/basestation/vision"),
                                      Vision::FaceTracker::DetectionMode::SingleImage);
      
      // Start fresh with each owner
      robot.GetFaceWorld().ClearAllFaces();
      
      // Use this one image as the training image for this owner
      Recognize(robot, img, faceTracker, ownerTrainFile, "CurrentOwner", true, 10);
    
      auto knownFaceIDs = robot.GetFaceWorld().GetKnownFaceIDs();
      if(knownFaceIDs.empty()) {
        PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoOwnerFaceDetected",
                            "File: %s", ownerTrainFile.c_str());
        continue;
      }
      
      // Set first face as owner
      auto const ownerID = knownFaceIDs[0];
      ASSERT_NE(ownerID, Vision::TrackedFace::UnknownFace);
      ++numOwnerTrainings;
      robot.GetFaceWorld().SetOwnerID(ownerID);
      ASSERT_TRUE(robot.GetFaceWorld().GetOwnerID() == ownerID);
      
      // Check to make sure images in this dir are recognized as the owner
      // and images in every other dir are NOT the owner
      for(auto & testDir : peopleDirs)
      {
        const bool shouldBeOwner = (testDir == ownerDir);
        
        auto testFiles = Util::FileUtils::FilesInDirectory(dataPath+testDir, true, imageFileExtensions);
        ASSERT_FALSE(testFiles.empty());
        
        for(auto & testFile : testFiles)
        {
          Recognize(robot, img, faceTracker, testFile, "TestImage", shouldBeOwner);
          ++stats.totalCount;
          
          // Get the faces observed in the current image
          auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
          if(observedFaceIDs.empty()) {
            PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoTestFaceDetected",
                                "File: %s", testFile.c_str());
            ++stats.falseNegCount;
            continue;
          }
          
          // Make sure one of the observed faces was or was not the owner
          // (depending on whether we are in the owner dir)
          bool ownerFound = false;
          for(auto & observedID : observedFaceIDs)
          {
            if(observedID == ownerID) {
              ownerFound = true;
              break;
            }
          }
          
          if(shouldBeOwner) {
            //EXPECT_TRUE(ownerFound);
            if(!ownerFound) {
              ++stats.falseNegCount;
            }
          } else {
            //EXPECT_FALSE(ownerFound);
            if(ownerFound){
              ++stats.falsePosCount;
            }
          }
          
        } // for each testFile
      } // for each testDir
    } // for each ownerTrainFile
  } // for each ownerDir
  
  // Display stats:
  const f32 falsePosRate = (f32)(100*stats.falsePosCount)/(f32)stats.totalCount;
  const f32 falseNegRate = (f32)(100*stats.falseNegCount)/(f32)stats.totalCount;
  PRINT_NAMED_INFO("FaceRecognition.PairwiseComparison.Stats",
                   "Total comparisons = %d, FalsePos = %d (%.1f%%), FalseNeg = %d (%.1f%%)",
                   stats.totalCount,
                   stats.falsePosCount,
                   falsePosRate,
                   stats.falseNegCount,
                   falseNegRate);
  
  EXPECT_LT(falsePosRate, 1.f); // Less than 1% false positives
  EXPECT_LT(falseNegRate, 10.f); // Less than 10% false negatives
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
} // FaceRecognition.SinglePersonPairwiseComparison


TEST(FaceRecognition, VideoRecognitionAndTracking)
{
  Result lastResult = RESULT_OK;
  
  RobotInterface::MessageHandlerStub  msgHandler;
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
  
  std::set<Vision::TrackedFace::ID_t> allIDs;
  
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
  
  for(s32 iReload=0; iReload<2; ++iReload)
  {
    faceTracker = new Vision::FaceTracker(cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                          "/config/basestation/vision"),
                                          Vision::FaceTracker::DetectionMode::Video);
    
    if(iReload > 0) {
      Result loadResult = faceTracker->LoadAlbum("testAlbum");
      ASSERT_EQ(loadResult, RESULT_OK);
      
      for(auto & test : testDirData)
      {
        test.isForTraining = false;
      }
    }
    
    for(auto & test : testDirData)
    {
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
          if(observedID != Vision::TrackedFace::UnknownFace)
          {
            allIDs.insert(observedID);
          }
          
          if(isNameSet)
          {
            auto observedFace = robot.GetFaceWorld().GetFace(observedID);
            ASSERT_NE(observedFace, nullptr);
            if(test.names.count(observedFace->GetName()) > 0) {
              //PRINT_NAMED_INFO("FaceRecognition.VideoRecognitionAndTracking.RecognizedFace",
              //                 "Correctly found %s", observedFace->GetName().c_str());
              stats.facesRecognized++;
            } else if(observedID != Vision::TrackedFace::UnknownFace && allNames.count(observedFace->GetName()) > 0) {
              stats.falsePositives++;
            }
          } else if(observedID != Vision::TrackedFace::UnknownFace) {
            stats.facesRecognized++;
            faceTracker->AssignNameToID(observedID, testDir);
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

