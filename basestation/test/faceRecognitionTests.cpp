#include "util/helpers/includeGTest.h"

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

// Helper for getting all image files (jpeg and png) from a given directory
static void GetImageFiles(const std::string& inDir, std::vector<std::string>& filenames)
{
  static const std::regex imageFilenameMatcher("[^.].*\\.(jpg|jpeg|png|JPG|JPEG|PNG)\0");
  
  DIR* dir = opendir(inDir.c_str());
  ASSERT_TRUE(nullptr != dir);
  
  dirent *entry;
  while((entry = readdir(dir)) != nullptr) {
    if(entry->d_type == DT_REG && std::regex_match(entry->d_name, imageFilenameMatcher)) {
      filenames.push_back(inDir + entry->d_name);
    }
  }
}

// Helper for getting subdirectories from a given directory
static void GetSubDirs(const std::string& parentDir, std::vector<std::string>& subDirs)
{
  DIR* dir = opendir(parentDir.c_str());
  ASSERT_TRUE(nullptr != dir);
  
  dirent* entry = nullptr;
  while ( (entry = readdir(dir)) != nullptr) {
    if(entry->d_type == DT_DIR && entry->d_name[0] != '.') {
      PRINT_NAMED_INFO("FaceRecognition.PairwiseComparison.AddingPersonDir", "Person: %s", entry->d_name);
      subDirs.push_back(parentDir + entry->d_name + "/");
    }
  }
  closedir(dir);
  ASSERT_FALSE(subDirs.empty());
}

// Helper lambda to pass an image file into the vision component and
// then update FaceWorld with any resulting detections
static void Recognize(Robot& robot, Vision::Image& img, Vision::FaceTracker& faceTracker,
                      const std::string& filename, const char *dispName, bool shouldBeOwner)
{
  Result lastResult = RESULT_OK;
  static TimeStamp_t timestamp = 0;
  
  timestamp += 30; // fake time
  img.SetTimestamp(timestamp);
  
  lastResult = img.Load(filename);
  ASSERT_TRUE(RESULT_OK == lastResult);
  
# if SHOW_IMAGES
  Vision::ImageRGB dispImg(img);
# endif
  
  lastResult = faceTracker.Update(img);
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
    dispImg.DrawText(face.GetRect().GetTopLeft(), std::to_string(face.GetID()), drawColor, 1.5f);
#   endif
  }
  
# if SHOW_IMAGES
  dispImg.Display(dispName);
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
  GetSubDirs(dataPath, peopleDirs);

  struct {
    s32 falsePosCount = 0;
    s32 falseNegCount = 0;
    s32 totalCount = 0;
  } stats;
  
  for(auto & ownerDir : peopleDirs)
  {
    std::vector<std::string> ownerTrainFiles;
    
    GetImageFiles(ownerDir, ownerTrainFiles);
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
        
        std::vector<std::string> testFiles;
        GetImageFiles(testDir, testFiles);
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


TEST(FaceRecognition, MultiPersonPairwiseComparison)
{
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
  GetSubDirs(dataPath, peopleDirs);
  
  struct {
    s32 falsePosCount = 0;
    s32 falseNegCount = 0;
    s32 totalCount = 0;
  } stats;
  
  Vision::FaceTracker faceTracker(dataPlatform->pathToResource(Util::Data::Scope::Resources,
                                                               "/config/basestation/vision"),
                                  Vision::FaceTracker::DetectionMode::SingleImage);

  
  // Enroll the first image of each person
  faceTracker.EnableNewFaceEnrollment(true);
  s32 ctr=0;
  std::map<std::string, Vision::TrackedFace::ID_t> expectedIDs;
  for(auto & ownerDir : peopleDirs)
  {
    std::vector<std::string> ownerTrainFiles;
    
    GetImageFiles(ownerDir, ownerTrainFiles);
    ASSERT_FALSE(ownerTrainFiles.empty());
    
    // Use this one image as the training image for this owner
    Recognize(robot, img, faceTracker, ownerTrainFiles[0], ("User" + std::to_string(ctr++)).c_str(), true);
      
    auto knownFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
    ASSERT_EQ(knownFaceIDs.size(), 1);
    if(knownFaceIDs.empty()) {
      // TODO: Remove this and fail if knownFaceIDs is empty
      PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoOwnerFaceDetected",
                          "File: %s", ownerTrainFiles[0].c_str());
      continue;
    }
    
    expectedIDs[ownerDir] = knownFaceIDs.begin()->second;
  }
  
  auto knownIDs = robot.GetFaceWorld().GetKnownFaceIDs();
  ASSERT_EQ(knownIDs.size(), peopleDirs.size());
  for(s32 i=0; i<knownIDs.size(); ++i) {
    ASSERT_EQ(knownIDs[i], i+1);
  }
  
  // Check to make sure every image in every dir is recognized correctly and
  // don't enroll any faces as new
  faceTracker.EnableNewFaceEnrollment(false);
  for(auto & testDir : peopleDirs)
  {
    const Vision::TrackedFace::ID_t expectedID = expectedIDs[testDir];
    
    std::vector<std::string> testFiles;
    GetImageFiles(testDir, testFiles);
    ASSERT_FALSE(testFiles.empty());
    
    for(auto & testFile : testFiles)
    {
      Recognize(robot, img, faceTracker, testFile, "TestImage", true);
      ++stats.totalCount;
      
      // Get the faces observed in the current image
      auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
      ASSERT_EQ(observedFaceIDs.size(), 1);
      if(observedFaceIDs.empty()) {
        // TODO: Remove this and fail if observedFaceIDs is empty
        PRINT_NAMED_WARNING("FaceRecognition.PairwiseComparison.NoTestFaceDetected",
                            "File: %s", testFile.c_str());
        ++stats.falseNegCount;
        continue;
      }
      
      const Vision::TrackedFace::ID_t observedID = observedFaceIDs.begin()->second;
      
      if(observedID != expectedID) {
        if(observedID == Vision::TrackedFace::UnknownFace) {
        // Did not match any person
          ++stats.falseNegCount;
        } else {
          // Matched wrong person
          ++stats.falsePosCount;
        }
      }      
    } // for each testFile
  } // for each testDir

  // Display stats:
  const f32 falsePosRate = (f32)(100*stats.falsePosCount)/(f32)stats.totalCount;
  const f32 falseNegRate = (f32)(100*stats.falseNegCount)/(f32)stats.totalCount;
  PRINT_NAMED_INFO("FaceRecognition.MultiPersonPairwiseComparison.Stats",
                   "%lu People, Total comparisons = %d, FalsePos = %d (%.1f%%), FalseNeg = %d (%.1f%%)",
                   peopleDirs.size(),
                   stats.totalCount,
                   stats.falsePosCount,
                   falsePosRate,
                   stats.falseNegCount,
                   falseNegRate);
  
  EXPECT_LT(falsePosRate, 1.f); // Less than 1% false positives
  EXPECT_LT(falseNegRate, 10.f); // Less than 10% false negatives
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
} // FaceRecognition.MultiPersonPairwiseComparison

