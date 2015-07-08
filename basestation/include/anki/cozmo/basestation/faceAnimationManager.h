/**
 * File: faceAnimationManager.h
 *
 * Author: Andrew Stein
 * Date:   7/7/2015
 *
 * Description: Defines container for managing available animations for the robot's face display.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_FACE_ANIMATION_MANAGER_H
#define ANKI_COZMO_FACE_ANIMATION_MANAGER_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include <string>
#include <vector>
#include <unordered_map>

// Forward declaration
namespace cv {
  class Mat;
}

namespace Anki {
namespace Cozmo {
  
  // NOTE: this is a singleton class
  class FaceAnimationManager
  {
  public:
    static const s32 IMAGE_WIDTH = 128;
    static const s32 IMAGE_HEIGHT = 64;
    
    // Get a pointer to the singleton instance
    inline static FaceAnimationManager* getInstance();
    static void removeInstance();
    
    // Set the root directory of animations, and trigger a read of available
    // animations in it.
    bool SetRootDir(const char* dir);

    // Grab the requested frame for the given animation and populate the
    // buffer with it.
    Result GetFrame(const std::string& animName, u32 frameNum, u8* buffer) const;
    
    // Return the total number of frames in the given animation. Returns 0 if the
    // animation doesn't exist.
    u32  GetNumFrames(const std::string& animName) const;
    
  protected:
    
    static Result CompressRLE(const cv::Mat& image, u8* rleData);
    
    // Protected default constructor for singleton.
    FaceAnimationManager();
    
    static FaceAnimationManager* _singletonInstance;
    
    bool         _hasRootDir;
    std::string  _rootDir;
    
    struct AvailableAnim {
      time_t lastLoadedTime;
      std::vector<std::string> filenames;
      size_t GetNumFrames() const { return filenames.size(); }
    };
    
    std::unordered_map<std::string, AvailableAnim> _availableAnimations;
    
     Result ReadFaceAnimationDir();
    
  }; // class FaceAnimationManager
  
  
  //
  // Inlined Implementations
  //
  
  inline FaceAnimationManager* FaceAnimationManager::getInstance()
  {
    // If we haven't already instantiated the singleton, do so now.
    if(0 == _singletonInstance) {
      _singletonInstance = new FaceAnimationManager();
    }
    
    return _singletonInstance;
  }
  
  
} // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_FACE_ANIMATION_MANAGER_H

