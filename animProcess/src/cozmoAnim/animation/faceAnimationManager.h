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

#include <string>
#include <deque>
#include <unordered_map>

#include "anki/common/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/vision/basestation/image.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"

namespace Anki {
  
  // Forward declaration
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {

  // NOTE: this is a singleton class
  class FaceAnimationManager
  {
  public:
    
    static const s32 IMAGE_WIDTH = FACE_DISPLAY_WIDTH;
    static const s32 IMAGE_HEIGHT = FACE_DISPLAY_HEIGHT;
    static const std::string ProceduralAnimName;
    
    // Get a pointer to the singleton instance
    inline static FaceAnimationManager* getInstance();
    static void removeInstance();
    
    void ReadFaceAnimationDir(const Util::Data::DataPlatform* dataPlatform, bool fromCache=false);

    // Populate the given RGB565 frame for the given animation with the specified frame number.
    // Returns false if animation or frame do not exist.
    bool GetFrame(const std::string& animName, u32 frameNum, Vision::ImageRGB565& frame);
    
    // Return the total number of frames in the given animation. Returns 0 if the
    // animation doesn't exist.
    u32  GetNumFrames(const std::string& animName);
    
    // Ability to add keyframes at runtime, for procedural face streaming
    Result AddImage(const std::string& animName, const Vision::ImageRGB565& faceImg, u32 holdTime_ms = 0);
    
    // Remove all frames from an existing animation
    Result ClearAnimation(const std::string& animName);
    
    // Clear all FaceAnimations
    void Clear();
    
    // Get the total number of available animations
    size_t GetNumAvailableAnimations() const;
    
  protected:
    
    // Protected default constructor for singleton.
    FaceAnimationManager();
    
    // Pops the front frame of the ProceduralAnim only
    void PopFront();

    static FaceAnimationManager* _singletonInstance;

    struct AvailableAnim {
      time_t lastLoadedTime;
      std::deque< Vision::ImageRGB565 > frames;
      size_t GetNumFrames() const { return frames.size(); }
    };
    
    AvailableAnim* GetAnimationByName(const std::string& name);
    void LoadAnimationImageFrames(const std::string& animationFolder, const std::string& animName);
    
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

