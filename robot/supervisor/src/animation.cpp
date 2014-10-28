/**
 * File: animation.cpp
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

#include "animation.h"

#include "headController.h"
#include "liftController.h"

#include "anki/common/robot/utilities.h"

#define DEBUG_ANIMATIONS 0

namespace Anki {
namespace Cozmo {
  
  Animation::Animation()
  : _ID(0)
  , _startTime_ms(0)
  , _isPlaying(false)
  , _allTracksReady(false)
  {

  }
  
  
  Animation::Track::Track()
  : isReady(false)
  , numFrames(0)
  , currFrame(0)
  {
    
  }
  
  
  void Animation::Init()
  {
#   if DEBUG_ANIMATIONS
    PRINT("Init Animation %d\n", _ID);
#   endif
    
    _startTime_ms = HAL::GetTimeStamp();
    
    _allTracksReady = false;
    
    _isPlaying = true;
    
    /*
    _returnToOrigState = returnToOrigStateWhenDone;
    
    if(_returnToOrigState) {
      _origHeadAngle  = HeadController::GetAngleRad();
      _origLiftHeight = LiftController::GetHeightMM();
      
      // TODO: Need HAL::GetLED() method to store original LED state
     
       for(s32 iLED=0; iLED < HAL::NUM_LEDS; ++iLED) {
       origLEDcolors[iLED] = HAL::
       }
     
    }
     */
    
    // Reset each track and call TransitionInto for any initial keyframes
    // that have a relative start time of 0
    for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
      _tracks[iTrack].currFrame = 0;
      _tracks[iTrack].isReady   = false;
      
      if(_tracks[iTrack].numFrames > 0 &&
         _tracks[iTrack].frames[0].relTime_ms == 0)
      {
        _tracks[iTrack].frames[0].TransitionInto(_startTime_ms);
      }
    }
    
  } // Init()
  
  
  bool Animation::CheckTrackReadiness()
  {
    if(!_allTracksReady)
    {
      // Mark as true for now, and let the loop below unset it
      _allTracksReady = true;
      
      for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
        
        Animation::Track& track = _tracks[iTrack];
        
        // If this track isn't marked as ready yet, check it now
        if(!track.isReady) {
          if(track.numFrames == 0) {
            // Track with no keyframes is always ready
            track.isReady = true;
          } else if(track.frames[0].relTime_ms > 0) {
            // Tracks with first keyframe at time > 0 are always ready
            track.isReady = true;
          } else {
            // Check whether the first keyframe is "in position"
            track.isReady = track.frames[0].IsInPosition();
          }
          
          // In case this track just became ready:
          _allTracksReady &= track.isReady;
          
        } // if(!track.isReady)
        
      } // for each track
      
      if(_allTracksReady) {
        // If all track just became ready, update the start time for this
        // animation to now and call TransitionInto for keyframes that are
        // first for their track but do _not_ have relTime==0
        _startTime_ms = HAL::GetTimeStamp();
        
        for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack)
        {
          if(_tracks[iTrack].numFrames > 0 &&
             _tracks[iTrack].frames[0].relTime_ms > 0)
          {
            _tracks[iTrack].frames[0].TransitionInto(_startTime_ms);
          }
        } // for each track
        
      } // if(_allTracksReady)
      
    } // if (!_allTracksReady)
    
    return _allTracksReady;
      
  } // CheckTrackReadiness()
  
  
  void Animation::Update()
  {
    if(CheckTrackReadiness() == false) {
      // Wait for all tracks to be ready
      //PRINT("Waiting for all tracks to be ready to start animation...\n");
      return;
    }

    bool trackPlaying[NUM_TRACKS];
    
    for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack)
    {
      // Get a reference to the current track, for convenience
      Animation::Track& track = _tracks[iTrack];

      if(track.numFrames == 0 || track.currFrame >= track.numFrames) {
        // Skip empty or already-finished tracks
        trackPlaying[iTrack] = false;
        
      } else {
        // TODO: "Lock" controller for this track
        
        trackPlaying[iTrack] = true;
        
        // Get a reference to the current frame of the current track, for convenience
        KeyFrame& currKeyFrame = track.frames[track.currFrame];
        
        // Has the current keyframe's time now passed?
        const u32 absFrameTime_ms = currKeyFrame.relTime_ms + _startTime_ms;
        if(HAL::GetTimeStamp() >= absFrameTime_ms)
        {
          // We are transitioning out of this keyframe and into the next
          // one in the list (if there is one).
          currKeyFrame.TransitionOutOf(_startTime_ms);
          
          ++track.currFrame;
          
          if(track.currFrame < track.numFrames) {
#           if DEBUG_ANIMATIONS
            PRINT("Moving to keyframe %d of %d in track %d\n", track.currFrame+1, track.numFrames, iTrack);
#           endif
            KeyFrame& nextKeyFrame = track.frames[track.currFrame];
            nextKeyFrame.TransitionInto(_startTime_ms);
          } else {
#           if DEBUG_ANIMATIONS
            PRINT("Track %d finished all %d of its frames\n", iTrack, track.numFrames);
            trackPlaying[iTrack] = false;
#           endif
          }
          
        } // if GetTimeStamp() >= absFrameTime_ms
      } // if/else track.numFrames == 0
      
    } // for each track
    
    // isPlaying should be true after this loop if any of the tracks are still
    // playing. False otherwise.
    _isPlaying = false;
    for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
      _isPlaying |= trackPlaying[iTrack];
    }
    
    if(!_isPlaying) {
#if   DEBUG_ANIMATIONS
      PRINT("No tracks in animation %d still playing. Stopping.\n", _ID);
#     endif
      Stop();
    }
    
  } // Update
  
  
  void Animation::Stop()
  {
    /*
    if(_returnToOrigState) {
      HeadController::SetAngleRad(_origHeadAngle);
      LiftController::SetDesiredHeight(_origLiftHeight);
    }
     */
    
    for(s32 iTrack = 0; iTrack < NUM_TRACKS; ++iTrack)
    {
      _tracks[iTrack].frames[_tracks[iTrack].currFrame].Stop();
    }
    
    _isPlaying = false;
    
#   if DEBUG_ANIMATIONS
    PRINT("Stopped playing animation %d.\n", _ID);
#   endif
  }
  
  
  bool Animation::IsDefined()
  {
    for(s32 iTrack = 0; iTrack < NUM_TRACKS; ++iTrack)
    {
      if(_tracks[iTrack].numFrames > 0) {
        return true;
      }
    }
    
    return false;
  }
  
  
  void Animation::Clear()
  {
    for(s32 iTrack = 0; iTrack < NUM_TRACKS; ++iTrack)
    {
      // TODO: Make a Clear() method in KeyFrame and call that here:
      _tracks[iTrack].currFrame = 0;
      _tracks[iTrack].numFrames = 0;
    }
  }
  
  Animation::TrackType Animation::GetTrack(const KeyFrame::Type kfType)
  {
    switch(kfType)
    {
      case KeyFrame::HEAD_ANGLE:
      case KeyFrame::START_HEAD_NOD:
      case KeyFrame::STOP_HEAD_NOD:
        return HEAD;
        
      case KeyFrame::LIFT_HEIGHT:
      case KeyFrame::START_LIFT_NOD:
      case KeyFrame::STOP_LIFT_NOD:
        return LIFT;
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      case KeyFrame::DRIVE_ARC:
      case KeyFrame::BACK_AND_FORTH:
      case KeyFrame::START_WIGGLE:
      case KeyFrame::POINT_TURN:
      //case KeyFrame::STOP_WIGGLE:
        return POSE;
        
      case KeyFrame::SET_LED_COLORS:
      case KeyFrame::BLINK_EYES:
      case KeyFrame::SET_EYE:
      case KeyFrame::FLASH_EYES:
      case KeyFrame::SPIN_EYES:
        return LIGHTS;
        
      case KeyFrame::PLAY_SOUND:
        return SOUND;
        
      default:
        PRINT("Unknown track for KeyFrame type %d.\n", kfType);
        return NUM_TRACKS;
        
    } // switch(kfType)
    
  } // GetTrack()
  
  
  
  Result Animation::AddKeyFrame(const KeyFrame& keyframe)
  {
    const TrackType whichTrack = Animation::GetTrack(keyframe.type);
    
    Animation::Track& track = _tracks[whichTrack];
    
    AnkiConditionalErrorAndReturnValue(track.numFrames < Animation::MAX_KEYFRAMES,
                                       RESULT_FAIL,
                                       "Animation.AddKeyFrame.TooManyFrames",
                                       "No more space to add given keyframe to specified animation track.\n");
    
    track.frames[track.numFrames++] = keyframe;
    
#   if DEBUG_ANIMATIONS
    PRINT("Added frame %d to track %d of animation %d\n",
          track.numFrames, whichTrack, _ID);
#   endif
    
    return RESULT_OK;
  }

  
} // namespace Cozmo
} // namespace Anki

