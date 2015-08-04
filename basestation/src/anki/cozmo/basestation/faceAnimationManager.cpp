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

#include "anki/common/basestation/platformPathManager.h"

#include "util/logging/logging.h"

#include "opencv2/highgui/highgui.hpp"

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
  
  FaceAnimationManager::FaceAnimationManager()
  : _hasRootDir(false)
  {
    SetRootDir("");
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
  Result FaceAnimationManager::ReadFaceAnimationDir()
  {
    const std::string animationFolder = _rootDir;

    DIR* dir = opendir(animationFolder.c_str());
    if ( dir != nullptr) {
      dirent* ent = nullptr;
      while ( (ent = readdir(dir)) != nullptr) {
        if (ent->d_type == DT_DIR && ent->d_name[0] != '.') {
          const std::string animName(ent->d_name);
          std::string fullDirName = animationFolder + ent->d_name;
          struct stat attrib{0};
          int result = stat(fullDirName.c_str(), &attrib);
          if (result == -1) {
            PRINT_NAMED_WARNING("FaceAnimationManager.ReadFaceAnimationDir",
                                "could not get mtime for %s", fullDirName.c_str());
            continue;
          }
          bool loadAnimDir = false;
          auto mapIt = _availableAnimations.find(animName);
          if (mapIt == _availableAnimations.end()) {
            _availableAnimations[animName].lastLoadedTime = attrib.st_mtimespec.tv_sec;
            loadAnimDir = true;
          } else {
            if (mapIt->second.lastLoadedTime < attrib.st_mtimespec.tv_sec) {
              mapIt->second.lastLoadedTime = attrib.st_mtimespec.tv_sec;
              loadAnimDir = true;
            } else {
              //PRINT_NAMED_INFO("Robot.ReadAnimationFile", "old time stamp for %s", fullFileName.c_str());
            }
          }
          if (loadAnimDir) {
            
            DIR* animDir = opendir(fullDirName.c_str());
            if(animDir != nullptr) {
              dirent* frameEntry = nullptr;
              while ( (frameEntry = readdir(animDir)) != nullptr) {
                if(frameEntry->d_type == DT_REG && frameEntry->d_name[0] != '.') {
                  
                  // Get the frame number in this filename
                  const std::string filename(frameEntry->d_name);
                  size_t underscorePos = filename.find_last_of("_");
                  size_t dotPos = filename.find_last_of(".");
                  if(dotPos == std::string::npos) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Could not find '.' in frame filename %s\n",
                                      filename.c_str());
                    return RESULT_FAIL;
                  } else if(underscorePos == std::string::npos) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Could not find '_' in frame filename %s\n",
                                      filename.c_str());
                    return RESULT_FAIL;
                  } else if(dotPos <= underscorePos+1) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Unexpected relative positions for '.' and '_' in frame filename %s\n",
                                      filename.c_str());
                    return RESULT_FAIL;
                  }
                  
                  const std::string digitStr(filename.substr(underscorePos+1,
                                                             (dotPos-underscorePos-1)));
                  
                  s32 frameNum = 0;
                  try {
                    frameNum = std::stoi(digitStr);
                  } catch (std::invalid_argument&) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Could not get frame number from substring '%s' "
                                      "of filename '%s'.\n",
                                      digitStr.c_str(), filename.c_str());
                    return RESULT_FAIL;
                  }
                  
                  if(frameNum < 0) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Found negative frame number (%d) for filename '%s'.\n",
                                      frameNum, filename.c_str());
                    return RESULT_FAIL;
                  }
                  
                  AvailableAnim& anim = _availableAnimations[animName];
                  
                  // Add empty frames if there's a gap
                  if(frameNum > 1) {
                    
                    s32 emptyFramesAdded = 0;
                    while(anim.rleFrames.size() < frameNum-1) {
                      anim.rleFrames.push_back({});
                      ++emptyFramesAdded;
                    }
                    
                    if(emptyFramesAdded > 0) {
                      PRINT_NAMED_INFO("FaceAnimationManager.ReadFaceAnimationDir",
                                       "Inserted %d empty frames before frame %d in animation %s.\n",
                                       emptyFramesAdded, frameNum, animName.c_str());
                    }
                  }
                  
                  // Read the image
                  const std::string fullFilename(fullDirName + PATH_SEPARATOR + frameEntry->d_name);
                  cv::Mat img = cv::imread(fullFilename, CV_LOAD_IMAGE_GRAYSCALE);
                  
                  if(img.rows != IMAGE_HEIGHT || img.cols != IMAGE_WIDTH) {
                    PRINT_NAMED_ERROR("FaceAnimationManager.ReadFaceAnimationDir",
                                      "Image in %s is %dx%d instead of %dx%d.\n",
                                      fullFilename.c_str(),
                                      img.cols, img.rows,
                                      IMAGE_WIDTH, IMAGE_HEIGHT);
                    return RESULT_FAIL;
                  }
                  
                  // Binarize
                  img = img > 128;
                  
                  // DEBUG
                  //cv::imshow("FaceAnimImage", img);
                  //cv::waitKey(30);
                  
                  anim.rleFrames.push_back({});
                  CompressRLE(img, anim.rleFrames.back());
                }
              }
            }
            
            PRINT_NAMED_INFO("FaceAnimationManager.ReadFaceAnimationDir",
                             "Added %lu files/frames to animation %s",
                             _availableAnimations[animName].GetNumFrames(),
                             animName.c_str());
          }
        }
      }
      closedir(dir);
    } else {
      PRINT_NAMED_INFO("FaceAnimationManager.ReadFaceAnimationDir", "folder not found %s", animationFolder.c_str());
    }
    
    return RESULT_OK;
    
  } // ReadFaceAnimationDir()
  
  
  bool FaceAnimationManager::SetRootDir(const std::string dir)
  {
    _hasRootDir = false;
    
    std::string fullPath(PREPEND_SCOPED_PATH(PlatformPathManager::FaceAnimation, dir));
    
    // Check if directory exists
    struct stat info;
    if( stat( fullPath.c_str(), &info ) != 0 ) {
      PRINT_NAMED_WARNING("FaceAnimationManager.SetRootDir.NoAccess",
                          "Could not access path %s (errno %d)\n",
                          fullPath.c_str(), errno);
      return false;
    }
    if (!S_ISDIR(info.st_mode)) {
      PRINT_NAMED_WARNING("FaceAnimationManager.SetRootDir.NotADir", "\n");
      return false;
    }
    
    _hasRootDir = true;
    _rootDir = fullPath;
    
    // Every sub-directory in the root directory is considered an available
    // animation, with as many frames as there are files
    ReadFaceAnimationDir();
    
    return true;
    
  } // SetRootDir()
  
  u32 FaceAnimationManager::GetNumFrames(const std::string &animName) const
  {

    auto animIter = _availableAnimations.find(animName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_WARNING("FaceAnimationManager.GetNumFrames",
                          "Unknown animation requested: %s.\n",
                          animName.c_str());
      return 0;
    } else {
      return static_cast<u32>(animIter->second.GetNumFrames());
    }
  } // GetNumFrames()
  
  const std::vector<u8>* FaceAnimationManager::GetFrame(const std::string& animName, u32 frameNum) const
  {
    auto animIter = _availableAnimations.find(animName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                        "Unknown animation requested: %s.\n",
                        animName.c_str());
      return nullptr;
    } else {
      const AvailableAnim& anim = animIter->second;
      
      if(frameNum < anim.GetNumFrames()) {
        
        return &(anim.rleFrames[frameNum]);
        
      } else {
        PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                          "Requested frame number %d is invalid. "
                          "Only %lu frames available in animatino %s.\n",
                          frameNum, animIter->second.GetNumFrames(),
                          animName.c_str());
        return nullptr;
      }
    }
  } // GetFrame()
  
  Result FaceAnimationManager::CompressRLE(const cv::Mat& img, std::vector<u8>& rleData)
  {
    // Frame is in 8-bit RLE format:
    // 00xxxxxx   CLEAR COLUMN (x = count)
    // 01xxxxxx   REPEAT COLUMN (x = count)
    // 1xxxxxyy   RLE PATTERN (x = count, y = pattern)
    
    if(img.rows != IMAGE_HEIGHT || img.cols != IMAGE_WIDTH) {
      PRINT_NAMED_ERROR("FaceAnimationManager.CompressRLE",
                        "Expected %dx%d image but got %dx%d image\n",
                        IMAGE_WIDTH, IMAGE_HEIGHT, img.cols, img.rows);
      return RESULT_FAIL;
    }

    uint64_t packed[IMAGE_WIDTH];

    memset(packed, 0, sizeof(packed));
    rleData.clear();

    // Convert image into 1bpp column major format
    for(s32 i=0; i<IMAGE_HEIGHT; i++) {
      const u8* pixels = img.ptr(i);
      
      for(s32 j=0; j<IMAGE_WIDTH; j++) {
        if (!*(pixels++)) { continue ; }

        // If lower half of face disappears, change to 0x8000000000000000L >> (i ^ 63)
        packed[j] |= 1L << i;
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

      // If next row is a column encoding, we don't need to encode blank patterns
      if (!pattern && !packed[x]) {
        continue ;
      }
      
      rleData.push_back(0x80 | ((count-1) << 2) | pattern);
    }
    
    return RESULT_OK;
  }
  
} // namespace Cozmo
} // namespace Anki
