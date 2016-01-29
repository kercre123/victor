#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "anki/common/types.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "anki/cozmo/basestation/faceWorld.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotInterface/messageHandler.h"
#include "anki/cozmo/basestation/robotInterface/messageHandlerStub.h"

#include <dirent.h>
#include <regex>

#define SHOW_IMAGES 1

extern Anki::Util::Data::DataPlatform* dataPlatform;

using namespace Anki;
using namespace Anki::Cozmo;


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
  
  // Show the same frame N times to fake face being still in video for a few frames
  const s32 fakeVideoFrames = 1; // min == 1!
  for(s32 i=0; i<fakeVideoFrames; ++i) {
    lastResult = faceTracker.Update(img);
  }
  
  EXPECT_TRUE(RESULT_OK == lastResult);
  
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
    
    std::string label = std::to_string(face.GetID());
    if(face.IsBeingTracked()) {
      label += "*";
    }
    dispImg.DrawText(face.GetRect().GetTopLeft(), label, drawColor, 1.5f);
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
  // DEBUG!
  return;
  
  Result lastResult = RESULT_OK;
  
  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);
  
  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.RecognitionNotSupported",
                        "Skipping test because the compiled face tracker does not support recognition");
    return;
  }
  
  const std::string dataPath = dataPlatform->pathToResource(Util::Data::Scope::Resources, "test/faceRecognitionTests/");
  
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
    
    for(auto & ownerTrainFile : ownerTrainFiles)
    {
      // Create a new face tracker for each owner train file, to start from
      // scratch
      Vision::FaceTracker faceTracker(dataPlatform->pathToResource(Util::Data::Scope::Resources,
                                                                   "/config/basestation/vision"),
                                      Vision::FaceTracker::DetectionMode::SingleImage);
      
      // Start fresh with each owner
      robot.GetFaceWorld().ClearAllFaces();
      
      // Use this one image as the training image for this owner
      Recognize(robot, img, faceTracker, ownerTrainFile, "CurrentOwner", true);
    
      auto knownFaceIDs = robot.GetFaceWorld().GetKnownFaceIDs();
      EXPECT_EQ(knownFaceIDs.size(), 1);
      if(knownFaceIDs.empty()) {
        // TODO: Remove this and fail if knownFaceIDs is empty
        PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoOwnerFaceDetected",
                            "File: %s", ownerTrainFile.c_str());
        continue;
      }
      
      // Set first face as owner
      auto const ownerID = knownFaceIDs[0];
      robot.GetFaceWorld().SetOwnerID(ownerID);
      ASSERT_TRUE(robot.GetFaceWorld().GetOwnerID() == ownerID);
      
      // Check to make sure every image in this dir is recognized as the owner
      // and every image in every other dir is NOT the owner
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
          EXPECT_EQ(observedFaceIDs.size(), 1);
          if(observedFaceIDs.empty()) {
            // TODO: Remove this and fail if observedFaceIDs is empty
            PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoTestFaceDetected",
                                "File: %s", testFile.c_str());
            ++stats.falseNegCount;
            continue;
          }
          
          // Make sure one of the observed faces was or was not the owner
          // (depending on whether we are in the owner dir)
          bool ownerFound = false;
          for(auto & observation : observedFaceIDs)
          {
            if(observation.second == ownerID) {
              ownerFound = true;
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


TEST(FaceRecognition, SinglePersonVideoRecognitionAndTracking)
{
  Result lastResult = RESULT_OK;
  
  RobotInterface::MessageHandlerStub  msgHandler;
  Robot robot(1, &msgHandler, nullptr, dataPlatform);
  
  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_WARNING("FaceRecognition.SinglePersonVideoRecognitionAndTracking.RecognitionNotSupported",
                        "Skipping test because the compiled face tracker does not support recognition");
    return;
  }
  
  const std::string dataPath = dataPlatform->pathToResource(Util::Data::Scope::Resources, "test/faceRecVideoTests/");
  
  // All-new tracker and face world for each person for this test
  Vision::FaceTracker faceTracker(dataPlatform->pathToResource(Util::Data::Scope::Resources,
                                                               "/config/basestation/vision"),
                                  Vision::FaceTracker::DetectionMode::Video);
  robot.GetFaceWorld().ClearAllFaces();
  
  const s32 NumBlankFramesBetweenPeople = 15;
  
  Vision::Image img;
  
  // Find all directories of face images, one per person
  std::vector<std::string> peopleDirs;
  Util::FileUtils::ListAllDirectories(dataPath, peopleDirs);
  
  std::set<Vision::TrackedFace::ID_t> allIDs;
  
  // Check to make sure every image in every dir is recognized correctly and
  // don't enroll any faces as new
  for(auto & testDir : {"andrew", "kevin", "peter", "andrew2", "kevin+peter"})
  {
    auto testFiles = Util::FileUtils::FilesInDirectory(dataPath+testDir, true, imageFileExtensions);
    ASSERT_FALSE(testFiles.empty());
    
    struct {
      s32 totalFrames = 0;
      s32 facesDetected = 0;
      s32 facesRecognized = 0;
      s32 numFaceIDs = 0;
    } stats;
    
    for(auto & testFile : testFiles)
    {
      Recognize(robot, img, faceTracker, testFile, "TestImage", true);
      stats.totalFrames++;
      
      // Get the faces observed in the current image
      auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
      EXPECT_EQ(observedFaceIDs.size(), 1);
      if(observedFaceIDs.empty()) {
        // TODO: Remove this and fail if observedFaceIDs is empty
        PRINT_NAMED_WARNING("FaceRecognition.SinglePersonVideoRecognitionAndTracking.NoTestFaceDetected",
                            "File: %s", testFile.c_str());
      } else {
        stats.facesDetected++;
        
        const Vision::TrackedFace::ID_t observedID = observedFaceIDs.begin()->second;
        
        if(observedFaceIDs.size() > 1) {
          PRINT_NAMED_WARNING("FaceRecogntion.SinglePersonVideoRecognitionAndTracking.MultipleFacesDetected",
                              "Detected %lu faces instead of one. Using first.", observedFaceIDs.size());
        }
        
        if(observedID != Vision::TrackedFace::UnknownFace)
        {
          stats.facesRecognized++;
          allIDs.insert(observedID);
        }
      }

    } // for each testFile
    
    stats.numFaceIDs = (s32)allIDs.size();
    
    PRINT_NAMED_INFO("FaceRecognition.SinglePersonVideoRecognitionAndTracking.Stats",
                     "%s: %d images, %d faces detected (%.1f%%), %d faces recognized (%.1f%%), %d total IDs assigned",
                     testDir,
                     stats.totalFrames, stats.facesDetected,
                     (f32)(stats.facesDetected*100)/(f32)stats.totalFrames,
                     stats.facesRecognized,
                     (f32)(stats.facesRecognized*100)/(f32)stats.facesDetected,
                     stats.numFaceIDs);

    // Show the system blank frames before next video so it stops tracking
    for(s32 iBlank=0; iBlank<NumBlankFramesBetweenPeople; ++iBlank) {
      Recognize(robot, img, faceTracker, "", "TestImage", true);
      auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
      EXPECT_TRUE(observedFaceIDs.empty());
    }
  } // for each testDir

  ASSERT_TRUE(RESULT_OK == lastResult);
  
} // FaceRecognition.SinglePersonVideoRecognitionAndTracking

