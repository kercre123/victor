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
#include <unordered_map>

#include "cannedAnimLib/faceAnimation.h"
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/vision/engine/image.h"

namespace Anki {
  
  // Forward declaration
  namespace Util {
  namespace Data {
    class DataPlatform;
  }
  }
  
namespace Cozmo {
  
  class FaceAnimation;

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
    
    // Populate the given frame for the given animation with the specified frame number.
    // Returns false if animation or frame do not exist.
    bool GetFrame(const std::string& animName, u32 frameNum, Vision::Image& frame);
    bool GetFrame(const std::string& animName, u32 frameNum, Vision::ImageRGB565& frame);
    
    // Returns true if the underlying images for the given face animation are grayscale.
    bool IsGrayscale(const std::string& animName);
    
    // Return the total number of frames in the given animation. Returns 0 if the
    // animation doesn't exist.
    u32  GetNumFrames(const std::string& animName);
    
    // Ability to add image keyframes  to the special "procedural" anim at runtime, for procedural face streaming
    Result AddProceduralImage(const Vision::Image& faceImg, u32 holdTime_ms = 0);
    Result AddProceduralImage(const Vision::ImageRGB565& faceImg, u32 holdTime_ms = 0);
    
    // Remove all frames from an existing animation
    Result ClearAnimation(const std::string& animName);
    
    // Clear all FaceAnimations
    void Clear();
    
    // Get the total number of available animations
    size_t GetNumAvailableAnimations() const;
    
  protected:
    
    // Protected default constructor for singleton.
    FaceAnimationManager();
    
    // Templated helper to add underlying frames to the procedural FaceAnimation
    template <typename ImageType>
    Result AddProceduralImageHelper(const ImageType& faceImg, const bool isGrayscale, const u32 holdTime_ms);
    
    // Templated helper to get the underlying frames from a FaceAnimation
    template <typename ImageType>
    bool GetFrameHelper(const std::string& animName, u32 frameNum, ImageType& frame);
    
    // Pops the front frame of the ProceduralAnim only
    void PopFront();

    static FaceAnimationManager* _singletonInstance;
    
    FaceAnimation* GetAnimationByName(const std::string& name);
    void LoadAnimationImageFrames(std::string animationFolder, std::string animName);
    
    std::unordered_map<std::string, FaceAnimation> _availableAnimations;

  private:

    template<class ImageType>
    void LoadImage(FaceAnimation& anim, const std::string& fullFilename);
    
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

