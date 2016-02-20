#include "util/helpers/includeGTest.h"

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

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki;
using namespace Anki::Cozmo;

TEST(FaceRecognition, TestOwnerID)
{
  Result lastResult = RESULT_OK;
  
  Robot robot(1, cozmoContext);
  
  if(false == Vision::FaceTracker::IsRecognitionSupported()) {
    PRINT_NAMED_WARNING("FaceRecognition.TestOwnerID.RecognitionNotSupported",
                        "Skipping test because the compiled face tracker does not support recognition");
    return;
  }
  
  // Find all directories of face images, one per person
  std::vector<std::string> peopleDirs;
  const std::string dataPath = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources, "test/faceRecognitionTests/");
  DIR* dir = opendir(dataPath.c_str());
  ASSERT_TRUE(nullptr != dir);
  
  dirent* entry = nullptr;
  while ( (entry = readdir(dir)) != nullptr) {
    if(entry->d_type == DT_DIR && entry->d_name[0] != '.') {
      PRINT_NAMED_INFO("FaceRecognition.TestOwnerID.AddingPersonDir", "Person: %s", entry->d_name);
      peopleDirs.push_back(dataPath + entry->d_name + "/");
    }
  }
  closedir(dir);
  ASSERT_FALSE(peopleDirs.empty());
  
  // Helper for getting all image files from a given directory
  auto GetImageFiles = [](const std::string& inDir, std::vector<std::string>& filenames)
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
  };
  
  
  Vision::Image img;
  
  // Helper lambda to pass an image file into the vision component and
  // then update FaceWorld with any resulting detections
  auto Recognize = [&robot, &img](Vision::FaceTracker& faceTracker, const std::string& filename, const char *dispName) {
    Result lastResult = RESULT_OK;
    static TimeStamp_t timestamp = 0;

    timestamp += 30; // fake time
    img.SetTimestamp(timestamp);

    lastResult = img.Load(filename);
    ASSERT_TRUE(RESULT_OK == lastResult);
    
#   if SHOW_IMAGES
    Vision::ImageRGB dispImg(img);
#   endif

    lastResult = faceTracker.Update(img);
    ASSERT_TRUE(RESULT_OK == lastResult);
    
    for(auto & face : faceTracker.GetFaces()) {
      // Do some bogus pose parent setting to keep face world from complaining
      Pose3d headPose = face.GetHeadPose();
      headPose.SetParent(robot.GetWorldOrigin());
      face.SetHeadPose(headPose);
      
      lastResult = robot.GetFaceWorld().AddOrUpdateFace(face);
      ASSERT_TRUE(RESULT_OK == lastResult);
      
#     if SHOW_IMAGES
      dispImg.DrawRect(face.GetRect(), NamedColors::RED, 2);
      dispImg.DrawText(face.GetRect().GetTopLeft(), std::to_string(face.GetID()), NamedColors::RED, 1.5f);
#     endif
    }
    
#   if SHOW_IMAGES
    dispImg.Display(dispName);
#   endif
    
    lastResult = robot.GetFaceWorld().Update();
    ASSERT_TRUE(RESULT_OK == lastResult);
  };

  
  for(auto & ownerDir : peopleDirs)
  {
    std::vector<std::string> ownerTrainFiles;
    
    GetImageFiles(ownerDir, ownerTrainFiles);
    ASSERT_FALSE(ownerTrainFiles.empty());
    
    for(auto & ownerTrainFile : ownerTrainFiles)
    {
      // Create a new face tracker for each owner train file, to start from
      // scratch
      Vision::FaceTracker faceTracker(cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                   "/config/basestation/vision"),
                                      Vision::FaceTracker::DetectionMode::SingleImage);

      // Use this one image as the training image for this owner
      Recognize(faceTracker, ownerTrainFile, "CurrentOwner");
      
      auto knownFaceIDs = robot.GetFaceWorld().GetKnownFaceIDs();
      EXPECT_TRUE(knownFaceIDs.size() == 1);
      if(knownFaceIDs.empty()) {
        // TODO: Remove this and fail if knownFaceIDs is empty
        PRINT_NAMED_WARNING("FaceRecognition.TestOwnerID.NoOwnerFaceDetected",
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
          Recognize(faceTracker, testFile, "TestImage");
          
          // Get the faces observed in the current image
          auto observedFaceIDs = robot.GetFaceWorld().GetKnownFaceIDsObservedSince(img.GetTimestamp());
          EXPECT_TRUE(observedFaceIDs.size() == 1);
          if(observedFaceIDs.empty()) {
            // TODO: Remove this and fail if observedFaceIDs is empty
            PRINT_NAMED_WARNING("FaceRecognition.TestOwnerID.NoTestFaceDetected",
                                "File: %s", testFile.c_str());
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
            EXPECT_TRUE(ownerFound);
          } else {
            EXPECT_FALSE(ownerFound);
          }
          
        } // for each testFile
      } // for each testDir
    } // for each ownerTrainFile
  } // for each ownerDir
  
  ASSERT_TRUE(RESULT_OK == lastResult);
  
} // FaceRecognition.TestOwnerID
