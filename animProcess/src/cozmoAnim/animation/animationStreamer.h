/**
 * File: animationStreamer.h
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description: 
 * 
 *   Handles streaming a given animation from a CannedAnimationContainer
 *   to a robot.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_AnimationStreamer_H__
#define __Anki_Cozmo_AnimationStreamer_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/engine/image.h"
#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/baseTypes/track.h"
#include "clad/types/keepFaceAliveParameters.h"

#include <list>
#include <memory>

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class ProceduralFace;
  class AnimContext;
  class TrackLayerComponent;
  
  namespace Audio {
    class AnimationAudioClient;
    class ProceduralAudioClient;
  }
  
  
  class AnimationStreamer
  {
  public:
    
    using Tag = AnimationTag;
    using FaceTrack = Animations::Track<ProceduralFaceKeyFrame>;
    
    // TODO: This could be removed in favor of just referring to ::Anki::Cozmo, but avoiding touching too much code now.
    static const Tag kNotAnimatingTag = ::Anki::Cozmo::kNotAnimatingTag;
    
    AnimationStreamer(const AnimContext* context);
    
    ~AnimationStreamer();
    
    Result Init();
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    //
    // If interruptRunning == true, any currently-streaming animation will be aborted.
    // Actual streaming occurs on calls to Update().
    // 
    // If name == "" or anim == nullptr, it is equivalent to calling Abort()
    // if there is an animation currently playing, or no-op if there's no
    // animation playing
    Result SetStreamingAnimation(const std::string& name,
                                 Tag tag,
                                 u32 numLoops = 1,
                                 bool interruptRunning = true);
    
    Result SetProceduralFace(const ProceduralFace& face, u32 duration_ms);    

    void Process_displayFaceImageChunk(const RobotInterface::DisplayFaceImageBinaryChunk& msg);
    void Process_displayFaceImageChunk(const RobotInterface::DisplayFaceImageGrayscaleChunk& msg);
    void Process_displayFaceImageChunk(const RobotInterface::DisplayFaceImageRGBChunk& msg);

    Result SetFaceImage(const Vision::Image& img, u32 duration_ms);
    Result SetFaceImage(const Vision::ImageRGB565& img, u32 duration_ms);
    
    Audio::ProceduralAudioClient* GetProceduralAudioClient() const { return _proceduralAudioClient.get(); }
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update();

    // Stop currently running animation
    void Abort();
    
    const std::string GetStreamingAnimationName() const;
    const Animation* GetStreamingAnimation() const { return _streamingAnimation; }

    void EnableKeepFaceAlive(bool enable, u32 disableTimeout_ms);
    
    void SetDefaultKeepFaceAliveParams();
    void SetParamToDefault(KeepFaceAliveParameter whichParam);
    void SetParam(KeepFaceAliveParameter whichParam, float newValue);
    
    // Set/Reset the amount of time to wait before forcing KeepFaceAlive() after the last stream has stopped
    void SetKeepFaceAliveLastStreamTimeout(const f32 time_s)
      { _longEnoughSinceLastStreamTimeout_s = time_s; }
    void ResetKeepFaceAliveLastStreamTimeout();
    
    TrackLayerComponent* GetTrackLayerComponent() { return _trackLayerComponent.get(); }
    const TrackLayerComponent* GetTrackLayerComponent() const { return _trackLayerComponent.get(); }

    // Sets all tracks that should be locked
    void SetLockedTracks(u8 whichTracks)   { _lockedTracks = whichTracks; }
    bool IsTrackLocked(u8 trackFlag) const { return ((_lockedTracks & trackFlag) == trackFlag); }

    // Lock or unlock an individual track
    void LockTrack(AnimTrackFlag track) { _lockedTracks |= (u8)track; }
    void UnlockTrack(AnimTrackFlag track) { _lockedTracks &= ~(u8)track; }

    void DrawToFace(const Vision::ImageRGB& img, Array2d<u16>& img565_out);
    
    // Whether or not to redirect a face image to the FaceInfoScreenManager
    // for display on a debug screen
    void RedirectFaceImagesToDebugScreen(bool redirect) { _redirectFaceImagesToDebugScreen = redirect; }

  private:
    
    Result SetStreamingAnimation(Animation* anim,
                                 Tag tag,
                                 u32 numLoops = 1,
                                 bool interruptRunning = true,
                                 bool isInternalAnim = true);

    // Initialize the streaming of an animation with a given tag
    // (This will call anim->Init())
    Result InitStream(Animation* anim, Tag withTag);
    
    // Actually stream the animation (called each tick)
    Result UpdateStream(Animation* anim, bool storeFace);
    
    // This performs the test cases for the animation while loop
    bool ShouldProcessAnimationFrame( Animation* anim, TimeStamp_t startTime_ms, TimeStamp_t streamingTime_ms );
    
    // Sends the start of animation message to engine
    Result SendStartOfAnimation();

    // Sends the end of animation message to engine if the
    // number of commanded loops of the animation has completed.
    // If abortingAnim == true, then the message is sent even if all loops were not completed.
    Result SendEndOfAnimation(bool abortingAnim = false);
    
    // Enables/Disables the backpack lights animation layer on the robot
    // if it hasn't already been enabled/disabled
    Result EnableBackpackAnimationLayer(bool enable);
    
    // Check whether the animation is done
    bool IsFinished(Animation* anim) const;
    
    void StopTracks(const u8 whichTracks);
    
    void StopTracksInUse() {
      // In case we are aborting an animation, stop any tracks that were in use
      // (For now, this just means motor-based tracks.) Note that we don't
      // stop tracks we weren't using, in case we were, for example, playing
      // a head animation while driving a path.
      StopTracks(_tracksInUse);
    }
    
    template<typename ImageType>
    Result SetFaceImageHelper(const ImageType& img, const u32 duration_ms);
    
    // pass the started/stopped animation name to webviz
    void SendAnimationToWebViz( bool starting ) const;
    
    const AnimContext* _context = nullptr;
    
    Animation*  _streamingAnimation = nullptr;
    Animation*  _neutralFaceAnimation = nullptr;
    Animation*  _proceduralAnimation = nullptr; // for creating animations "live" or dynamically

    std::unique_ptr<TrackLayerComponent>  _trackLayerComponent;
    
    void BufferFaceToSend(const ProceduralFace& procFace);
    void BufferFaceToSend(Vision::ImageRGB565& image);

    void UpdateCaptureFace(Vision::ImageRGB565& faceImg565);

    // Used to stream _just_ the stuff left in the various layers (all procedural stuff)
    Result StreamLayers();
    
    u32 _numLoops = 1;
    u32 _loopCtr  = 0;

    // Start and end messages sent to engine
    bool _startOfAnimationSent = false;
    bool _endOfAnimationSent   = false;
    
    bool _wasAnimationInterruptedWithNothing = false;
    
    bool _backpackAnimationLayerEnabled = false;

    // Whether or not the streaming animation was commanded internally
    // from within this class (as opposed to by an engine message)
    bool _playingInternalAnim = false;
    
    // When this animation started playing (was initialized) in milliseconds, in
    // "real" basestation time
    TimeStamp_t _startTime_ms;
    
    // Where we are in the animation in terms of what has been streamed out, since
    // we don't stream in real time. Each time we send an audio frame to the
    // robot (silence or actual audio), this increments by one audio sample
    // length, since that's what keeps time for streaming animations (not a
    // clock)
    TimeStamp_t _streamingTime_ms;
    
    // Time when procedural face layer can next be applied.
    // There's a minimum amount of time that must pass since the last
    // non-procedural face (which has higher priority) was drawn in order
    // to smooth over gaps in between non-procedural frames that can occur
    // when trying to render them at near real-time. Otherwise, procedural
    // face layers like eye darts could play during these gaps.
    TimeStamp_t _nextProceduralFaceAllowedTime_ms = 0;
    
    // Last time we streamed anything
    f32 _lastStreamTime = std::numeric_limits<f32>::lowest();

#   define PLAY_ROBOT_AUDIO_ON_DEVICE 0
#   if PLAY_ROBOT_AUDIO_ON_DEVICE
    // TODO: Remove these once we aren't playing robot audio on the device
    TimeStamp_t _playedRobotAudio_ms;
    std::deque<const RobotAudioKeyFrame*> _onDeviceRobotAudioKeyFrameQueue;
    const RobotAudioKeyFrame* _lastPlayedOnDeviceRobotAudioKeyFrame;
#   endif
    
    // Sends msg to appropriate destination as long as the specified track is unlocked
    bool SendIfTrackUnlocked(RobotInterface::EngineToRobot* msg, AnimTrackFlag track);
    
    Tag _tag;
    
    // For track locking
    u8 _lockedTracks;
    
    // Which tracks are currently playing
    u8 _tracksInUse;
    
    // For keep face alive animations
    std::map<KeepFaceAliveParameter, f32> _keepFaceAliveParams;
    
    std::unique_ptr<Audio::AnimationAudioClient> _animAudioClient;
    std::unique_ptr<Audio::ProceduralAudioClient> _proceduralAudioClient;
    
    // Time to wait before forcing KeepFaceAlive() after the latest stream has stopped
    f32 _longEnoughSinceLastStreamTimeout_s;

    // Image buffer that is fed directly to face display (in RGB565 format)
    Vision::ImageRGB565 _faceDrawBuf;

    // Image buffer for ProceduralFace
    Vision::ImageRGB _procFaceImg;

    // Storage and chunk tracking for faceImage data received from engine
    
    // Image used for both binary and grayscale images
    Vision::Image    _faceImageGrayscale;
    
    // Binary images
    u32              _faceImageId                       = 0;          // Used only for tracking chunks of the same image as they are received
    u8               _faceImageChunksReceivedBitMask    = 0;
    const u8         kAllFaceImageChunksReceivedMask    = 0x3;        // 2 bits for 2 expected chunks

    // Grayscale images
    u32                 _faceImageGrayscaleId                    = 0;      // Used only for tracking chunks of the same image as they are received
    u32                 _faceImageGrayscaleChunksReceivedBitMask = 0;
    const u32           kAllFaceImageGrayscaleChunksReceivedMask = 0x7fff; // 15 bits for 15 expected chunks (FACE_DISPLAY_NUM_PIXELS / 1200 pixels_per_msg ~= 15)
    
    // RGB images
    Vision::ImageRGB565 _faceImageRGB565;
    u32                 _faceImageRGBId                    = 0;          // Used only for tracking chunks of the same image as they are received
    u32                 _faceImageRGBChunksReceivedBitMask = 0;
    const u32           kAllFaceImageRGBChunksReceivedMask = 0x3fffffff; // 30 bits for 30 expected chunks (FACE_DISPLAY_NUM_PIXELS / 600 pixels_per_msg ~= 30)
        
    // Tic counter for sending animState message
    u32           _numTicsToSendAnimState            = 0;

    bool _redirectFaceImagesToDebugScreen = false;

  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_AnimationStreamer_H__ */
