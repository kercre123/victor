/**
 * File: faceEvalConfusion.cpp
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: See header.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/vision/tools/faceEval/faceEvalConfusion.h"

#include "coretech/vision/engine/compressedImage.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/tools/faceEval/simpleArgParser.h"

#include "util/console/consoleSystem.h"
#include "util/fileUtils/fileUtils.h"

#include <fstream>
#include <iomanip>
#include <map>
#include <random>
#include <set>

#define LOG_CHANNEL "FaceEval"

namespace Anki {
namespace Vision {
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FaceEvalConfusion::FaceEvalConfusion(const FileList& files,
                                     const SimpleArgParser& argParser)
: IFaceEval(argParser)
, _fileList(files)
{
  NativeAnkiUtilConsoleSetValueWithString("DetectionMode", "Still");
  argParser.GetArg("--resizeFactor", _params.resizeFactor);
  argParser.GetArg("--recognitionThreshold", _params.recognitionThreshold);
  argParser.GetArg("--minEyeDistToEnroll", _params.minEyeDistToEnroll);
  argParser.GetArg("--maxHeadAngleToEnroll", _params.maxHeadAngleToEnroll);
  
  if(!argParser.GetArg("--maxEnrollees", _params.maxEnrollees))
  {
    _params.maxEnrollees = (int)_fileList.size();
  }
  
  // We will manage recognition manually
  GetFaceTracker().EnableRecognition(false);
  GetFaceTracker().EnableGazeDetection(true); // force part extraction
  
  if(_params.maxEnrollees > 0)
  {
    std::random_device randomDevice;
    std::mt19937       randomEngine(randomDevice());
    std::shuffle(_fileList.begin(), _fileList.end(), randomEngine);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static std::string GetIdentier(const std::string fullpathname)
{
  const auto personIndex = fullpathname.find("person");
  if(personIndex != std::string::npos)
  {
    return fullpathname.substr(personIndex, 8);
  }
  
  const std::string filename = Util::FileUtils::GetFileName(fullpathname);
  
  if(filename.substr(0,5) == "2018-")
  {
    return filename.substr(0,32);
  }
  else
  {
    return filename.substr(0,8);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalConfusion::DisplayHelper(FaceID_t faceID, const EnrolledFace& enrolledFace, float score, const char* windowName, int pauseTime) const
{
  ImageRGB img;
  img.Load(enrolledFace.file);
  img.Resize(0.5f);
  DrawFaceOnImage(enrolledFace.face, img);
  img.Resize(0.5f);
  std::stringstream dispStr;
  dispStr << faceID << ": " << GetIdentier(enrolledFace.file);
  if(score >= 0.f)
  {
    dispStr << "[" << std::fixed << std::setprecision(2) << score << " vs. " << _params.recognitionThreshold << "]";
  }
#if ANKI_DEVELOPER_CODE
  img.DrawText({1, img.GetNumRows()-1}, dispStr.str(), NamedColors::RED, 0.4f);
#endif
  img.Display(windowName, pauseTime);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalConfusion::ShowTiledImage(const std::map<int, EnrolledFace>& enrolledFaces) const
{
#if ANKI_DEVELOPER_CODE
  const int kTileWidth = 128;
  const int kTileHeight = 72;
  ImageRGB tiledImg(720/2, 1280/2);
  
  std::map<std::string, std::vector<int>> identityToIDs;
  for(const auto& enrolledFace : enrolledFaces)
  {
    identityToIDs[enrolledFace.second.identity].emplace_back(enrolledFace.first);
  }
  
  for(const auto& identityToID : identityToIDs)
  {
    tiledImg.FillWith(0);
    int row=0, col=0;
    for(const int ID : identityToID.second)
    {
      const EnrolledFace& enrolledFace = enrolledFaces.at(ID);
      ImageRGB img;
      img.Load(enrolledFace.file);
      
      Rectangle<s32> roiRect(col*kTileWidth, row*kTileHeight, kTileWidth, kTileHeight);
      ++col;
      if(col >= tiledImg.GetNumCols()/kTileWidth)
      {
        col = 0;
        ++row;
      }
      ImageRGB roi = tiledImg.GetROI(roiRect);
      
      img.Resize(roi);
      
      tiledImg.DrawRect(roiRect, (IsEnrollable(enrolledFace.face) ? NamedColors::GREEN : NamedColors::RED), 3);
    }
    
    tiledImg.Display(identityToID.first.c_str(), 5);
  }
#endif
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FaceEvalConfusion::IsEnrollable(const TrackedFace& face) const
{
  if((face.GetHeadPitch().getAbsoluteVal() > _params.maxHeadAngleToEnroll) ||
     (face.GetHeadYaw().getAbsoluteVal() > _params.maxHeadAngleToEnroll))
  {
    return false;
  }
  
  Point2f leftCen;
  Point2f rightCen;
  const bool haveEyes = face.GetEyeCenters(leftCen, rightCen);
  DEV_ASSERT(haveEyes, "FaceEvalConfusion.IsEnrollable.ExpectingEyes");
  const float eyeDist = ComputeDistanceBetween(leftCen, rightCen);
  
  if(eyeDist < _params.minEyeDistToEnroll)
  {
    return false;
  }
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FaceEvalConfusion::Run()
{
  FaceTracker& faceTracker = GetFaceTracker();
  
  // Go through all faces and enroll them one by one as separate album entries
  //UniqueFaces uniqueFaces;
  
  std::map<int, EnrolledFace> faceIDtoEnrolledFace;
  int faceID = 0;
  
  for(const auto& file : _fileList)
  {
    Image img;
    Result result = img.Load(file);
    if(RESULT_OK != result)
    {
      LOG_ERROR("FaceEvalConfusion.Run.LoadImageFileFailed", "%s", file.c_str());
      continue;
    }
    
    img.Resize(_params.resizeFactor);
    
    const float kCropFactor = 1.f;
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    DebugImageList<CompressedImage> debugImages;
    faceTracker.Update(img, kCropFactor, faces, updatedIDs, debugImages);
    
    const std::string& idString = GetIdentier(file);
    
    for(const auto& face : faces)
    {
      if(_params.maxEnrollees>0 && !IsEnrollable(face))
      {
        continue;
      }
      
      if(!face.HasEyes())
      {
        LOG_WARNING("FaceEvalConfusion.Run.NoEyesFound", "Identifier:%s File:%s",
                    idString.c_str(), file.c_str());
      }
      else
      {
        // TODO this was to quickly fix a build, there must be a better way
#if ANKI_DEVELOPER_CODE
        const Result addResult = faceTracker.DevAddFaceToAlbum(img, face, faceID);
        if(RESULT_OK != addResult)
        {
          LOG_WARNING("FaceEvalConfusion.Run.CouldNotAddToAlbum", "Identifier:%s File:%s",
                      idString.c_str(), file.c_str());
          continue;
        }
#endif
        
        auto result = faceIDtoEnrolledFace.emplace(faceID, EnrolledFace{
          .identity = idString,
          .file = file,
          .face = face
        });
        
        if(!result.second)
        {
          LOG_ERROR("FaceEvalConfusion.Run.DuplicateEntry", "ID:%d Identifier:%s",
                    faceID, idString.c_str());
          continue;
        }

        // Face got added, increment ID
        ++faceID;
      }
      
      if(faceID >= _params.maxEnrollees)
      {
        break;
      }
    }
    
    if(faceID >= _params.maxEnrollees)
    {
      break;
    }
  }
  
  if(DisplayMode::All == GetDisplayMode())
  {
    ShowTiledImage(faceIDtoEnrolledFace);
  }
  
  LOG_INFO("FaceEvalConfusion.Run.AddedFaceIDs", "Unique Faces: %zu", faceIDtoEnrolledFace.size());
  
  int numFalsePos = 0;
  int numFalseNeg = 0;
  int totalIntraPersonComparisons = 0;
  int totalInterPersonComparisons = 0;
  std::vector<std::pair<float, bool>> results;
  for(auto iter1 = faceIDtoEnrolledFace.begin(); iter1 != faceIDtoEnrolledFace.end(); ++iter1)
  {
    if(!IsEnrollable(iter1->second.face))
    {
      continue;
    }
    
    const int enrollID1 = iter1->first;
    const std::string& identity1 = iter1->second.identity;
    
    if(DisplayMode::Off != GetDisplayMode())
    {
      DisplayHelper(enrollID1, iter1->second, -1.f, "Reference");
    }
    
    for(auto iter2 = iter1; iter2 != faceIDtoEnrolledFace.end(); ++iter2)
    {
      const int enrollID2 = iter2->first;
      const std::string& identity2 = iter2->second.identity;

      // TODO this was to quickly fix a build, there must be a better way
      float score = 0.f;
#if ANKI_DEVELOPER_CODE
      score = faceTracker.DevComputePairwiseMatchScore(enrollID1, enrollID2);
#endif
      const bool isSamePerson = (identity1 == identity2);
      
      results.emplace_back(score, isSamePerson);
      
      if(isSamePerson)
      {
        ++totalIntraPersonComparisons;
        if(score < _params.recognitionThreshold)
        {
          ++numFalseNeg;
          if(DisplayMode::All == GetDisplayMode() || DisplayMode::FalseNegatives == GetDisplayMode())
          {
            DisplayHelper(enrollID2, iter2->second, score, "FalseNeg", 0);
          }
        }
      }
      else
      {
        ++totalInterPersonComparisons;
        if(score >= _params.recognitionThreshold)
        {
          ++numFalsePos;
          if(DisplayMode::All == GetDisplayMode() || DisplayMode::FalsePositives == GetDisplayMode())
          {
            DisplayHelper(enrollID2, iter2->second, score, "FalsePos", 0);
          }
        }
      }
    }
  }
  
  const float falsePosRate = (float)numFalsePos / (float)totalInterPersonComparisons;
  const float falseNegRate = (float)numFalseNeg / (float)totalIntraPersonComparisons;
  LOG_INFO("FaceEvalDetection.Run.FalsePositives", "%d / %d (%.1f%%)",
           numFalsePos, totalInterPersonComparisons, 100.f*falsePosRate);
  LOG_INFO("FaceEvalDetection.Run.FalseNegatives", "%d / %d (%.1f%%)",
           numFalseNeg, totalIntraPersonComparisons, 100.f*falseNegRate);
  
  // Save results to file
  {
    std::ofstream file;
    const std::string& outFile = Util::FileUtils::FullFilePath({GetOutputDir(), "confusion_results.txt"});
    file.open(outFile, std::ios::out | std::ofstream::binary);
    if(file.is_open())
    {
      for(const auto& result : results)
      {
        file << result.first << ", " << result.second << std::endl;
      }
      file.close();
    }
  }
}
  
} // namespace Vision
} // namespace Anki
