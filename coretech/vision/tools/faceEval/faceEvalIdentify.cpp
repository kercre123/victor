/**
 * File: faceEvalIdentify.cpp
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/tools/faceEval/faceEvalIdentify.h"

#include "coretech/vision/engine/compressedImage.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/tools/faceEval/simpleArgParser.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"

#include <fstream>
#include <list>
#include <map>
#include <random>
#include <set>

#define LOG_CHANNEL "FaceEval"

namespace Anki {
namespace Vision {

namespace {
  const char* const kEnrollListFilename = "enrollList.txt";
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceEvalIdentify::FaceEvalIdentify(const std::list<std::string>& directories,
                                   const SimpleArgParser& argParser)
  : IFaceEval(argParser)
{
  std::copy(directories.begin(), directories.end(), std::back_inserter(_dirList));
  
  NativeAnkiUtilConsoleSetValueWithString("DetectionMode", "Still");
  argParser.GetArg("--resizeFactor", _params.resizeFactor);
  argParser.GetArg("--recognitionThreshold", _params.recognitionThreshold);
  argParser.GetArg("--recognitionThreshold2", _params.recognitionThreshold2);
  argParser.GetArg("--recognitionMargin", _params.recognitionMargin);
  argParser.GetArg("--numRuns", _params.numRuns);
  argParser.GetArg("--maxEnrollees", _params.maxEnrollees);
  argParser.GetArg("--saveImages", _params.saveImages);
  
  //argParser.GetArg("--minEyeDistToEnroll", _params.minEyeDistToEnroll);
  //argParser.GetArg("--maxHeadAngleToEnroll", _params.maxHeadAngleToEnroll);
  
  NativeAnkiUtilConsoleSetValueWithString("FaceRecognitionThreshold",
                                          std::to_string(_params.recognitionThreshold*1000.f).c_str());
  
  // We will manage recognition manually
  GetFaceTracker().EnableRecognition(false);
  GetFaceTracker().EnableGazeDetection(true); // force part extraction
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalIdentify::Run()
{
  if(_params.numRuns > 1 || _params.maxEnrollees > 0)
  {
    for(int iRun=0; iRun<_params.numRuns; ++iRun)
    {
      LOG_INFO("FaceEvalIdentify.Run", "Run %d of %d, %d Enrollees", iRun, _params.numRuns, _params.maxEnrollees);
      ShuffleHelper(_dirList);
      GetFaceTracker().EraseAllFaces();
      
      if(_params.numRuns > 1)
      {
        _outSuffix = "_" + std::to_string(iRun);
        Util::FileUtils::CreateDirectory(Util::FileUtils::FullFilePath({GetOutputDir(), _outSuffix}));
      }
      RunInternal();
    }
  }
  else
  {
    RunInternal();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalIdentify::RunInternal()
{
  float avgNumAddedPerFace = 0.f;
  const char* kImageFileExt = "jpg";
  int faceID=0;
  _testFileList.clear();
  for(const auto& dir : _dirList)
  {
    std::set<std::string> filesToEnroll;
    const bool shouldEnrollDir = (_params.maxEnrollees < 0) || (faceID < _params.maxEnrollees);
    if(shouldEnrollDir && ReadEnrollList(dir, filesToEnroll))
    {
      // Got enrollment files. Use them to create an album entry for this directory,
      // by finding faces in the specified enrollment files and adding them to the
      // face tracker
      const int numAdded = EnrollFace(filesToEnroll, faceID);
      if(numAdded > 0)
      {
        // Add all other image files not on the enroll list to the test set
        auto newFiles = Util::FileUtils::FilesInDirectory(dir, true, kImageFileExt);
        for(const auto& file : newFiles)
        {
          if(filesToEnroll.count(file)==0)
          {
            _testFileList.emplace_back(file, faceID);
          }
        }
        
        ++faceID;
        avgNumAddedPerFace += numAdded;
      }
    }
    else
    {
      auto newFiles = Util::FileUtils::FilesInDirectory(dir, true, kImageFileExt);
      for(const auto& file : newFiles)
      {
        _testFileList.emplace_back(file, -1);
      }
    }
  }
  
  avgNumAddedPerFace /= (float)faceID;
  
  LOG_INFO("FaceEvalIdentify.Run.EnrollmentCompleted",
           "Enrolled %d faces (Avg: %.2f images/person). Found %zu test files",
           faceID, avgNumAddedPerFace, _testFileList.size());
  
  if(faceID > 0)
  {
    TestIdentification();
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceEvalIdentify::ReadEnrollList(const std::string& dir, std::set<std::string>& filesToEnroll)
{
  filesToEnroll.clear();
  const std::string& enrollListFile = Util::FileUtils::FullFilePath({dir, kEnrollListFilename});
  const bool hasEnrollList = Util::FileUtils::FileExists(enrollListFile);
  if(hasEnrollList)
  {
    // Each line is filename in this directory
    std::ifstream file(enrollListFile);
    if (!file)
    {
      LOG_ERROR("EvalFaceIdentity.ReadEnrollList.FileNotFound", "%s", enrollListFile.c_str());
      return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
      const std::string& filename = Util::FileUtils::FullFilePath({dir, line});
      if(Util::FileUtils::FileExists(filename))
      {
        filesToEnroll.insert(filename);
      }
      else
      {
        LOG_WARNING("EvalFaceIdentity.ReadEnrollList.MissingEnrollmentFile", "%s", filename.c_str());
      }
    }
    
    const bool gotFiles = !filesToEnroll.empty();
    return gotFiles;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FaceEvalIdentify::EnrollFace(const std::set<std::string>& enrollFiles, const int faceID)
{
  auto& faceTracker = GetFaceTracker();
  
  int numAdded = 0;
  for(const auto& file : enrollFiles)
  {
    Image img;
    Result result = img.Load(file);
    if(RESULT_OK != result)
    {
      LOG_ERROR("FaceEvalIdentify.EnrollFace.LoadImageFileFailed", "%s", file.c_str());
      continue;
    }
    
    img.Resize(_params.resizeFactor);
    
    const float kCropFactor = 1.f;
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    DebugImageList<CompressedImage> debugImages;
    faceTracker.Update(img, kCropFactor, faces, updatedIDs, debugImages);
    
    if(faces.empty())
    {
      continue;
    }
    
    if(faces.size() > 1)
    {
      LOG_WARNING("FaceEvalIdentify.EnrollFace.UsingFirstFace", "%zu found", faces.size());
    }
    
    const auto& face = faces.front();
    
    if(!face.HasEyes())
    {
      LOG_WARNING("FaceEvalIdentify.EnrollFace.NoEyesFound", "ID:%d File:%s",
                  faceID, file.c_str());
      continue;
    }
    
    // TODO this was to quickly fix a build, there must be a better way
#if ANKI_DEVELOPER_CODE
    result = faceTracker.DevAddFaceToAlbum(img, face, faceID);
    if(RESULT_OK != result)
    {
      LOG_WARNING("FaceEvalIdentify.EnrollFace.CouldNotAddToAlbum", "ID:%d File:%s",
                  faceID, file.c_str());
      continue;
    }
#endif
    
    if(_params.saveImages)
    {
      const std::string filename("enroll" + std::to_string(faceID) + ".jpg");
      img.Save(Util::FileUtils::FullFilePath({GetOutputDir(), _outSuffix, filename}));
    }
    
    ++numAdded;
    
    if(numAdded >= _params.maxEnrollmentsPerPerson)
    {
      LOG_INFO("FaceEvalIdentify.EnrollFace.MaxEnrollments", "FaceID:%d maxed out at %d enrollments",
               faceID, numAdded);
      break;
    }
  }
  
  return numAdded;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalIdentify::TestIdentification()
{
  auto& faceTracker = GetFaceTracker();
  
  s32 numFacesChecked = 0;
  s32 numKnownFacesChecked = 0;
  s32 numFalseNeg = 0;
  s32 numFalsePos = 0; // matched to the wrong face
  s32 numFalseNeg2 = 0;
  s32 numFalsePos2 = 0;
  
  struct TestResult
  {
    float score;
    float score2;
    bool  groundTruth;
  };
  std::list<TestResult> testResults;
  
  for(const auto& testFile : _testFileList)
  {
    const std::string& file = testFile.first;
    
    Image img;
    Result result = img.Load(file);
    if(RESULT_OK != result)
    {
      LOG_ERROR("FaceEvalIdentify.TestIdentification.LoadImageFileFailed", "%s", file.c_str());
      continue;
    }
    
    img.Resize(_params.resizeFactor);
    
    const float kCropFactor = 1.f;
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    DebugImageList<CompressedImage> debugImages;
    faceTracker.Update(img, kCropFactor, faces, updatedIDs, debugImages);
    
    FailureFlags failureFlags;
    const int expectedID = testFile.second;
    for(const auto& face : faces)
    {
      if(!face.HasEyes())
      {
        continue;
      }

      // TODO this was to quickly fix a build, there must be a better way
      std::vector<std::pair<int,float>> matches;
#if ANKI_DEVELOPER_CODE
      const Result findResult = faceTracker.DevFindFaceInAlbum(img, face, 3, matches);
      if(RESULT_OK != findResult || matches.empty())
      {
        LOG_ERROR("FaceEvalIdentify.TestIdentification.FindFaceInAlbumFailed", "%s", file.c_str());
        continue;
      }
#endif
      
      //const float verifyScore = faceTracker.ComputePairwiseMatchScore(matchedID, img, face);
      
      const int matchedID = matches.front().first;
      
      bool scoreAboveThreshold = false;
      const float score = matches.front().second;
      const float score2 = (matches.size() > 1 ? matches[1].second : score);
      const bool aboveHighThreshold = Util::IsFltGT(score, _params.recognitionThreshold);
      const bool aboveLowThresholdWithMargin = (Util::IsFltGT(score, _params.recognitionThreshold2) &&
                                                Util::IsFltGT(score-score2, _params.recognitionMargin));
      if(aboveHighThreshold)
      {
        scoreAboveThreshold = true;
      }
      else if(aboveLowThresholdWithMargin)
      {
        scoreAboveThreshold = true;
      }
      
      testResults.emplace_back(TestResult{score, score2, (matchedID == expectedID)});
      
      std::string saveFilename;
      ++numFacesChecked;
      if(expectedID >= 0)
      {
        ++numKnownFacesChecked;
        if(!aboveHighThreshold)
        {
          ++numFalseNeg;
          if(!aboveLowThresholdWithMargin)
          {
            ++numFalseNeg2;
          }
          
          failureFlags.SetBitFlag(FailureType::FalseNegative, true);
          
          if(_params.saveImages)
          {
            saveFilename = "FalseNegative_" + std::to_string(numFacesChecked) + ".jpg";
          }
        }
      }
      
      if(expectedID != matchedID)
      {
        if(aboveHighThreshold)
        {
          ++numFalsePos;
          failureFlags.SetBitFlag(FailureType::FalsePositive, true);
          
          if(_params.saveImages)
          {
            saveFilename = "FalsePositive_" + std::to_string(numFacesChecked) + ".jpg";
          }
        }
        if(aboveLowThresholdWithMargin)
        {
          ++numFalsePos2;
        }
      }
      
      std::string scoreStr;
      for(const auto& match : matches)
      {
        scoreStr += std::to_string((int)std::round(1000.f*match.second));
        scoreStr += " ";
      }
      const std::string displayStr = ("True:" + std::to_string(expectedID) +
                                      " Found:" + std::to_string(matchedID) +
                                      " Score:" + scoreStr);// +
                                      //" VerifyScore:" + std::to_string((int)std::round(1000.f*verifyScore)));
    
      if(!_outSuffix.empty() && !saveFilename.empty())
      {
        saveFilename = Util::FileUtils::FullFilePath({_outSuffix, saveFilename});
      }
      DrawFaces(img, {face}, failureFlags, displayStr, 0, "Identification", saveFilename);
    }
  }
  
  const float falsePosRate = (float)numFalsePos / (float)numFacesChecked;
  const float falseNegRate = (float)numFalseNeg / (float)numKnownFacesChecked;
  LOG_INFO("FaceEvalIdentify.TestIdentification.FalsePositives",
           "%d / %d (%.1f%%) vs. %d / %d (%.1f%%) without 2nd threshold and margin",
           numFalsePos, numFacesChecked, 100.f*falsePosRate,
           numFalsePos2, numFacesChecked,
           100.f*((float)numFalsePos2/(float)numFacesChecked));
  LOG_INFO("FaceEvalIdentify.TestIdentification.FalseNegatives",
           "%d / %d (%.1f%%) vs. %d / %d (%.1f%%) without 2nd threshold and margin",
           numFalseNeg, numKnownFacesChecked, 100.f*falseNegRate,
           numFalseNeg2, numKnownFacesChecked,
           100.f*((float)numFalseNeg2/(float)numKnownFacesChecked));
  
  // Save results to file
  {
    std::ofstream file;
    const std::string baseName = "identification_results" + _outSuffix + ".txt";
    const std::string& outFile = Util::FileUtils::FullFilePath({GetOutputDir(), _outSuffix, baseName});
    file.open(outFile, std::ios::out | std::ofstream::binary);
    if(file.is_open())
    {
      for(const auto& result : testResults)
      {
        file << result.score << ", " << result.score2 << ", " << result.groundTruth << std::endl;
      }
      file.close();
    }
  }
}
  
} // namespace Vision
} // namespace Anki
