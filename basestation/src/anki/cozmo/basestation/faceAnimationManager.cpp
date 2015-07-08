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
    const std::string animationFolder = PREPEND_SCOPED_PATH(FaceAnimation, "");

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
                if(frameEntry->d_type == DT_REG) {
                  _availableAnimations[animName].filenames.emplace_back(fullDirName + PATH_SEPARATOR + frameEntry->d_name);
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
  
  
  bool FaceAnimationManager::SetRootDir(const char* dir)
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
      return animIter->second.GetNumFrames();
    }
  } // GetNumFrames()
  
  Result FaceAnimationManager::GetFrame(const std::string &animName, u32 frameNum, u8 *buffer) const
  {
    auto animIter = _availableAnimations.find(animName);
    if(animIter == _availableAnimations.end()) {
      PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                        "Unknown animation requested: %s.\n",
                        animName.c_str());
      return RESULT_FAIL;
    } else {
      const AvailableAnim& anim = animIter->second;
      
      if(frameNum < anim.GetNumFrames()) {
        cv::Mat img = cv::imread(anim.filenames[frameNum], CV_LOAD_IMAGE_GRAYSCALE);
        
        if(img.rows != IMAGE_HEIGHT || img.cols != IMAGE_WIDTH) {
          PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                            "Image in %s is %dx%d instead of %dx%d.\n",
                            anim.filenames[frameNum].c_str(),
                            img.cols, img.rows,
                            IMAGE_WIDTH, IMAGE_HEIGHT);
          return RESULT_FAIL;
        }
        
        // Binarize
        img = img > 128;
        
        cv::imshow("FaceAnimImage", img);
        cv::waitKey(30);
        
        Result compressResult = CompressRLE(img, buffer);
        
        if(compressResult != RESULT_OK) {
          PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                            "Failed to compress image %s into RLE format.\n",
                            anim.filenames[frameNum].c_str());
          return compressResult;
        }
        
        return RESULT_OK;
        
      } else {
        PRINT_NAMED_ERROR("FaceAnimationManager.GetFrame",
                          "Requested frame number %d is invalid. "
                          "Only %lu frames available in animatino %s.\n",
                          frameNum, animIter->second.GetNumFrames(),
                          animName.c_str());
        return RESULT_FAIL;
      }
    }
  }
  
  Result FaceAnimationManager::CompressRLE(const cv::Mat& img, u8 *rleData)
  {
    // Frame is in 8-bit RLE format:
    //  0 terminates the image
    //  1-63 draw N full lines (N*128 pixels) of black or blue
    //  64-255 draw 0-191 pixels (N-64) of black or blue, then invert the color for the next run
    // The decoder starts out drawing black, and inverts the color on every byte >= 64
    
    // Count how many pixels are in each row
    cv::Mat rowSums;
    cv::reduce(img, rowSums, 1, CV_REDUCE_SUM, CV_32S);
    
    if(rowSums.rows != IMAGE_HEIGHT) {
      PRINT_NAMED_ERROR("FaceAnimationManager.CompressRLE",
                        "Unexpected number of rows in rowSums: %d instead of %d\n",
                        rowSums.rows, IMAGE_HEIGHT);
      return RESULT_FAIL;
    }
    
    s32 iRLE = 0;
    bool drawingBlack = true;
    rleData[0] = 0;
    
    s32 *rowSumData = rowSums.ptr<s32>(0);
    
    for(s32 iRow=0; iRow<rowSums.rows; ++iRow)
    {
      const s32 sum = rowSumData[iRow];
      
      if(sum == 0) {
        // All-zero row
        if(drawingBlack) {
          ++rleData[iRLE];
        } else {
          rleData[++iRLE] = 64; // change color
          rleData[++iRLE] = 1;
          drawingBlack = true;
        }
      } else if(sum >= 255*IMAGE_WIDTH) {
        // All-one row       
        if(drawingBlack) {
          if(iRow > 0) {
            ++iRLE;
          }
          rleData[iRLE] = 64; // change color
          rleData[++iRLE] = 1;
          drawingBlack = false;
        } else {
          ++rleData[iRLE];
        }
      } else {
        // Arbitrary number of "on" pixels on this row
        const u8* row = img.ptr<u8>(iRow);
        const bool isBlack = row[0] == 0;
        if(drawingBlack != isBlack) {
          // Need to change the drawing color
          rleData[++iRLE] = 64;
          drawingBlack = !drawingBlack;
        }
        
        // Init new RLE counter at next byte
        rleData[++iRLE] = 65;
        drawingBlack = !drawingBlack;
        
        for(s32 jCol=1; jCol<IMAGE_WIDTH; ++jCol) {
          if(row[jCol] == row[jCol-1]) {
            // Haven't changed color, keep incrementing current RLE count
            ++rleData[iRLE];
          } else {
            // Color just changed, move to next byte
            rleData[++iRLE] = 65;
            drawingBlack = !drawingBlack;
          }
        }
      }
    }
    
    ++iRLE;
    rleData[iRLE] = 0;
    
    return RESULT_OK;
  }
  
} // namespace Cozmo
} // namespace Anki
