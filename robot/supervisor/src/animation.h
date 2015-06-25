/**
 * File: animation.h
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Defines an animation, which consists of a list of keyframes for each 
 *   animate-able subsystem or "track", to be played concurrently. Note that a
 *   track can have no keyframes (numFrames==0) and it will simply be ignored (and
 *   left unlocked) during that animation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COZMO_ROBOT_ANIMATION_H
#define ANKI_COZMO_ROBOT_ANIMATION_H

#include "keyFrame.h"
#include "anki/cozmo/shared/cozmoTypes.h"

namespace Anki {
namespace Cozmo {

//  class Animation
//  {
//  public:
//    
//    enum TrackType {
//      HEAD = 0,
//      LIFT,
//      POSE,
//      SOUND,
//      LIGHTS, // TODO: Separate eyes from other lights eventually?
//      SPECIAL,
//      NUM_TRACKS
//    };
//    
//    Animation();
//    
//    //
//    // Methods for controlling playback:
//    //
//    
//    // Called each time an animatin is about to be played
//    void Init();
//    
//    // Called while the animation is playing
//    void Update();
//    
//    // Stop the animation prematurely (called internally when an animation completes)
//    void Stop();
//    
//    // Query whether the animation is currently playing
//    bool IsPlaying() const;
//    
//    //
//    // Methods for defining animations:
//    //
//    
//    // True if any tracks have frames defined.
//    bool IsDefined();
//    void Clear();
//    void SetID(AnimationID_t newID);
//    Result AddKeyFrame(const KeyFrame& keyframe);
//    
//  private:
//    
//    static const s32 MAX_KEYFRAMES = 32; // total, shared b/w tracks
//    
//    AnimationID_t _ID; // needed, or simply the slot it's stored in?
//    
//    // what time the animation started, relative to which each keyFrame's time
//    // is specified
//    TimeStamp_t _startTime_ms;
//    
//    bool _isPlaying;
//
//    KeyFrame _frames[MAX_KEYFRAMES];
//    s32      _totalNumFrames;
//    bool     _framesSorted;
//    
//    // If not already sorted, groups keyframes by track in the _frames list, so
//    // that all keyframes of each track are contiguous in the list.
//    Result SortKeyFrames();
//
//    struct Track {
//      bool     isReady;   // true once first frame is "in position"
//      s32      startOffset;
//      s32      numFrames;
//      s32      currFrame;
//      
//      Track();
//    } _tracks[NUM_TRACKS];
//    
//    static TrackType GetTrack(const KeyFrame::Type kfType);
//    
//    // Stop the track (if it is not empty)
//    void StopTrack(TrackType whichTrack);
//    
//    bool _allTracksReady;
//    bool CheckTrackReadiness();
//    
//    void Lock(TrackType whichTrack);
//    void Unlock(TrackType whichTrack);
//    
//    /*
//     // Flag to return track to their original state when the animation is
//     // complete or is stopped/interrupted. Note that this does not apply to
//     // body pose, since there is no localization during animation.
//     bool _returnToOrigState;
//     
//     // Place to store original states for each subsystem, used when
//     // returnToOrigState is true.
//     // TODO: Could probably store original angle/height in u8 (don't need so much resolution)
//     f32       _origHeadAngle;
//     f32       _origLiftHeight;
//     u32       _origLEDcolors[HAL::NUM_LEDS];
//     SoundID_t _origSoundID;
//     */
//    
//  }; // class Animation
//  
//  
//  //
//  // Inline Implementations
//  //
//  
//  inline bool Animation::IsPlaying() const {
//    return _isPlaying;
//  }
//  
//  inline void Animation::SetID(AnimationID_t newID) {
//    _ID = newID;
//  }
  
} //  namespace Cozmo
} //  namespace Anki

#endif // ANKI_COZMO_ROBOT_ANIMATION_H

