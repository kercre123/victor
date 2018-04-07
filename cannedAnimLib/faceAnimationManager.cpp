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

#include "cannedAnimLib/faceAnimationManager.h"
#include "cannedAnimLib/keyframe.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/data/dataScope.h"
#include "coretech/common/engine/array2d_impl.h"
#include "util/console/consoleInterface.h"
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

  // Enable/disable automatic adding of scanlines (interlacing) to face images.
  // Use with care (not using interlacing can result in face screen damage).
  // Note that this only changes scanline behavior for any face images read or sent in _after_ it is set.
  // For example, unless a re-read of the face animation dir is triggered, setting this will have no
  // effect on face animations read from disk at startup.
  CONSOLE_VAR(bool, kAddScanlinesToFaceImages, "FaceAnimation", true);
  
  FaceAnimationManager* FaceAnimationManager::_singletonInstance = nullptr;
  const std::string FaceAnimationManager::ProceduralAnimName("_PROCEDURAL_");
  
  FaceAnimationManager::FaceAnimationManager()
  {
    // Add the special ProceduralAnim to the list of available anims,
    // and make it use a color underlying image type.
    FaceAnimation proceduralAnim;
    proceduralAnim.SetGrayscale(false);
    _availableAnimations[ProceduralAnimName] = proceduralAnim;
  }
  
  void FaceAnimationManager::removeInstance()
  {
    // check if the instance has been created yet
    if(nullptr != _singletonInstance) {
      delete _singletonInstance;
      _singletonInstance = nullptr;
    }
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

    // Set up the worker that will process all the image frame folders
    using MyDispatchWorker = Util::DispatchWorker<3, std::string, std::string>;
    MyDispatchWorker::FunctionType workerFunction = std::bind(&FaceAnimationManager::LoadAnimationImageFrames, 
                                                              this, std::placeholders::_1, std::placeholders::_2);
    MyDispatchWorker worker(workerFunction);

    const std::vector<std::string> paths = {"assets/faceAnimations/", "config/facePNGs/faceAnimations/"};

    for(const auto& path : paths)
    {
      const std::string animationFolder = dataPlatform->pathToResource(resourceScope, path);
      
      // Get the list of all the directory names
      std::vector<std::string> faceAnimDirNames;
      Util::FileUtils::ListAllDirectories(animationFolder, faceAnimDirNames);
      
      // Go through the list of directories, removing disallowed names, updating timestamps, and removing ones that don't need to be loaded
      auto nameIter = faceAnimDirNames.begin();
      while (nameIter != faceAnimDirNames.end())
      {
        const std::string& animName = *nameIter;
        
        // Remove the disallowed name from the list if it's there
        if (animName == ProceduralAnimName)
        {
          PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir.ReservedName",
                            "'%s' is a reserved face animation name. Ignoring.", ProceduralAnimName.c_str());
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
          _availableAnimations[animName].SetLastLoadedTime(tmpSeconds);
        } else if (fromCache) {
          // It should probably be the default behavior to clear the
          // "frames" vector when "loadAnimDir" is true, right?
          _availableAnimations[animName].Clear();
          _availableAnimations[animName].SetLastLoadedTime(tmpSeconds);
        } else {
          if (mapIt->second.GetLastLoadedTime() < tmpSeconds) {
            mapIt->second.SetLastLoadedTime(tmpSeconds);
          } else {
            // If we already have this anim loaded and its timestamp isn't old, we don't need to reload it
            nameIter = faceAnimDirNames.erase(nameIter);
            continue;
          }
        }
        
        // Now we can start looking at the next name, this one is ok to load
        worker.PushJob(animationFolder, animName);
        ++nameIter;
      }
    }
    
    // Go through and load the anims from our list
    worker.Process();
    
  } // ReadFaceAnimationDir()
  
  
  bool FaceAnimationManager::GetFrame(const std::string &animName, u32 frameNum, Vision::Image& frame)
  {
    return GetFrameHelper<Vision::Image>(animName, frameNum, frame);
  }
  
  
  bool FaceAnimationManager::GetFrame(const std::string &animName, u32 frameNum, Vision::ImageRGB565& frame)
  {
    return GetFrameHelper<Vision::ImageRGB565>(animName, frameNum, frame);
  }
  
  template<class ImageType>
  void FaceAnimationManager::LoadImage(FaceAnimation& anim, const std::string& fullFilename)
  {
    ImageType img;
    Result loadResult = img.Load(fullFilename);
    
    if(loadResult != RESULT_OK) {
      PRINT_NAMED_ERROR("FaceAnimationManager.LoadImage.LoadError", "%s", fullFilename.c_str());
      return;
    }
    if(img.GetNumRows() != IMAGE_HEIGHT || img.GetNumCols() != IMAGE_WIDTH) {      
      img.Resize(IMAGE_HEIGHT, IMAGE_WIDTH);
    }
    
    anim.AddFrame(img);
  }
  
  void FaceAnimationManager::LoadAnimationImageFrames(std::string animationFolder, std::string animName)
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
      
      FaceAnimation& anim = _availableAnimations[animName];

      // If the animation name has rgb in it then load it as rgb
      if(animName.find("rgb") != std::string::npos)
      {
        anim.SetGrayscale(false);
      }
      // Otherwise load as grayscale
      else
      {
        anim.SetGrayscale(true);
      }
      
      // Add empty frames if there's a gap
      if(frameNum > 1) {
        
        s32 emptyFramesAdded = 0;
        while(anim.GetNumFrames() < frameNum-1) {
          anim.AddFrame(Vision::Image{});
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
      if(anim.IsGrayscale())
      {
        LoadImage<Vision::Image>(anim, fullFilename);
      }
      else
      {
        LoadImage<Vision::ImageRGB565>(anim, fullFilename);
      }
    }
  }
  
  FaceAnimation* FaceAnimationManager::GetAnimationByName(const std::string& name)
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
  
  Result FaceAnimationManager::AddProceduralImage(const Vision::Image& faceImg, u32 holdTime_ms)
  {
    const bool isGrayscale = true;
    return AddProceduralImageHelper(faceImg, isGrayscale, holdTime_ms);
  }
  
  Result FaceAnimationManager::AddProceduralImage(const Vision::ImageRGB565& faceImg, u32 holdTime_ms)
  {
    const bool isGrayscale = false;
    return AddProceduralImageHelper(faceImg, isGrayscale, holdTime_ms);
  }

  Result FaceAnimationManager::ClearAnimation(const std::string& animName)
  {
    FaceAnimation* anim = GetAnimationByName(animName);
    if(anim == nullptr) {
      return RESULT_FAIL;
    } else {
      anim->Clear();
      return RESULT_OK;
    }
  }
  
  u32 FaceAnimationManager::GetNumFrames(const std::string &animName)
  {
    FaceAnimation* anim = GetAnimationByName(animName);
    if(nullptr == anim) {
      PRINT_NAMED_WARNING("FaceAnimationManager.GetNumFrames",
                          "Unknown animation requested: %s",
                          animName.c_str());
      return 0;
    } else {
      return static_cast<u32>(anim->GetNumFrames());
    }
  } // GetNumFrames()
  
  
  bool FaceAnimationManager::IsGrayscale(const std::string& animName)
  {
    const auto* anim = GetAnimationByName(animName);
    if (anim == nullptr) {
      PRINT_NAMED_ERROR("FaceAnimationManager.IsGrayscale",
                        "Unknown animation requested: %s.",
                        animName.c_str());
      return false;
    }
    
    return anim->IsGrayscale();
  }
  
  template <typename ImageType>
  Result FaceAnimationManager::AddProceduralImageHelper(const ImageType& faceImg, const bool isGrayscale, const u32 holdTime_ms)
  {
    const auto& animName = ProceduralAnimName;
    
    FaceAnimation* anim = GetAnimationByName(animName);
    if(nullptr == anim) {
      return RESULT_FAIL;
    }
    
    // Set the animation to grayscale/color if not already
    if (isGrayscale != anim->IsGrayscale()) {
      const bool animIsEmpty = (anim->GetNumFrames() == 0);
      if (!animIsEmpty) {
        PRINT_NAMED_WARNING("FaceAnimationManager.AddProceduralImageHelper.ClearingExisting",
                            "%s: Clearing existing face animation frames since they are %sgrayscale.",
                            animName.c_str(),
                            isGrayscale ? "NOT " : "");
        anim->Clear();
      }
      anim->SetGrayscale(isGrayscale);
    }
    
    anim->AddFrame(faceImg, (holdTime_ms == 0));
    
    if(holdTime_ms > ANIM_TIME_STEP_MS) {
      const s32 numFramesToAdd = holdTime_ms / ANIM_TIME_STEP_MS - 1;
      for(s32 i=0; i<numFramesToAdd; ++i) {
        anim->AddFrame(ImageType{});
      }
    }
    
    return RESULT_OK;
  }
  
  template <typename ImageType>
  bool FaceAnimationManager::GetFrameHelper(const std::string& animName, u32 frameNum, ImageType& frame)
  {
    auto animIter = _availableAnimations.find(animName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                        "Unknown animation requested: %s.",
                        animName.c_str());
      return false;
    } else {
      FaceAnimation& anim = animIter->second;
      
      if ((animName == ProceduralAnimName)) 
      {
        if(anim.GetNumFrames() == 0)
        {
          return false;
        }

        anim.GetFrame(0, frame);

        // Pop front if the anim is not holding or
        // it is holding and there is more than one frame left
        const bool shouldPop = !anim.ShouldHold() ||
                               (anim.ShouldHold() && anim.GetNumFrames() > 1);

        if(shouldPop)
        {
          PopFront();
        }

        return !frame.IsEmpty();
      } 
      else if(frameNum < anim.GetNumFrames()) 
      {
        anim.GetFrame(frameNum, frame);
        return true;
      } 
      else
      {
        PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                          "Requested frame number %d is invalid. "
                          "Only %lu frames available in animation %s.",
                          frameNum, (unsigned long)anim.GetNumFrames(),
                          animName.c_str());
        return false;
      }
    }
  } // GetFrameHelper()
  
  
  void FaceAnimationManager::PopFront()
  {
    auto animIter = _availableAnimations.find(ProceduralAnimName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_ERROR("FaceAnimationManager.PopFront.NoProceduralAnim", "");
      return;
    } else {
      FaceAnimation& anim = animIter->second;
      anim.PopFront();
    }
  }

} // namespace Cozmo
} // namespace Anki
