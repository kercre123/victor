#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"
#include "util/fileUtils/fileUtils.h"
#include "util/random/randomGenerator.h"

#include "json/json.h"

#include "engine/cozmoContext.h"

#include "anki/common/types.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"

#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/profiler.h"

#include "opencv2/highgui/highgui.hpp"

#include <unistd.h>
#include <thread>

using namespace Anki;

const char *gWindowName = "RecognizeFaces";

namespace Anki {
namespace Vision{
  extern s32 kFaceRecognitionThreshold;
  extern f32 kTimeBetweenFaceEnrollmentUpdates_sec;
  extern s32 kMinFaceDetectConfidenceToEnroll;
  extern bool kGetEnrollmentTimeFromImageTimestamp;
}
}

// For forcing tracker to stop between people
void ShowBlanks(Vision::FaceTracker& faceTracker, s32 kNumBlankFramesBetweenPeople,
                s32 numRows, s32 numCols, bool doDisplay)
{
  static Vision::Image blankImg;
  if(blankImg.GetNumRows() != numRows || blankImg.GetNumCols() != numCols) {
    blankImg.Allocate(numRows, numCols);
    blankImg.FillWith(0);
  }
  
  // Show the system blank frames before next video so it stops tracking
  for(s32 iBlank=0; iBlank<kNumBlankFramesBetweenPeople; ++iBlank) {
    std::list<Vision::TrackedFace> faces;
    std::list<Vision::UpdatedFaceID> updatedIDs;
    Result result = faceTracker.Update(blankImg, faces, updatedIDs);
    
    DEV_ASSERT(RESULT_OK == result, "RecognizeFaces.FaceTrackerBlankUpdateFailed");
    
    // Since we won't detect anything in these blank frames, we could
    // move through them too quickly and cause the tracker to persist
    // across people (??)
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
  }
  
  if(doDisplay)
  {
    static Vision::ImageRGB blankImgDisp;
    if(blankImgDisp.GetNumRows() != blankImg.GetNumRows() ||
       blankImgDisp.GetNumCols() != blankImg.GetNumCols())
    {
      blankImgDisp.Allocate(blankImg.GetNumRows(), blankImg.GetNumCols());
      blankImgDisp.FillWith(Vision::PixelRGB(255,0,0));
    }
    blankImgDisp.Display(gWindowName);
  }
  
} // ShowBlanks()


void ReorderImageFileList(std::vector<std::string>& imageFiles)
{
  std::map<s32, std::string> numToFile;
  
  for(auto & file : imageFiles)
  {
    // Extract the filenumber b/w the last understore and last period
    auto underscore = file.rfind("_");
    auto period     = file.rfind(".");
    
    assert(underscore != std::string::npos);
    assert(period != std::string::npos);
    
    std::string numStr = file.substr(underscore+1, period-underscore-1);
    const s32 N = std::stoi(numStr);
    
    numToFile[N] = file;
  }
  
  imageFiles.clear();
  for(auto & orderedFile : numToFile)
  {
    imageFiles.push_back(orderedFile.second);
  }
  
} // ReorderImageFileList()

void onMouse(int event, int x, int y, int, void* userData)
{
  if(event > 0) {
    std::function<void()>* fcn = (std::function<void()>*)userData;
    (*fcn)();
  }
}

void LiveEnroll(Vision::FaceTracker& faceTracker, const std::string& name, const std::string& albumName)
{
  // Set up capture device
  PRINT_NAMED_INFO("LiveEnroll.WaitingForCaptureDevice", "");
  cv::VideoCapture vidCap(0);
  PRINT_NAMED_INFO("LiveEnroll.CaptureDeviceOpened", "");
  
  vidCap.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
  vidCap.set(CV_CAP_PROP_FRAME_WIDTH, 320);
  vidCap.set(CV_CAP_PROP_FPS, 15);
  
  Vision::ImageRGB frame;
  bool enrollmentComplete = false;
  
  // Get existing library
  std::list<Vision::LoadedKnownFace> loadedFaces;
  Result loadResult = faceTracker.LoadAlbum(albumName, loadedFaces);
  if(RESULT_OK != loadResult) {
    PRINT_NAMED_WARNING("LiveEnroll.LoadAlbumFail", "Could not load album named '%s'",
                        albumName.c_str());
  }
  
  for(auto const& nameAndID : loadedFaces)
  {
    if(nameAndID.name == name) {
      PRINT_NAMED_WARNING("LiveEnroll.ReplacingUser",
                          "User '%s' already exists. Will re-enroll.", name.c_str());
      faceTracker.EraseFace(nameAndID.faceID);
      break;
    }
  }
  
  // Set up the various enrollment modes and ordering
  auto enrollPose = Vision::FaceEnrollmentPose::LookingStraight;
  faceTracker.SetFaceEnrollmentMode(enrollPose);
  const TimeStamp_t kMinTimePerEnrollMode_ms = 1000;
  TimeStamp_t lastModeChangeTime_ms = 0;
  
  const std::map<Vision::FaceEnrollmentPose, std::string> instructions{
    {Vision::FaceEnrollmentPose::LookingStraight,       "LOOK STRAIGHT AT COSMO"},
    {Vision::FaceEnrollmentPose::LookingStraightClose,  "SLOWLY MOVE CLOSER"},
    {Vision::FaceEnrollmentPose::LookingStraightFar,    "SLOWLY MOVE AWAY"},
    {Vision::FaceEnrollmentPose::LookingLeft,           "SLOWLY TURN TO THE RIGHT"},
    {Vision::FaceEnrollmentPose::LookingRight,          "SLOWLY TURN TO THE LEFT"},
    {Vision::FaceEnrollmentPose::LookingUp,             "SLOWLY TILT HEAD UP"},
    {Vision::FaceEnrollmentPose::LookingDown,           "SLOWLY TILT HEAD DOWN"},
    {Vision::FaceEnrollmentPose::Disabled,              "DONE!"},
  };
  
  // Enrollment using left/right/up
//  std::vector<Vision::FaceEnrollmentPose> sequence{
//    Vision::FaceEnrollmentPose::LookingStraight,
//    Vision::FaceEnrollmentPose::LookingLeft,
//    Vision::FaceEnrollmentPose::LookingRight,
//    Vision::FaceEnrollmentPose::LookingStraight,
//    Vision::FaceEnrollmentPose::LookingUp,
//    Vision::FaceEnrollmentPose::LookingStraight,
//    Vision::FaceEnrollmentPose::LookingStraightClose,
//    Vision::FaceEnrollmentPose::LookingStraightFar,
//    Vision::FaceEnrollmentPose::None
//  };

  // Enrollment using just straight-ahead at various distances
  std::vector<Vision::FaceEnrollmentPose> sequence{
    Vision::FaceEnrollmentPose::LookingStraight,
    Vision::FaceEnrollmentPose::LookingStraightClose,
    Vision::FaceEnrollmentPose::LookingStraightFar,
    Vision::FaceEnrollmentPose::Disabled
  };
  
  DEV_ASSERT(sequence.size() < 10, "Enroll.EnrollmentSequenceTooLong");

  // Helpers for speaking the instructions and waiting until we're ready
  auto sayInstructions = [&instructions, &enrollPose, &lastModeChangeTime_ms, &frame]() {
    system(("say -v Vicki " + instructions.at(enrollPose)).c_str());
    lastModeChangeTime_ms = frame.GetTimestamp();
  };
  
  bool ready = false;
  
  std::function<void()> MakeReady = [&ready,&sayInstructions]() {
    if(!ready) {
      ready = true;
      std::thread sayThread(sayInstructions);
      sayThread.detach();
    }
  };
  
  cv::namedWindow("LiveEnroll");
  cv::setMouseCallback("LiveEnroll", onMouse, &MakeReady);

  
  Vision::TrackedFace face;
  while(!enrollmentComplete)
  {
    const bool success = vidCap.read(frame.get_CvMat_());
    if(success) {
      if(frame.IsEmpty()) {
        std::cout << "Empty frame grabbed" << std::endl;
      } else {
        
        cv::cvtColor(frame.get_CvMat_(), frame.get_CvMat_(), CV_BGR2RGB);
        
        auto timestamp = std::chrono::system_clock::now();
        frame.SetTimestamp((TimeStamp_t)std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
        
        if(ready)
        {
          std::list<Vision::TrackedFace> faces;
          std::list<Vision::UpdatedFaceID> updatedIDs;
          Result result = faceTracker.Update(frame.ToGray(), faces, updatedIDs);
          DEV_ASSERT(RESULT_OK == result, "Enroll.FaceTrackerUpdateFail");
          
          if(!faces.empty())
          {
            face = faces.front();
            
            if(face.GetName().empty())
            {
              auto newEnrollPose = sequence[face.GetNumEnrollments()/2];
              if(newEnrollPose != enrollPose)
              {
                faceTracker.SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::Disabled);
                
                if(frame.GetTimestamp() - lastModeChangeTime_ms > kMinTimePerEnrollMode_ms)
                {
                  // Time to switch enrollment modes and say the instructions
                  enrollPose = newEnrollPose;
                  std::thread sayThread(sayInstructions);
                  sayThread.detach();
                  
                  faceTracker.SetFaceEnrollmentMode(enrollPose);
                  
                  if(enrollPose == Vision::FaceEnrollmentPose::Disabled) {
                    enrollmentComplete = true;
                  }
                }
              }
              
              char temp[128];
              snprintf(temp, 127, "ID=%d D=%.1f R=%.1f, P=%.1f, Y=%.1f",
                       face.GetID(),
                       face.GetIntraEyeDistance(), face.GetHeadRoll().getDegrees(),
                       face.GetHeadPitch().getDegrees(), face.GetHeadYaw().getDegrees());
              
              frame.DrawText({1, 20}, temp, NamedColors::RED, 0.4f);
            } else {
              frame.DrawText({1,frame.GetNumRows()-1}, "Recognized as " + face.GetName(), NamedColors::RED, 0.75f);
            }
            
            frame.DrawRect(face.GetRect(), NamedColors::RED, 2);
          }
        
          frame.DrawText({1, frame.GetNumRows()-20}, instructions.at(enrollPose), NamedColors::RED, 0.5f);
          frame.DrawText({1, frame.GetNumRows()-1},
                         "Mode: " + std::to_string((u8)enrollPose) +
                         " N: " + std::to_string(face.GetNumEnrollments()),
                         NamedColors::RED, 0.75f);
        }
        frame.Display("LiveEnroll", 30);
      }
    } else {
      std::cout << "Frame grab failed" << std::endl;
    }
    
  } // while(!enrollmentComplete)
  
  vidCap.release();
  
  frame.CloseDisplayWindow("LiveEnroll");
  
  // Assign the name to the ID and save
  faceTracker.AssignNameToID(face.GetID(), name, Vision::UnknownFaceID);
  faceTracker.SaveAlbum(albumName);
  
} // LiveEnroll()


void LiveRecognize(Vision::FaceTracker& faceTracker, const std::string& albumName)
{
  // Load album of faces to recognize
  std::list<Vision::LoadedKnownFace> loadedFaces;
  Result loadResult = faceTracker.LoadAlbum(albumName, loadedFaces);
  DEV_ASSERT(loadResult == RESULT_OK, "Recognize.LoadAlbumFail");
  
  // Set up capture device
  cv::VideoCapture vidCap(0);
  vidCap.set(CV_CAP_PROP_FRAME_HEIGHT, 240);
  vidCap.set(CV_CAP_PROP_FRAME_WIDTH, 320);
  vidCap.set(CV_CAP_PROP_FPS, 15);
  
  Util::RandomGenerator rng;
  const s32 kMinSayNameSpacing_ms = 1000;
  const s32 kMaxSayNameSpacing_ms = 5000;
  s32 nextSayNameTime = 0;
  
  Vision::ImageRGB frame;
  faceTracker.SetFaceEnrollmentMode(Vision::FaceEnrollmentPose::Disabled);
  Vision::TrackedFace face;
  
  bool done = false;
  std::function<void()> SetDone = [&done]() {
    done = true;
  };
  
  cv::namedWindow("LiveRecognize");
  cv::setMouseCallback("LiveRecognize", onMouse, &SetDone);
  
  auto sayName = [&face]() {
    system(("say -v Vicki Hi " + face.GetName() + "!").c_str());
  };
  
  const auto startTime = std::chrono::system_clock::now();
  
  while(!done)
  {
    const bool success = vidCap.read(frame.get_CvMat_());
    if(success) {
      if(frame.IsEmpty()) {
        std::cout << "Empty frame grabbed" << std::endl;
      } else {
        cv::cvtColor(frame.get_CvMat_(), frame.get_CvMat_(), CV_BGR2RGB);
        //std::chrono::duration<std::chrono::milliseconds>
        auto timestamp = std::chrono::system_clock::now() - startTime;
        frame.SetTimestamp((TimeStamp_t)std::chrono::duration_cast<std::chrono::milliseconds>(timestamp).count());
        
        std::list<Vision::TrackedFace> faces;
        std::list<Vision::UpdatedFaceID> updatedIDs;
        Result result = faceTracker.Update(frame.ToGray(), faces, updatedIDs);
        DEV_ASSERT(RESULT_OK == result, "Enroll.FaceTrackerUpdateFail");
        
        if(!faces.empty())
        {
          face = faces.front();
          frame.DrawText({1, frame.GetNumRows()-1}, face.GetName(), NamedColors::RED, 0.7f);
          frame.DrawRect(face.GetRect(), NamedColors::RED, 2);
          
          if(!face.GetName().empty() && frame.GetTimestamp() > nextSayNameTime) {
            std::thread sayNameThread(sayName);
            sayNameThread.detach();
            
            nextSayNameTime = frame.GetTimestamp() + rng.RandIntInRange(kMinSayNameSpacing_ms, kMaxSayNameSpacing_ms);
          }
        }
        
        frame.Display("LiveRecognize", 30);
      }
    } else {
      std::cout << "Frame grab failed" << std::endl;
    }
  }
  
  frame.CloseDisplayWindow("LiveRecognize");
  
  vidCap.release();
} // LiveRecognize()


void CannedRun(const std::string& mode, Vision::FaceTracker& faceTracker, const std::string& cwdPath, const std::string& inputPath)
{
  using namespace Anki::Util;
  
  static const std::vector<const char*> imageFileExtensions = {"jpg", "jpeg", "png"};
  
  // TODO: Get these parameters from command line
  const s32 kNumEnrollmentsNeeded = 10;
  const bool kDoDisplay = true;
  const s32 kNumBlankFramesBetweenPeople = 5;
  const s32 kTestBlankingFrequency = 15; // blank out every this-many frames during test to restart tracker
  const std::string albumName(FileUtils::FullFilePath({cwdPath, "trainedAlbum"}));
  const s32 kFramePeriod_ms = 65;
  const s32 saveFaceCropSize = 0;

  Vision::Image img;
  Vision::ImageRGB dispImg;
  Vision::Profiler timer;
  
  std::vector<std::string> groupDirs;
  FileUtils::ListAllDirectories(inputPath, groupDirs);
  
  std::map<std::string, Vision::FaceID_t> nameToFaceID;
  
  // Adjust console vars
  //Vision::kFaceRecognitionThreshold = 500;
  Vision::kGetEnrollmentTimeFromImageTimestamp = true;
  
  TimeStamp_t fakeTime_ms = 0;

  if(mode == "Train")
  {
    //
    // Training phase
    //
    for(s32 iGroup=0; iGroup<groupDirs.size(); ++iGroup)
    {
      std::vector<std::string> peopleDirs;
      FileUtils::ListAllDirectories(FileUtils::FullFilePath({inputPath, groupDirs[iGroup]}) , peopleDirs);
      
      for(s32 iPerson=0; iPerson<peopleDirs.size(); ++iPerson)
      {
        auto imageFiles = FileUtils::FilesInDirectory(FileUtils::FullFilePath({inputPath, groupDirs[iGroup], peopleDirs[iPerson]}),
                                                      false, imageFileExtensions);
        
        ReorderImageFileList(imageFiles);
       
        // Feed in files until we've filled up the numEnrollments for this face
        Vision::TrackedFace face;
        s32 iFile = 0;
        do {
          if(kDoDisplay) {
            timer.Tic("ProcFrame");
          }
          
          const std::string fullImageFile(FileUtils::FullFilePath({inputPath, groupDirs[iGroup], peopleDirs[iPerson], imageFiles[iFile]}));
          img.Load(fullImageFile);
          if(img.IsEmpty()) {
            PRINT_NAMED_WARNING("RecognizeFaces.ImageLoadFailure", "Image %d: %s", iFile, fullImageFile.c_str());
            ++iFile;
            continue;
          }
          
          img.SetTimestamp(fakeTime_ms);
          fakeTime_ms += kFramePeriod_ms;
          
          std::list<Vision::TrackedFace> faces;
          std::list<Vision::UpdatedFaceID> updatedIDs;
          Result result = faceTracker.Update(img, faces, updatedIDs);
          
          DEV_ASSERT(RESULT_OK == result, "RecognizeFaces.FaceTrackerUpdateFailed");
          
          if(faces.size() > 0) {
            if(faces.size() > 1) {
              PRINT_NAMED_WARNING("RecognizeFaces.MultipleFaces", "Found %lu faces in %s. Using first.",
                                  faces.size(), imageFiles[iFile].c_str());
            }
            
            // Wait till we get a recognized face back (not just a tracked one)
            if(faces.front().GetID() > 0)
            {
              if(Vision::UnknownFaceID == face.GetID()) {
                // This is the first recognized face we've found for this person
                face = faces.front();
              } else {
                if(faces.front().GetID() != face.GetID()) {
                  PRINT_NAMED_WARNING("RecognizeFaces.FaceChangedID",
                                      "Detected face with ID %d does not match initial ID %d",
                                      faces.front().GetID(), face.GetID());
                }
                face = faces.front();
              }
            }
          }
          
          if(kDoDisplay)
          {
            dispImg = img;
            if(!faces.empty()) {
              auto faceIter = faces.begin();
              dispImg.DrawRect(faceIter->GetRect(), NamedColors::RED, 2);
              dispImg.DrawText(faceIter->GetRect().GetTopLeft(), "N="+std::to_string(faceIter->GetNumEnrollments()),
                               NamedColors::RED, .75f);
              ++faceIter;
              while(faceIter != faces.end()) {
                dispImg.DrawRect(faceIter->GetRect(), NamedColors::CYAN, 2);
                ++faceIter;
              }
            }
            dispImg.DrawText({0,dispImg.GetNumRows()}, imageFiles[iFile], NamedColors::RED, .5f);
            const s32 framePeriod_ms = 1000 / 15; // 15fps
            auto t_ms = framePeriod_ms - std::round(timer.Toc("ProcFrame"));
            dispImg.Display(gWindowName, t_ms <= 0 ? 1 : t_ms);
          }
          
        } while(++iFile < imageFiles.size() && face.GetNumEnrollments() < kNumEnrollmentsNeeded);
        
        if(face.GetNumEnrollments() >= kNumEnrollmentsNeeded) {
          faceTracker.AssignNameToID(face.GetID(), peopleDirs[iPerson], Vision::UnknownFaceID);
          PRINT_NAMED_INFO("RecognizeFaces.EnrollmentCompleted", "Enrolled ID %d with name %s",
                           face.GetID(), peopleDirs[iPerson].c_str());
          nameToFaceID[peopleDirs[iPerson]] = face.GetID();
          
          // Save after each new person completes
          if(!albumName.empty()) {
            Result saveResult = faceTracker.SaveAlbum(albumName);
            if(saveResult != RESULT_OK) {
              PRINT_NAMED_WARNING("RecognizeFaces.SaveFailed",
                                  "Tried to save to: %s", albumName.c_str());
            }
          }
        }
        
        ShowBlanks(faceTracker, kNumBlankFramesBetweenPeople, img.GetNumRows(), img.GetNumCols(), kDoDisplay);
        fakeTime_ms += kNumBlankFramesBetweenPeople*kFramePeriod_ms;
        
      } // for each person
      
    } // for each group
    
  } else if(mode == "Test") {
    
    //
    // Testing Phase
    //
    struct Stats {
      s32 numFalseNegatives = 0;
      s32 numFalsePositives = 0;
      s32 numFacesDetected  = 0;
    } stats;
    
    struct Detection {
      Vision::FaceID_t detectedID;
      Vision::FaceID_t correctID;
      f32  score;
    };
    
    std::vector<Detection> detections;
    
    for(s32 iGroup=0; iGroup<groupDirs.size(); ++iGroup)
    {
      std::vector<std::string> peopleDirs;
      FileUtils::ListAllDirectories(FileUtils::FullFilePath({inputPath, groupDirs[iGroup]}) , peopleDirs);
      
      for(s32 iPerson=0; iPerson<peopleDirs.size(); ++iPerson)
      {
        auto imageFiles = FileUtils::FilesInDirectory(FileUtils::FullFilePath({inputPath, groupDirs[iGroup], peopleDirs[iPerson]}),
                                                      true, imageFileExtensions);
        
        ReorderImageFileList(imageFiles);
        
        std::string saveDir;
        if(saveFaceCropSize > 0) {
          saveDir = FileUtils::FullFilePath({inputPath, groupDirs[iGroup], peopleDirs[iPerson], "faceCrop"});
          FileUtils::CreateDirectory(saveDir);
        }
        
        // Feed in files until we've filled up the numEnrollments for this face
        for(s32 iFile = 0; iFile < imageFiles.size(); ++iFile)
        {
          img.Load(imageFiles[iFile]);
          
          img.SetTimestamp(fakeTime_ms);
          fakeTime_ms += kFramePeriod_ms;
          
          if(iFile % kTestBlankingFrequency == 0) {
            ShowBlanks(faceTracker, kNumBlankFramesBetweenPeople, img.GetNumRows(), img.GetNumCols(), kDoDisplay);
            fakeTime_ms += kNumBlankFramesBetweenPeople*kFramePeriod_ms;
          }
          
          std::list<Vision::TrackedFace> faces;
          std::list<Vision::UpdatedFaceID> updatedIDs;
          Result result = faceTracker.Update(img, faces, updatedIDs);
          
          DEV_ASSERT(RESULT_OK == result, "RecognizeFaces.FaceTrackerUpdateFailed");
          
          Vision::TrackedFace face;
          const ColorRGBA* drawColor = &NamedColors::GREEN;
          if(faces.size() > 0) {
            ++stats.numFacesDetected;
            
            if(faces.size() > 1) {
              PRINT_NAMED_WARNING("RecognizeFaces.MultipleFaces", "Found %lu faces in %s. Using first.",
                                  faces.size(), imageFiles[iFile].c_str());
            }
            
            face = faces.front();
            
            if(saveFaceCropSize > 0)
            {
              Rectangle<f32> rectF32 = face.GetRect();
              Rectangle<s32> roiRect(rectF32.GetX(), rectF32.GetY(), rectF32.GetWidth(), rectF32.GetHeight());
              Vision::Image faceROI = img.GetROI(roiRect);
              faceROI.Resize(saveFaceCropSize, saveFaceCropSize);
              cv::imwrite(FileUtils::FullFilePath({saveDir, imageFiles[iFile]}), faceROI.get_CvMat_());
            }

            if(Vision::UnknownFaceID == face.GetID() || face.GetName().empty()) {
              // Did not recognize the face: false negative
              ++stats.numFalseNegatives;
              drawColor = &NamedColors::ORANGE;
            }
            else if(face.GetName() != peopleDirs[iPerson]) {
              // Wrong name: false positive
              ++stats.numFalsePositives;
              drawColor = &NamedColors::RED;
            }
            
            auto nameToFaceIter = nameToFaceID.find(face.GetName());
            
            detections.emplace_back(Detection{
              .score = face.GetScore(),
              .detectedID = face.GetID(),
              .correctID  = (nameToFaceIter == nameToFaceID.end() ?
                             Vision::UnknownFaceID :
                             nameToFaceIter->second)
            });
            
          } // if faces.size() > 0
          
          if(kDoDisplay)
          {
            dispImg = img;
            if(!faces.empty()) {
              auto faceIter = faces.begin();
              dispImg.DrawRect(faceIter->GetRect(), *drawColor, 2);
              dispImg.DrawText({0, dispImg.GetNumRows()-1},
                               faceIter->GetName() + " Score=" + std::to_string(faceIter->GetScore()), *drawColor,
                               .75f);
              ++faceIter;
              while(faceIter != faces.end()) {
                dispImg.DrawRect(faceIter->GetRect(), NamedColors::CYAN, 2);
                ++faceIter;
              }
            }
            dispImg.Display(gWindowName);
          }
          
        } // for each file
        
        ShowBlanks(faceTracker, kNumBlankFramesBetweenPeople, img.GetNumRows(), img.GetNumCols(), kDoDisplay);
        fakeTime_ms += kNumBlankFramesBetweenPeople*kFramePeriod_ms;
        
      } // for each person
    } // for each group
    
    PRINT_NAMED_INFO("RecognizeFaces.Stats",
                     "Detected %d faces. "
                     "%d not recognized (FNR=%.1f%%). "
                     "%d recognized wrong (FPR=%.1f%%",
                     stats.numFacesDetected, stats.numFalseNegatives,
                     (f32)stats.numFalseNegatives/(f32)stats.numFacesDetected * 100.f,
                     stats.numFalsePositives,
                     (f32)stats.numFalsePositives/(f32)stats.numFacesDetected * 100.f);
    
    // Write out the detections to a file
    std::stringstream fileBody;
    for(auto const& detection : detections)
    {
      fileBody << detection.detectedID << " " << detection.correctID << " " << detection.score << std::endl;
    }
    FileUtils::WriteFile(FileUtils::FullFilePath({inputPath, "detectionResults.txt"}), fileBody.str());
    
  } else {
    PRINT_NAMED_ERROR("RecognizeFaces.CannedData.UnknownMode", "%s", mode.c_str());
  }
  
} // CannedRun()

int main(int argc, char* argv[])
{
  using namespace Anki::Util;
  
  if(argc < 2 || (argc > 1 && std::string(argv[1]) == "help")) {
    printf("\n");
    printf("%s [cmd] [option]\n\n", argv[0]);
    printf("  Valid Commands:\n");
    printf("\n");
    printf("   help:          Display this message.\n");
    printf("\n");
    printf("   LiveEnroll:    Enroll a new person for 'live' recognition with USB camera.\n");
    printf("                  Use [option] to specify the name for the enrollee.\n");
    printf("                  Will overwrite existing user of same name, if any.\n");
    printf("                  Click in window to start.\n");
    printf("\n");
    printf("   LiveRecognize: Open USB camera and recognize enrolled faces in each frame.\n");
    printf("                  Click in window to stop.\n");
    printf("\n");
    printf("   CannedTrain:   Enroll faces from saved images from Cozmo's camera.\n");
    printf("                  Use [option] to specify the root path for the data.\n");
    printf("\n");
    printf("   CannedTest:    Complement to CannedTrain for testing the enrolled album.\n");
    printf("                  Use [option] to specify the root path for the data.\n");
    printf("\n");
    exit(1);
  }
  
  std::string runMode("LiveRecognize"); // Default
  if(argc > 1) {
    runMode = argv[1];
  }
  
  //
  // Common Setup
  //
  PrintfLoggerProvider* loggerProvider = new PrintfLoggerProvider();
  loggerProvider->SetMinLogLevel(ILoggerProvider::LOG_LEVEL_DEBUG);
  gLoggerProvider = loggerProvider;
  
  char temp[1256];
  getcwd(temp, 1255);
  const std::string cwdPath(temp);
  
  PRINT_NAMED_INFO("CozmoTests.main","cwdPath %s", cwdPath.c_str());
  PRINT_NAMED_INFO("CozmoTests.main","executable name %s", argv[0]);
  
  // NOTE: Copy/paste from engine unit test setup
  // still troubleshooting different run time environments,
  // need to find a way to detect where the resources folder is located on disk.
  // currently this is relative to the executable.
  // Another option is to pass it in through the environment variables.
  std::string resourcePath = FileUtils::FullFilePath({cwdPath, "resources"});
  std::string filesPath    = FileUtils::FullFilePath({cwdPath, "files"});
  std::string cachePath    = FileUtils::FullFilePath({cwdPath, "temp"});
  std::string externalPath = FileUtils::FullFilePath({cwdPath, "temp"});
  Util::Data::DataPlatform dataPlatform(filesPath, cachePath, externalPath, resourcePath);
  Vector::CozmoContext cozmoContext(&dataPlatform, nullptr);
  
  Json::Value config;
  config["FaceDetection"]["DetectionMode"] = "video";
  config["FaceRecognition"]["RunMode"] = "synchronous";
  
  const std::string dataPath(cozmoContext.GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                             "/config/engine/vision"));
  Vision::FaceTracker faceTracker(dataPath, config);
  
  // Default input path for "canned" modes
  std::string inputPath("/Users/andrew/Dropbox (Anki, Inc)/FaceVideos");
  
  // Run specified mode
  if(runMode == "LiveEnroll") {
    std::string albumName("LiveAlbum");
    std::string enrollName("Person");
    if(argc > 2) {
      enrollName = argv[2];
    }
    
    LiveEnroll(faceTracker, enrollName, albumName);
  }
  else if(runMode == "LiveRecognize") {
    std::string albumName("LiveAlbum");
    LiveRecognize(faceTracker, albumName);
  }
  else if(runMode == "CannedTest") {
    if(argc > 2) {
      inputPath = argv[2];
    }
    CannedRun("Test", faceTracker, cwdPath, inputPath);
    
  } else if(runMode == "CannedTrain") {
    if(argc > 2) {
      inputPath = argv[2];
    }
    CannedRun("Train", faceTracker, cwdPath, inputPath);
    
  } else {
    PRINT_NAMED_ERROR("RecognizeFaces.UnknownRunMode", "%s", runMode.c_str());
    exit(1);
  }
  
  return 0;
} // main()
