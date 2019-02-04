/**
 * File: faceEvalDetection.cpp
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2018
 **/


#include "coretech/vision/tools/faceEval/faceEvalDetection.h"
#include "coretech/vision/tools/faceEval/simpleArgParser.h"

#include "coretech/vision/engine/compressedImage.h"
#include "coretech/vision/engine/faceTracker.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"

#define LOG_CHANNEL "FaceEval"

namespace Anki {
namespace Vision {

FaceEvalDetection::FaceEvalDetection(const FileList& faceFiles,
                                     const FileList& backgroundFiles,
                                     const SimpleArgParser& argParser)
: IFaceEval(argParser)
{
  for(const auto& file : faceFiles)
  {
    _testFiles.emplace_back(TestFile{file, 1});
  }
  for(const auto& file : backgroundFiles)
  {
    _testFiles.emplace_back(TestFile{file, 0});
  }
  
  argParser.GetArg("--numTimesToShowImage", _params.numTimesToShowImage);
  argParser.GetArg("--verbose", _params.verbose);
  argParser.GetArg("--resizeFactor", _params.resizeFactor);
  argParser.GetArg("--useStillMode", _params.useStillMode);
  argParser.GetArg("--delayCount", _params.delayCount);
  argParser.GetArg("--detectionThreshold", _params.detectionThreshold);
  
  if(_params.useStillMode)
  {
    _params.numTimesToShowImage = 1;
  }
  
  NativeAnkiUtilConsoleSetValueWithString("DetectionMode", (_params.useStillMode ? "Still" : "Movie"));
  NativeAnkiUtilConsoleSetValueWithString("DelayCount", std::to_string(_params.delayCount).c_str());
  NativeAnkiUtilConsoleSetValueWithString("LostMaxRetry", "0");
  NativeAnkiUtilConsoleSetValueWithString("LostMaxHold", "0");
  NativeAnkiUtilConsoleSetValueWithString("FaceDetectionThreshold", std::to_string(_params.detectionThreshold*1000).c_str());
  
  GetFaceTracker().EnableRecognition(false);
  GetFaceTracker().EnableGazeDetection(true); // to force part detection, since recognition is disabled
}

void FaceEvalDetection::Run()
{
  // Load each image and try to detect a face in it
  Image img;
  FaceTracker& faceTracker = GetFaceTracker();
  
  int numFalsePos = 0;
  int numFalseNeg = 0;
  int totalFacesFound = 0;
  int totalTrueFaces = 0;
  int numPartsFound = 0;
  
  for(const auto& testFile : _testFiles)
  {
    Result result = img.Load(testFile.filename);
    if(RESULT_OK != result)
    {
      LOG_ERROR("FaceEvalDetection.Run.LoadImageFileFailed", "%s", testFile.filename.c_str());
      continue;
    }
    
    totalTrueFaces += testFile.numFaces;
    
    img.Resize(_params.resizeFactor);
    
    const float kCropFactor = 1.f;
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    DebugImageList<CompressedImage> debugImages;
    for(int iShow=0; iShow < _params.numTimesToShowImage; ++iShow)
    {
      faces.clear();
      updatedIDs.clear();
      faceTracker.Update(img, kCropFactor, faces, updatedIDs, debugImages);
    }
    
    if(_params.numTimesToShowImage > 1)
    {
      // Clear the tracker
      faceTracker.AccountForRobotMove();
    }
    
    const int numFacesFound = (int)faces.size();
    totalFacesFound += numFacesFound;
    
    if(_params.verbose)
    {
      LOG_INFO("FaceEvalDetection.Run.FacesFound", "%s: Found %d, Expected %d",
               //Util::FileUtils::GetFileName(testFile.filename, true).c_str(),
               testFile.filename.c_str(),
               numFacesFound, testFile.numFaces);
    }
    
    const int falsePosCount = std::max(0, numFacesFound - testFile.numFaces);
    const int falseNegCount = std::max(0, testFile.numFaces - numFacesFound);
    numFalsePos += falsePosCount;
    numFalseNeg += falseNegCount;
    
    int partsCount = 0;
    for(const auto& face : faces)
    {
      if(face.HasEyes())
      {
        ++partsCount;
      }
    }
    numPartsFound += partsCount;
    
    FailureFlags failureFlags;
    failureFlags.SetBitFlag(FailureType::FalsePositive, falsePosCount>0);
    failureFlags.SetBitFlag(FailureType::FalseNegative, falseNegCount>0);
    failureFlags.SetBitFlag(FailureType::MissingParts,  partsCount!=faces.size());
  
    DrawFaces(img, faces, failureFlags);
  }
  
  const float falsePosRate = (float)numFalsePos / (float)_testFiles.size();
  const float falseNegRate = (float)numFalseNeg / (float)totalTrueFaces;
  LOG_INFO("FaceEvalDetection.Run.FalsePositives", "%d / %zu (%.1f%%)",
           numFalsePos, _testFiles.size(), 100.f*falsePosRate);
  LOG_INFO("FaceEvalDetection.Run.FalseNegatives", "%d / %d (%.1f%%)",
           numFalseNeg, totalTrueFaces, 100.f*falseNegRate);
  
  const float partDetectionRate = (float)numPartsFound / (float)totalFacesFound;
  LOG_INFO("FaceEvalDetection.Run.PartDetection", "Found parts for %d / %d faces (%.1f%%)",
           numPartsFound, totalFacesFound, 100.f*partDetectionRate);
}


} // namespace Vision
} // namespace Anki

