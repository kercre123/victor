/**
 * File: faceAnimationManager.cpp
 *
 * Author: Andrew Stein
 * Date:   7/7/2015
 *
 * Description: Implements container for managing available animations for the robot's face display.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/cozmo/basestation/faceAnimationManager.h"
#include "anki/vision/basestation/image.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/basestation/utils/data/dataScope.h"
#include "anki/common/basestation/array2d_impl.h"
#include "anki/cozmo/basestation/keyframe.h"
#include "anki/cozmo/robot/faceDisplayDecode.h"
#include "clad/types/animationKeyFrames.h"
#include "util/dispatchWorker/dispatchWorker.h"
#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"
#include <algorithm>

#include <opencv2/highgui.hpp>

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

namespace Anki {
namespace Cozmo {

  FaceAnimationManager* FaceAnimationManager::_singletonInstance = nullptr;
  const std::string FaceAnimationManager::ProceduralAnimName("_PROCEDURAL_");
  
  u8 FaceAnimationManager::_firstScanLine = 0;
  
  FaceAnimationManager::FaceAnimationManager()
  {
    _availableAnimations[ProceduralAnimName];
  }
  
  void FaceAnimationManager::removeInstance()
  {
    // check if the instance has been created yet
    if(nullptr != _singletonInstance) {
      delete _singletonInstance;
      _singletonInstance = nullptr;
    }
  }
  
  static void AddScanlinesHelper(const Vision::Image& img, std::pair<std::vector<u8>,std::vector<u8>>& rleFrames)
  {
    // Compress twice: once for each scanline version
    Vision::Image imgWithScanlines;
    img.CopyTo(imgWithScanlines);
    for(s32 iScanline=0; iScanline<img.GetNumRows(); iScanline+=2)
    {
      memset(imgWithScanlines.GetRow(iScanline), 0, img.GetNumCols()*sizeof(u8));
    }
    FaceAnimationManager::CompressRLE(imgWithScanlines, rleFrames.first);
    
    img.CopyTo(imgWithScanlines);
    for(s32 iScanline=1; iScanline<img.GetNumRows(); iScanline+=2)
    {
      memset(imgWithScanlines.GetRow(iScanline), 0, img.GetNumCols()*sizeof(u8));
    }
    FaceAnimationManager::CompressRLE(imgWithScanlines, rleFrames.second);
  }
  
  // Read the animations in a dir
  void FaceAnimationManager::ReadFaceAnimationDir(const Util::Data::DataPlatform* dataPlatform, bool fromCache)
  {
    if (dataPlatform == nullptr) { return; }
    Util::Data::Scope resourceScope;
    if (fromCache) {
        resourceScope = Util::Data::Scope::Cache;
    } else {
        resourceScope = Util::Data::Scope::Resources;
    }
    const std::string animationFolder = dataPlatform->pathToResource(resourceScope, "assets/faceAnimations/");
    
    // Get the list of all the directory names
    std::vector<std::string> faceAnimDirNames;
    Util::FileUtils::ListAllDirectories(animationFolder, faceAnimDirNames);
    
    // Set up the worker that will process all the image frame folders
    using MyDispatchWorker = Util::DispatchWorker<3, const std::string&>;
    MyDispatchWorker::FunctionType workerFunction = std::bind(&FaceAnimationManager::LoadAnimationImageFrames, this, animationFolder, std::placeholders::_1);
    MyDispatchWorker worker(workerFunction);
    
    // Go through the list of directories, removing disallowed names, updating timestamps, and removing ones that don't need to be loaded
    auto nameIter = faceAnimDirNames.begin();
    while (nameIter != faceAnimDirNames.end())
    {
      const std::string& animName = *nameIter;
      
      // Remove the disallowed name from the list if it's there
      if (animName == ProceduralAnimName)
      {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir.ReservedName",
                          "'_PROCEDURAL_' is a reserved face animation name. Ignoring.");
        nameIter = faceAnimDirNames.erase(nameIter);
        continue;
      }
      
      std::string fullDirName = Util::FileUtils::FullFilePath({animationFolder, animName});
      struct stat attrib{0};
      int result = stat(fullDirName.c_str(), &attrib);
      if (result == -1) {
        PRINT_NAMED_WARNING("FaceAnimationManager.ReadFaceAnimationDir",
                            "could not get mtime for %s", fullDirName.c_str());
        nameIter = faceAnimDirNames.erase(nameIter);
        continue;
      }
      
      auto mapIt = _availableAnimations.find(animName);
#ifdef __APPLE__ // TODO: COZMO-1057
      time_t tmpSeconds = attrib.st_mtimespec.tv_sec; //This maps to __darwin_time_t
#else
      time_t tmpSeconds = attrib.st_mtime;
#endif
      
      if (mapIt == _availableAnimations.end()) {
        _availableAnimations[animName].lastLoadedTime = tmpSeconds;
      } else if (fromCache) {
        // It should probably be the default behavior to clear the
        // "rleFrames" vector when "loadAnimDir" is true, right?
        _availableAnimations[animName].rleFrames.clear();
        _availableAnimations[animName].lastLoadedTime = tmpSeconds;
      } else {
        if (mapIt->second.lastLoadedTime < tmpSeconds) {
          mapIt->second.lastLoadedTime = tmpSeconds;
        } else {
          // If we already have this anim loaded and its timestamp isn't old, we don't need to reload it
          nameIter = faceAnimDirNames.erase(nameIter);
          continue;
        }
      }
      
      // Now we can start looking at the next name, this one is ok to load
      worker.PushJob(animName);
      ++nameIter;
    }
    
    // Go through and load the anims from our list
    worker.Process();
    
  } // ReadFaceAnimationDir()
  
  
  void FaceAnimationManager::LoadAnimationImageFrames(const std::string& animationFolder, const std::string& animName)
  {
    const std::string fullDirName = Util::FileUtils::FullFilePath({animationFolder, animName});
    std::vector<std::string> fileNames = Util::FileUtils::FilesInDirectory(fullDirName);
    
    // Even though files *might* be sorted alphabetically by the readdir call inside FilesInDirectory,
    // we can't rely on it so do it ourselves
    std::sort(fileNames.begin(), fileNames.end());
    
    for (auto fileIter = fileNames.begin(); fileIter != fileNames.end(); ++fileIter)
    {
      const std::string& filename = *fileIter;
      size_t underscorePos = filename.find_last_of("_");
      size_t dotPos = filename.find_last_of(".");
      if(dotPos == std::string::npos) {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Could not find '.' in frame filename %s",
                          filename.c_str());
        return;
      } else if(underscorePos == std::string::npos) {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Could not find '_' in frame filename %s",
                          filename.c_str());
        return;
      } else if(dotPos <= underscorePos+1) {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Unexpected relative positions for '.' and '_' in frame filename %s",
                          filename.c_str());
        return;
      }
      
      const std::string digitStr(filename.substr(underscorePos+1,
                                                 (dotPos-underscorePos-1)));
      
      s32 frameNum = 0;
      try {
        frameNum = std::stoi(digitStr);
      } catch (std::invalid_argument&) {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Could not get frame number from substring '%s' "
                          "of filename '%s'.",
                          digitStr.c_str(), filename.c_str());
        return;
      }
      
      if(frameNum < 0) {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Found negative frame number (%d) for filename '%s'.",
                          frameNum, filename.c_str());
        return;
      }
      
      AvailableAnim& anim = _availableAnimations[animName];
      
      // Add empty frames if there's a gap
      if(frameNum > 1) {
        
        s32 emptyFramesAdded = 0;
        while(anim.rleFrames.size() < frameNum-1) {
          anim.rleFrames.push_back({});
          ++emptyFramesAdded;
        }
        
        /*
         if(emptyFramesAdded > 0) {
         PRINT_NAMED_DEBUG("FaceAnimationManager.ReadFaceAnimationDir.InsertEmptyFrames",
         "Inserted %d empty frames before frame %d in animation %s.",
         emptyFramesAdded, frameNum, animName.c_str());
         }
         */
      }
      
      // Read the image
      const std::string fullFilename = Util::FileUtils::FullFilePath({fullDirName, filename});
      Vision::Image img;
      Result loadResult = img.Load(fullFilename);
      
      if(loadResult != RESULT_OK ||
         img.GetNumRows() != IMAGE_HEIGHT ||
         img.GetNumCols() != IMAGE_WIDTH)
      {
        PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                          "Image in %s is %dx%d instead of %dx%d.",
                          fullFilename.c_str(),
                          img.GetNumCols(), img.GetNumRows(),
                          IMAGE_WIDTH, IMAGE_HEIGHT);
        continue;
      }
      
      // Binarize
      img.Threshold(128);
      
      // DEBUG
      //cv::imshow("FaceAnimImage", img);
      //cv::waitKey(30);
      
      // Compress twice: once for each scanline version
      std::pair<std::vector<u8>, std::vector<u8> > compressedScanlinedPair;
      AddScanlinesHelper(img, compressedScanlinedPair);
      
      anim.rleFrames.push_back(std::move(compressedScanlinedPair));
    }
  }
  
  FaceAnimationManager::AvailableAnim* FaceAnimationManager::GetAnimationByName(const std::string& name)
  {
    auto animIter = _availableAnimations.find(name);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_WARNING("FaceAnimationManager.GetAnimationByName.UnknownName",
                          "Unknown animation requested: %s", name.c_str());
      return nullptr;
    } else {
      return &animIter->second;
    }
  }
  
  Result FaceAnimationManager::AddImage(const std::string& animName, const Vision::Image& faceImg, u32 holdTime_ms)
  {
    AvailableAnim* anim = GetAnimationByName(animName);
    if(nullptr == anim) {
      return RESULT_FAIL;
    }
    
    // Add scanlines and compress each way
    std::pair<std::vector<u8>, std::vector<u8> > compressedScanlinedPair;
    AddScanlinesHelper(faceImg.Threshold(128), compressedScanlinedPair);
    
    anim->rleFrames.push_back(std::move(compressedScanlinedPair));
    
    if(holdTime_ms > IKeyFrame::SAMPLE_LENGTH_MS)
    {
      const s32 numFramesToAdd = holdTime_ms / IKeyFrame::SAMPLE_LENGTH_MS - 1;
      for(s32 i=0; i<numFramesToAdd; ++i)
      {
        anim->rleFrames.push_back({});
      }
    }

    return RESULT_OK;
  }

  Result FaceAnimationManager::ClearAnimation(const std::string& animName)
  {
    AvailableAnim* anim = GetAnimationByName(animName);
    if(anim == nullptr) {
      return RESULT_FAIL;
    } else {
      anim->rleFrames.clear();
      return RESULT_OK;
    }
  }
  
  u32 FaceAnimationManager::GetNumFrames(const std::string &animName)
  {
    AvailableAnim* anim = GetAnimationByName(animName);
    if(nullptr == anim) {
      PRINT_NAMED_WARNING("FaceAnimationManager.GetNumFrames",
                          "Unknown animation requested: %s",
                          animName.c_str());
      return 0;
    } else {
      return static_cast<u32>(anim->GetNumFrames());
    }
  } // GetNumFrames()
  
  const std::vector<u8>* FaceAnimationManager::GetFrame(const std::string& animName, u32 frameNum) const
  {
    auto animIter = _availableAnimations.find(animName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                        "Unknown animation requested: %s.",
                        animName.c_str());
      return nullptr;
    } else {
      const AvailableAnim& anim = animIter->second;
      
      if(frameNum < anim.GetNumFrames()) {
        
        const std::vector<u8>* returnVectorPtr = nullptr;
        if(_firstScanLine == 0)
        {
          returnVectorPtr = &anim.rleFrames[frameNum].first;
        }
        else
        {
          returnVectorPtr = &anim.rleFrames[frameNum].second;
        }
        
        return returnVectorPtr;
        
      } else {
        PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                          "Requested frame number %d is invalid. "
                          "Only %lu frames available in animatino %s.",
                          frameNum, (unsigned long)animIter->second.GetNumFrames(),
                          animName.c_str());
        return nullptr;
      }
    }
  } // GetFrame()
  
  Result FaceAnimationManager::CompressRLE(const Vision::Image& img, std::vector<u8>& rleData)
  {
    // Frame is in 8-bit RLE format:
    // 00xxxxxx   CLEAR COLUMN (x = count)
    // 01xxxxxx   REPEAT COLUMN (x = count)
    // 1xxxxxyy   RLE PATTERN (x = count, y = pattern)
    
    if(img.GetNumRows() != IMAGE_HEIGHT || img.GetNumCols() != IMAGE_WIDTH) {
      PRINT_NAMED_ERROR("FaceAnimationManager.CompressRLE",
                        "Expected %dx%d image but got %dx%d image",
                        IMAGE_WIDTH, IMAGE_HEIGHT, img.GetNumCols(), img.GetNumRows());
      return RESULT_FAIL;
    }

    uint64_t packed[IMAGE_WIDTH];

    memset(packed, 0, sizeof(packed));
    rleData.clear();

    // Convert image into 1bpp column major format
    for(s32 i=0; i<IMAGE_HEIGHT; i++) {
      const u8* pixels = img.GetRow(i);
      
      for(s32 j=0; j<IMAGE_WIDTH; j++) {
        if (!*(pixels++)) { continue ; }

        // Note the trick here to force compiler to make this 64-bit
        // (Would like to just do: 1L << i, but compiler optimizes it out...)
        packed[j] |= 0x8000000000000000L >> (i ^ 63);
      }
    }

    // Begin RLE encoding
    for(int x = 0; x < IMAGE_WIDTH; ) {
      // Clear row encoding
      if (!packed[x]) {
        int count = 0;

        for (; !packed[x] && x < IMAGE_WIDTH && count < 0x40; x++, count++) ;
        rleData.push_back(count-1);

        continue ;
      }

      // Copy row encoding
      if (x >= 1 && packed[x] == packed[x-1]) {
        int count = 0;

        for (; packed[x] == packed[x-1] && x < IMAGE_WIDTH && count < 0x40; x++, count++) ;
        rleData.push_back((count-1) | 0x40);

        continue ;
      }

      // RLE pattern encoding
      uint64_t col = packed[x++];
      int pattern = -1;
      int count = 0;

      for (int y = 0; y < IMAGE_HEIGHT; y += 2, col >>= 2) {
        if ((col & 3) != pattern) {
          // Output value if primed
          if (count > 0) {
            rleData.push_back(0x80 | ((count-1) << 2) | pattern);
          }
          
          pattern = col & 3;
          count = 1;
        } else {
          count++;
        }
      }

      // Will next column use column encoding
      bool column = !packed[x] || (x < IMAGE_WIDTH && packed[x] == packed[x-1]);

      if (!pattern && column) {
        continue ;
      }
      
      rleData.push_back(0x80 | ((count-1) << 2) | pattern);
    }

    if (rleData.size() >= static_cast<size_t>(AnimConstants::MAX_FACE_FRAME_SIZE)) { // RLE compression didn't make the image smaller so just send it raw
      rleData.resize(static_cast<size_t>(AnimConstants::MAX_FACE_FRAME_SIZE));
      uint8_t* packedPtr = (uint8_t*)packed;
      for (int i=0; i<static_cast<size_t>(AnimConstants::MAX_FACE_FRAME_SIZE); ++i) {
        rleData[i] = *packedPtr;
        packedPtr++;
      }
    }
    return RESULT_OK;
  }
  
  
  void FaceAnimationManager::DrawFaceRLE(const std::vector<u8>& rleData,
                                         Vision::Image& outImg)
  {
    outImg = Vision::Image(FaceAnimationManager::IMAGE_HEIGHT, FaceAnimationManager::IMAGE_WIDTH);
    
    // Clear the display
    outImg.FillWith(0);
    
    uint64_t decodedImg[FaceAnimationManager::IMAGE_WIDTH];
    if (rleData.size() == static_cast<size_t>(AnimConstants::MAX_FACE_FRAME_SIZE)) {
      uint8_t* packedPtr = (uint8_t*)decodedImg;
      for (int i=0; i<static_cast<size_t>(AnimConstants::MAX_FACE_FRAME_SIZE); ++i) {
        *packedPtr = rleData[i];
        packedPtr++;
      }
    }
    else
    {
      FaceDisplayDecode(rleData.data(), FaceAnimationManager::IMAGE_HEIGHT, FaceAnimationManager::IMAGE_WIDTH, decodedImg);
    }
    
    // Translate from 1-bit/pixel,column-major ordering to 1-byte/pixel, row-major
    for (u8 i = 0; i < FaceAnimationManager::IMAGE_WIDTH; ++i) {
      for (u8 j = 0; j < FaceAnimationManager::IMAGE_HEIGHT; ++j) {
        if ((decodedImg[i] >> j) & 1) {
          outImg(j,i) = 255;
        }
      }
    }
  } // DrawFaceRLE()
  
} // namespace Cozmo
} // namespace Anki
