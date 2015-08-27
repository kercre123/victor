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
namespace Data {
class DataPlatform;
}

  // NOTE: this is a singleton class
  class FaceAnimationManager
  {
  public:
    static const s32 IMAGE_WIDTH = 128;
    static const s32 IMAGE_HEIGHT = 64;
    static const std::string ProceduralAnimName;
    
    // Get a pointer to the singleton instance
    inline static FaceAnimationManager* getInstance();
    static void removeInstance();
    
    void ReadFaceAnimationDir(Data::DataPlatform* dataPlatform);

    // Get a pointer to an RLE-compressed frame for the given animation.
    // Returns nullptr if animation or frame do not exist.
    const std::vector<u8>* GetFrame(const std::string& animName, u32 frameNum) const;
    
    // Return the total number of frames in the given animation. Returns 0 if the
    // animation doesn't exist.
    u32  GetNumFrames(const std::string& animName);
    
    // Ability to add keyframes at runtime, for procedural face streaming
    Result AddImage(const std::string& animName, const cv::Mat& faceImg);
    
    // Remove all frames from an existing animation
    Result ClearAnimation(const std::string& animName);
    
    // Clear all FaceAnimations
    void Clear();
    
    // Get the total number of available animations
    size_t GetNumAvailableAnimations() const;
    
    static Result CompressRLE(const cv::Mat& image, std::vector<u8>& rleData);
    
  protected:
    
    // Protected default constructor for singleton.
    FaceAnimationManager();
    
    static FaceAnimationManager* _singletonInstance;

    struct AvailableAnim {
      time_t lastLoadedTime;
      //std::vector<std::string> filenames;
      std::vector<std::vector<u8> > rleFrames;
      size_t GetNumFrames() const { return rleFrames.size(); }
    };
    
    AvailableAnim* GetAnimationByName(const std::string& name);
    
    std::unordered_map<std::string, AvailableAnim> _availableAnimations;
    

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
  
  inline void FaceAnimationManager::Clear() {
    _availableAnimations.clear();
  }
  
  // Get the total number of available animations
  inline size_t FaceAnimationManager::GetNumAvailableAnimations() const {
    return _availableAnimations.size();
  }
  
  
} // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_FACE_ANIMATION_MANAGER_H

