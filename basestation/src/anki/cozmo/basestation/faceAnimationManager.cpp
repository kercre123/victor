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
    //  0 terminates the image
    //  1-63 draw N full lines (N*128 pixels) of black or blue
    //  64-255 draw 0-191 pixels (N-64) of black or blue, then invert the color for the next run
    // The decoder starts out drawing black, and inverts the color on every byte >= 64
    
    if(img.rows != IMAGE_HEIGHT || img.cols != IMAGE_WIDTH) {
      PRINT_NAMED_ERROR("FaceAnimationManager.CompressRLE",
                        "Expected %dx%d image but got %dx%d image\n",
                        IMAGE_WIDTH, IMAGE_HEIGHT, img.cols, img.rows);
      return RESULT_FAIL;
    }
    
    bool drawingBlack = true;
    rleData.clear();
    
    s32 nrows = img.rows;
    s32 ncols = img.cols;
    if(img.isContinuous()) {
      ncols *= nrows;
      nrows = 1;
    }
    
    const u32 totalNumPixels = nrows * ncols;
    u32 pixCount = 0;
    u32 currColorCount = 0;
    
    for(s32 i=0; i<nrows; i++) {
      const u8* img_i = img.ptr(i);
      for(s32 j=0; j<ncols; j++, ++pixCount) {
        u8 pixel = img_i[j];
    
        bool colorChange = (pixel > 0 && drawingBlack) || (pixel == 0 && !drawingBlack);
        bool lastPixel = (pixCount == totalNumPixels - 1);

        // If the color changed (or this is the last pixel), then add the bytes for the previous color
        // and restart the counter for the new color.
        if (colorChange || lastPixel) {
          // If the last pixel is the same as the previous pixel, then increment currColorCount,
          // since we normally only get here if the color changed. If the last group of pixels
          // is black, however, just exit now.
          if (!colorChange) {
            if (drawingBlack) {
              break;
            }
            ++currColorCount;
          }

          u32 numRows = currColorCount / 128;
          u32 numRemainderPixels = currColorCount % 128;
          if (numRows > 0) {
            // If all image pixels are on, then we need to split up the count since max
            // rows you can draw at a time is 63.
            if (numRows == 64) {
              --numRows;
              numRemainderPixels = 128;
            }
            rleData.push_back(numRows);
          }
          rleData.push_back(numRemainderPixels + 64);

          drawingBlack = !drawingBlack;
          currColorCount = 1;
          
          // If the last pixel is white and it's a different color from the previous one, add it now.
          if (lastPixel && colorChange && !drawingBlack) {
            rleData.push_back(65);
          }
        } else {
          ++currColorCount;
        }
      }
    }
    
    // Terminator
    rleData.push_back(0);
    
    return RESULT_OK;
  }
  
} // namespace Cozmo
} // namespace Anki
