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

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "anki/cozmo/shared/animationTag.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "engine/actions/actionInterface.h"
#include "engine/events/ankiEvent.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/types/keepFaceAliveParameters.h"
#include "coretech/vision/shared/compositeImage/compositeImageLayer.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"

#include <unordered_map>
#include <unordered_set>

namespace Anki {

// Forward declarations
namespace Vision{
class CompositeImage;
class RGB565ImageBuilder;
}

namespace Cozmo {

// Forward declarations
class Animation;
class AnimationGroupContainer;
class CozmoContext;
class DataAccessorComponent;
class Robot;
  
class AnimationComponent : public IDependencyManagedComponent<RobotComponentID>, 
                           private Anki::Util::noncopyable, 
                           private Util::SignalHolder
{
public:
  
  using Tag = AnimationTag;
  
  enum class AnimResult {
    Completed = 0,   // Animation completed successfully
    Aborted,         // Animation was aborted
    Timedout,        // Animation timed out
    Stale,           // Animation still expecting response, didn't timeout, but tagCtr has rolled over and tag is being reused!
  };
  
  
  AnimationComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::DataAccessor);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
  };
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////


  void Init();
  
  typedef struct {
    u32 length_ms;
  } AnimationMetaInfo;

  Result GetAnimationMetaInfo(const std::string& animName, AnimationMetaInfo& metaInfo) const;

  void DoleAvailableAnimations();
  
  // Returns true when the list of available animations has been received from animation process
  bool IsInitialized() { return _isInitialized; }

  using AnimationCompleteCallback = std::function<void(const AnimResult res)>;
  
  // Set strictCooldown = true when we do NOT want to simply choose the animation closest
  // to being off cooldown when all animations in the group are on cooldown
  const std::string& GetAnimationNameFromGroup(const std::string& name, bool strictCooldown = false) const;
  
  // Tell animation process to play the specified animation
  // If a non-empty callback is specified, the actionTag of the calling action must be specified
  Result PlayAnimByName(const std::string& animName,
                        int numLoops = 1,
                        bool interruptRunning = true,
                        AnimationCompleteCallback callback = nullptr,
                        const u32 actionTag = 0,
                        float timeout_sec = _kDefaultTimeout_sec);

  // Tell animation process to render the specified animation
  // to the Procedural_Eyes layer of the specified composite image
  // OutDuration_ms is set to the length of the animation that is playing back
  Result PlayCompositeAnimation(const std::string& animName,
                                const Vision::CompositeImage& compositeImage, 
                                u32 frameInterval_ms,
                                int& outDuration_ms,
                                bool interruptRunning = true,
                                bool emptySpriteBoxesAreValid = false,
                                AnimationCompleteCallback callback = nullptr);
  
  bool IsPlayingAnimation() const { return _callbackMap.size() > 0; }
  
  Result StopAnimByName(const std::string& animName);

  // If you want to play multiple frames in sequence, duration_ms should be a multiple of ANIM_TIME_STEP_MS.
  //
  // Note: If you're streaming a real-time sequence, the rate at which you stream should also approximately
  // match the duration. e.g. If you're streaming one image every engine tick, then the duration should be
  // 2 x ANIM_TIME_STEP_MS which is roughly equal to BS_TIME_STEP_MS. For convenience you can use this.
  static constexpr u32 DEFAULT_STREAMING_FACE_DURATION_MS = ANIM_TIME_STEP_MS * ( (BS_TIME_STEP_MS % ANIM_TIME_STEP_MS == 0) ?
                                                                                  (BS_TIME_STEP_MS / ANIM_TIME_STEP_MS)      :
                                                                                  (BS_TIME_STEP_MS / ANIM_TIME_STEP_MS) + 1 );
  // (Didn't use the obvious 'ceil' function here because it's non-const and can't
  //  be used to init a static constexpr.)
  //
  // If the durations are too short, it may allow for procedural faces to (sporadically) interrupt the
  // face images. If the durations are too long, you won't be streaming in real-time. In either case you
  // should use GetAnimState_NumProcAnimFaceKeyframes() to monitor how many frames are currently in the
  // buffer and not call these DisplayFaceImage functions so frequently such that it grows too large,
  // otherwise there will be increasing lag in the stream.
  Result DisplayFaceImageBinary(const Vision::Image& img, u32 duration_ms, bool interruptRunning = false);
  Result DisplayFaceImage(const Vision::Image& img, u32 duration_ms, bool interruptRunning = false);
  Result DisplayFaceImage(const Vision::ImageRGB& img, u32 duration_ms, bool interruptRunning = false);
  Result DisplayFaceImage(const Vision::ImageRGB565& imgRGB565, u32 duration_ms, bool interruptRunning = false);
  // There is only one composite image in the animation process - duration is the amount of time the image will be displayed on screen
  // frameInterval_ms defines how often the composite images' GetFrame function should be called for internal sprite sequences
  Result DisplayFaceImage(const Vision::CompositeImage& compositeImage, 
                          u32 frameInterval_ms, u32 duration_ms, 
                          bool interruptRunning = false, bool emptySpriteBoxesAreValid = false);
  
  // Calling this function provides no gaurentee that the assets will actually be displayed
  // If a compositeFaceImage is currently displayed on the face all layers/image maps within
  // the compositeImage argument will be updated to their new values - set Count in the sprite map
  // for any sprite boxes that should no longer be displayed
  void UpdateCompositeImage(const Vision::CompositeImage& compositeImage, u32 applyAt_ms = 0);
  
  // Helper function that clears composite image layer - can be accomplished through UpdateCompositeImage
  // as well by specifying count values for sprite boxes/sprites if more nuance is required
  void ClearCompositeImageLayer(Vision::LayerName layerName, u32 applyAt_ms = 0);
  
  // Enable/Disable KeepFaceAlive
  // If enable == false, disableTimeout_ms is the duration over which the face should 
  // return to no adjustments
  Result EnableKeepFaceAlive(bool enable, u32 disableTimeout_ms = 0) const;

  // Restore all KeepFaceAlive parameters to defaults
  Result SetDefaultKeepFaceAliveParameters() const;
  
  // Set KeepFaceAliveParameterToDefault
  Result SetKeepFaceAliveParameterToDefault(KeepFaceAliveParameter param) const;
  
  // Set KeepFaceAlive parameter to specified value
  Result SetKeepFaceAliveParameter(KeepFaceAliveParameter param, f32 value) const;

  // Either start an eye shift or update an already existing eye shift with new params
  // Note: Eye shift will continue until removed so if eye shift with the same name
  // was already added without being removed, this will just update it
  Result AddOrUpdateEyeShift(const std::string& name, 
                             f32 xPix,
                             f32 yPix,
                             TimeStamp_t duration_ms,
                             f32 xMax = FACE_DISPLAY_HEIGHT,
                             f32 yMax = FACE_DISPLAY_WIDTH,
                             f32 lookUpMaxScale = 1.1f,
                             f32 lookDownMinScale = 0.85f,
                             f32 outerEyeScaleIncrease = 0.1f);
  
  // Removes eye shift layer by name
  // Does nothing if no such layer exists
  Result RemoveEyeShift(const std::string& name, u32 disableTimeout_ms = 0);

  // Returns true if an eye shift layer of the given name is currently applied
  bool IsEyeShifting(const std::string& name) const { return _activeEyeShiftLayers.count(name) > 0; }
  
  // Adds eye squinting layer with the given name
  Result AddSquint(const std::string& name, f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle);

  // Removes eye squinting layer by name
  // Does nothing if no such layer exists
  Result RemoveSquint(const std::string& name, u32 disableTimeout_ms = 0);

  // Returns true if an eye squint layer of the given name is currently applied
  bool IsEyeSquinting(const std::string& name) const { return _activeEyeSquintLayers.count(name) > 0; }

  // set saturation to a given level (default 1.0);
  Result SetFaceSaturation(float level);

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
  void HandleAnimAdded(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimationEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message);
  void HandleAnimState(const AnkiEvent<RobotInterface::RobotToEngine>& message);  

private:
  
  // Returns Tag if animation is playing.
  // Return NotAnimating otherwise.
  Tag IsAnimPlaying(const std::string& animName);
  
  Tag GetNextTag() { return ++_tagCtr; }

  template <typename MessageType, typename ImageType>
  Result DisplayFaceImageHelper(const ImageType& imgRGB565, u32 duration_ms, bool interruptRunning);

  void SetAnimationCallback(const std::string& animName,
                            AnimationCompleteCallback callback, 
                            const u32 currTag,
                            const u32 actionTag,
                            int numLoops,
                            float timeout_sec);
  
  static constexpr float _kDefaultTimeout_sec = 60.f;

  bool _isInitialized;
  Tag  _tagCtr;
  
  Robot* _robot = nullptr;
  DataAccessorComponent* _dataAccessor = nullptr;

  struct AnimationGroupWrapper{
    AnimationGroupWrapper(AnimationGroupContainer&  container)
    : _container(container){}
    AnimationGroupContainer&  _container;
  };
  std::unique_ptr<AnimationGroupWrapper> _animationGroups;
  
  // Map of available canned animations to associated metainfo
  std::unordered_map<std::string, AnimationMetaInfo> _availableAnims;
  
  bool _isDolingAnims;
  std::string _nextAnimToDole;
  
  std::string _currPlayingAnim;

  std::unordered_set<std::string> _activeEyeShiftLayers;
  std::unordered_set<std::string> _activeEyeSquintLayers;  
  
  u8 _lockedTracks;

  // For tracking whether or not an animation is playing based on
  // AnimStarted and AnimEnded messages
  bool          _isAnimating;
  std::string   _currAnimName;
  Tag           _currAnimTag;

  // Latest state message received from anim process
  AnimationState _animState;

  std::unique_ptr<Vision::RGB565ImageBuilder> _oledImageBuilder;

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

  int _compositeImageID;
  
};


} // namespace Cozmo
} // namespace Anki

#endif
