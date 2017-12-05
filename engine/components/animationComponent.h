/**
 * File: animationComponent.h
 *
 * Author: Kevin Yoon
 * Created: 2017-08-01
 *
 * Description: Control interface for animation process to manage execution of
 *              canned and idle animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Components_AnimationComponent_H__
#define __Cozmo_Basestation_Components_AnimationComponent_H__

#include "anki/common/types.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/shared/animationTag.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "engine/actions/actionInterface.h"
#include "engine/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"

#include <unordered_map>

namespace Anki {
namespace Cozmo {

// Forward declarations
class AnimationGroupContainer;
class Robot;
class CozmoContext;
  
class AnimationComponent : private Anki::Util::noncopyable, private Util::SignalHolder
{
public:
  
  using Tag = AnimationTag;
  
  enum class AnimResult {
    Completed = 0,   // Animation completed successfully
    Aborted,         // Animation was aborted
    Timedout,        // Animation timed out
    Stale,           // Animation still expecting response, didn't timeout, but tagCtr has rolled over and tag is being reused!
  };
  
  
  AnimationComponent(Robot& robot, const CozmoContext* context);

  void Init();
  void Update();
  
  typedef struct {
    u32 length_ms;
  } AnimationMetaInfo;

  Result GetAnimationMetaInfo(const std::string& animName, AnimationMetaInfo& metaInfo) const;

  void DoleAvailableAnimations();
  
  // Returns true when the list of available animations has been received from animation process
  bool IsInitialized() { return _isInitialized; }

  using AnimationCompleteCallback = std::function<void(const AnimResult res)>;
  
  const std::string& GetAnimationNameFromGroup(const std::string& name) const;
  
  // Tell animation process to play the specified animation
  // If a non-empty callback is specified, the actionTag of the calling action must be specified
  Result PlayAnimByName(const std::string& animName,
                        int numLoops = 1,
                        bool interruptRunning = true,
                        AnimationCompleteCallback callback = nullptr,
                        const u32 actionTag = 0,
                        float timeout_sec = _kDefaultTimeout_sec);
  
  bool IsPlayingAnimation() const { return _callbackMap.size() > 0; }
  
  Result StopAnimByName(const std::string& animName);

  // If you want to play multiple frames in sequence, duration_ms should be multiples of ANIM_TIME_STEP_MS.
  Result DisplayFaceImageBinary(const Vision::Image& img, u32 duration_ms, bool interruptRunning = false);
  Result DisplayFaceImage(const Vision::ImageRGB& img, u32 duration_ms, bool interruptRunning = false);
  Result DisplayFaceImage(const Vision::ImageRGB565& imgRGB565, u32 duration_ms, bool interruptRunning = false);

  // Enables only the specified tracks. 
  // Status of other tracks remain unchanged.
  void UnlockTracks(u8 tracks);
  void UnlockAllTracks();

  // Disables only the specified tracks. 
  // Status of other tracks remain unchanged.
  void LockTracks(u8 tracks);

  u8   GetLockedTracks() const {return _lockedTracks; }
  
  bool                IsAnimating()        const { return _isAnimating;  }
  const std::string&  GetPlayingAnimName() const { return _currAnimName; }
  u8                  GetPlayingAnimTag()  const { return _currAnimTag;  }

  // Accessors for latest animState values
  u32 GetAnimState_NumProcAnimFaceKeyframes() const { return _animState.numProcAnimFaceKeyframes; }   
  u8  GetAnimState_LockedTracks()             const { return _animState.lockedTracks;             }
  u8  GetAnimState_TracksInUse()              const { return _animState.tracksInUse;              }

  // Event/Message handling
  template<typename T>
  void HandleMessage(const T& msg);
  
  // Robot message handlers
  void HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimationEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimState(const AnkiEvent<RobotInterface::RobotToEngine>& message);  

private:
  
  // Returns Tag if animation is playing.
  // Return NotAnimating otherwise.
  Tag IsAnimPlaying(const std::string& animName);
  
  Tag GetNextTag() { return ++_tagCtr; }

  
  static constexpr float _kDefaultTimeout_sec = 60.f;

  bool _isInitialized;
  Tag  _tagCtr;
  
  Robot& _robot;

  AnimationGroupContainer&  _animationGroups;
  
  // Map of available canned animations to associated metainfo
  std::unordered_map<std::string, AnimationMetaInfo> _availableAnims;
  
  bool _isDolingAnims;
  std::string _nextAnimToDole;
  
  std::string _currPlayingAnim;

  u8 _lockedTracks;

  // For tracking whether or not an animation is playing based on
  // AnimStarted and AnimEnded messages
  bool          _isAnimating;
  std::string   _currAnimName;
  Tag           _currAnimTag;

  // Latest state message received from anim process
  AnimationState _animState;

  struct AnimCallbackInfo {
    AnimCallbackInfo(const std::string animName,
                     const AnimationCompleteCallback& callback,
                     const u32 actionTag,
                     const float abortTime_sec)
    : animName(animName)
    , callback(callback)
    , actionTag(actionTag)
    , abortTime_sec(abortTime_sec)
    {}
    
    void ExecuteCallback(AnimResult res)
    {
      // Execute callback as long as it's non-null and
      // 1) No actionTag (i.e. actionTag == 0) was associated with it
      // 2) Or the valid calling action is still active
      if ((callback != nullptr) &&
          ((actionTag == 0) || IActionRunner::IsTagInUse(actionTag))) {
        callback(res);
      }
    }
    
    const std::string animName;
    const AnimationCompleteCallback callback;
    const u32 actionTag;
    const float abortTime_sec;
  };

  // Map of animation tags to info needed for handling callbacks when the animation completes
  std::unordered_map<Tag, AnimCallbackInfo> _callbackMap;
  
};


} // namespace Cozmo
} // namespace Anki

#endif
