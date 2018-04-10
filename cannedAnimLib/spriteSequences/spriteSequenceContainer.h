/**
 * File: spriteSequenceContainer.h
 *
 * Author: Andrew Stein
 * Date:   7/7/2015
 *
 * Description: Defines container for managing available animations for the robot's face display.
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__
#define __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

#include <string>
#include <unordered_map>

#include "cannedAnimLib/spriteSequences/spriteSequence.h"
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/vision/engine/image.h"
#include "util/helpers/noncopyable.h"

namespace Anki {

// Forward declaration
namespace Util {
namespace Data {
class DataPlatform;
}
}

namespace Cozmo {

class SpriteSequence;

class SpriteSequenceContainer : private Util::noncopyable
{
public:
  SpriteSequenceContainer();
  
  static const s32 IMAGE_WIDTH = FACE_DISPLAY_WIDTH;
  static const s32 IMAGE_HEIGHT = FACE_DISPLAY_HEIGHT;
  static const std::string ProceduralAnimName;
  
  void ReadSpriteSequenceDir(const Util::Data::DataPlatform* dataPlatform, bool fromCache=false);
  
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
  
  // Clear all SpriteSequences
  void Clear();
  
  // Get the total number of available animations
  size_t GetNumAvailableAnimations() const;
  
protected:
  // Templated helper to add underlying frames to the procedural SpriteSequence
  template <typename ImageType>
  Result AddProceduralImageHelper(const ImageType& faceImg, const bool isGrayscale, const u32 holdTime_ms);
  
  // Templated helper to get the underlying frames from a SpriteSequence
  template <typename ImageType>
  bool GetFrameHelper(const std::string& animName, u32 frameNum, ImageType& frame);
  
  // Pops the front frame of the ProceduralAnim only
  void PopFront();
  
  SpriteSequence* GetAnimationByName(const std::string& name);
  void LoadAnimationImageFrames(std::string animationFolder, std::string animName);
  
  std::unordered_map<std::string, SpriteSequence> _availableSequences;

private:
  template<class ImageType>
  void LoadImage(SpriteSequence& anim, const std::string& fullFilename);
  
}; // class SpriteSequenceContainer

//
// Inlined Implementations
//

inline void SpriteSequenceContainer::Clear() {
  _availableSequences.clear();
}

// Get the total number of available animations
inline size_t SpriteSequenceContainer::GetNumAvailableAnimations() const {
  return _availableSequences.size();
}

} // namespace Cozmo
} // namespace Anki


#endif // __CannedAnimLib_SpriteSequences_SpriteSequenceContainer_H__

