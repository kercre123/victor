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

#include "eyeController.h"
#include "headController.h"
#include "liftController.h"
#include "steeringController.h"

#include "anki/common/robot/utilities.h"

#define DEBUG_ANIMATIONS 0

#define ENABLE_LOCK_AND_UNLOCK 0

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
    
    SortKeyFrames();
    
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
      
      if(_tracks[iTrack].numFrames > 0) {
        
        Lock(static_cast<TrackType>(iTrack));
        
        if(_frames[_tracks[iTrack].startOffset].relTime_ms == 0)
        {
          _frames[_tracks[iTrack].startOffset].TransitionInto(_startTime_ms);
        }
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
          } else if(_frames[track.startOffset].relTime_ms > 0) {
            // Tracks with first keyframe at time > 0 are always ready
            track.isReady = true;
          } else {
            // Check whether the first keyframe is "in position"
            track.isReady = _frames[track.startOffset].IsInPosition();
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
             _frames[_tracks[iTrack].startOffset].relTime_ms > 0)
          {
            Unlock(static_cast<TrackType>(iTrack));
            _frames[_tracks[iTrack].startOffset].TransitionInto(_startTime_ms);
            Lock(static_cast<TrackType>(iTrack));
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
        KeyFrame& currKeyFrame = _frames[track.startOffset + track.currFrame];
        
        // Has the current keyframe's time now passed?
        const u32 absFrameTime_ms = currKeyFrame.relTime_ms + _startTime_ms;
        if(HAL::GetTimeStamp() >= absFrameTime_ms)
        {
          const u8 nextTransitionIn = (track.currFrame+1 < track.numFrames ?
                                       _frames[track.startOffset + track.currFrame + 1].transitionIn : 0);
          
          // We are transitioning out of this keyframe and into the next
          // one in the list (if there is one).
          Unlock(static_cast<TrackType>(iTrack));
          currKeyFrame.TransitionOutOf(_startTime_ms, nextTransitionIn);
          Lock(static_cast<TrackType>(iTrack));
          
          ++track.currFrame;
          
          if(track.currFrame < track.numFrames) {
#           if DEBUG_ANIMATIONS
            PRINT("Moving to keyframe %d of %d in track %d\n", track.currFrame+1, track.numFrames, iTrack);
#           endif
            KeyFrame& nextKeyFrame = _frames[track.startOffset + track.currFrame];
            Unlock(static_cast<TrackType>(iTrack));
            nextKeyFrame.TransitionInto(_startTime_ms);
            Lock(static_cast<TrackType>(iTrack));
          } else {
#           if DEBUG_ANIMATIONS
            PRINT("Track %d finished all %d of its frames\n", iTrack, track.numFrames);
#           endif
            trackPlaying[iTrack] = false;
          }
          
        } // if GetTimeStamp() >= absFrameTime_ms
      } // if/else track.numFrames == 0
      
    } // for each track
    
    bool wasPlaying = _isPlaying;
    
    // isPlaying should be true after this loop if any of the tracks are still
    // playing. False otherwise.
    _isPlaying = false;
    for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
      _isPlaying |= trackPlaying[iTrack];
    }
    
    if(wasPlaying && !_isPlaying) {
#if   DEBUG_ANIMATIONS
      PRINT("No tracks in animation %d still playing. Stopping.\n", _ID);
#     endif
      Stop();
    }
    
  } // Update
  
  
  void Animation::Stop()
  {
    // Stop any subsystems this animation was using:
    for(s32 iTrack=0; iTrack < NUM_TRACKS; ++iTrack) {
      StopTrack(static_cast<TrackType>(iTrack));
    }
    
    _isPlaying = false;
    
#   if DEBUG_ANIMATIONS
    PRINT("Stopped playing animation %d.\n", _ID);
#   endif
  }
  
  
  bool Animation::IsDefined()
  {
    return _totalNumFrames > 0;
  }
  
  
  void Animation::Clear()
  {
    if(_isPlaying) {
      Stop();
    }
    
    _totalNumFrames = 0;
    _framesSorted = false;
    
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
      case KeyFrame::HOLD_HEAD_ANGLE:
      case KeyFrame::START_HEAD_NOD:
      case KeyFrame::STOP_HEAD_NOD:
        return HEAD;
        
      case KeyFrame::LIFT_HEIGHT:
      case KeyFrame::HOLD_LIFT_HEIGHT:
      case KeyFrame::START_LIFT_NOD:
      case KeyFrame::STOP_LIFT_NOD:
        return LIFT;
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      case KeyFrame::DRIVE_ARC:
      case KeyFrame::BACK_AND_FORTH:
      case KeyFrame::START_WIGGLE:
      case KeyFrame::POINT_TURN:
      case KeyFrame::HOLD_POSE:
      //case KeyFrame::STOP_WIGGLE:
        return POSE;
        
      case KeyFrame::BLINK_EYES:
      case KeyFrame::SET_EYE:
      case KeyFrame::FLASH_EYES:
      case KeyFrame::SPIN_EYES:
      case KeyFrame::STOP_EYES:
        return LIGHTS;
        
      case KeyFrame::PLAY_SOUND:
      case KeyFrame::WAIT_FOR_SOUND:
      case KeyFrame::STOP_SOUND:
        return SOUND;
        
      case KeyFrame::TRIGGER_ANIMATION:
        return SPECIAL;
        
      default:
        PRINT("Unknown track for KeyFrame type %d.\n", kfType);
        return NUM_TRACKS;
        
    } // switch(kfType)
    
  } // GetTrack()
  
  
  
  Result Animation::AddKeyFrame(const KeyFrame& keyframe)
  {
    AnkiConditionalErrorAndReturnValue(_totalNumFrames < Animation::MAX_KEYFRAMES,
                                       RESULT_FAIL,
                                       "Animation.AddKeyFrame.TooManyFrames",
                                       "No more space to add given keyframe to specified animation.\n");

    
    _frames[_totalNumFrames++] = keyframe;
    _framesSorted = false; // trigger a re-sort on next Init() now that we've got a new frame
    
#   if DEBUG_ANIMATIONS
    PRINT("Added frame %d to animation %d\n",
          _totalNumFrames, _ID);
#   endif
    
    /*
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
    */
    
    return RESULT_OK;
  }
  
  Result Animation::SortKeyFrames()
  {
    if(!_framesSorted)
    {
      KeyFrame buffer[MAX_KEYFRAMES];
      
      // Copy the frames into the temp buffer, and then copy them back into the
      // _frames array, in order. The algorithm below is certainly not optimal
      // in terms of efficiency (we loop over the frames list NUM_TRACKS times,
      // re-checking the type of every frame every time. But given the small
      // number of tracks (5) and number of frames (~20-30), this seems like
      // it's probably good enough for now.
  
      memcpy(buffer, _frames, _totalNumFrames*sizeof(KeyFrame));
      
#     if DEBUG_ANIMATIONS
      bool sanityCheck[MAX_KEYFRAMES];
      memset(sanityCheck, 0, sizeof(bool)*MAX_KEYFRAMES);
#     endif
      
      s32 curStartOffset = 0;
      for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
        _tracks[iTrack].startOffset = curStartOffset;
        
        const TrackType curTrackType = static_cast<TrackType>(iTrack);
        for(s32 iFrame=0; iFrame<_totalNumFrames; ++iFrame) {
          // If this frame should be in the current track
          if(Animation::GetTrack(buffer[iFrame].type) == curTrackType) {
            _frames[_tracks[iTrack].startOffset + _tracks[iTrack].numFrames++] = buffer[iFrame];
            ++curStartOffset;
            
#           if DEBUG_ANIMATIONS
            // Make sure each frame is used only once
            AnkiConditionalWarn(!sanityCheck[iFrame],
                                "Animation.SortKeyFrames.ReusingFrame",
                                "Frame %d is being added to multiple tracks.\n", iFrame);
            sanityCheck[iFrame] = true;
#           endif
          }
        } // for each frame
        
      } // for each track
      
      _framesSorted = true;
      
#     if DEBUG_ANIMATIONS
      for(s32 iTrack=0; iTrack<NUM_TRACKS; ++iTrack) {
        PRINT("Track %d of animation %d has %d frames starting at offset %d.\n",
              iTrack, _ID, _tracks[iTrack].numFrames, _tracks[iTrack].startOffset);
      }
#     endif
      
    } // if(!_framesSorted)
    
    return RESULT_OK;
  }
  
  void Animation::Lock(TrackType whichTrack)
  {
#   if ENABLE_LOCK_AND_UNLOCK
    switch(whichTrack)
    {
      case HEAD:
        HeadController::Disable();
        break;
        
      case LIFT:
        LiftController::Disable();
        break;
        
      case LIGHTS:
        EyeController::Disable();
        break;
        
      default:
        // Nothing to do?
        break;
    }
#   endif
  } // Lock()

  
  void Animation::Unlock(TrackType whichTrack)
  {
#   if ENABLE_LOCK_AND_UNLOCK
    switch(whichTrack)
    {
      case HEAD:
        HeadController::Enable();
        break;
        
      case LIFT:
        LiftController::Enable();
        break;
        
      case LIGHTS:
        EyeController::Enable();
        break;
        
      default:
        // Nothing to do?
        break;
    }
#   endif
  } // Unlock()

  
  void Animation::StopTrack(TrackType whichTrack)
  {
    if(_tracks[whichTrack].numFrames > 0)
    {
      Unlock(whichTrack);
      
      switch(whichTrack)
      {
        case HEAD:
          HeadController::Stop();
          break;
          
        case LIFT:
          LiftController::Stop();
          break;
          
        case LIGHTS:
          //EyeController::StopAnimating();
          break;
          
        case POSE:
          SteeringController::ExecuteDirectDrive(0, 0);
          break;
          
        case SOUND:
          Messages::StopSoundOnBaseStation msg;
          HAL::RadioSendMessage(GET_MESSAGE_ID(Messages::StopSoundOnBaseStation), &msg);
          break;
          
        case SPECIAL:
          // Nothing to do here
          break;
          
        default:
          AnkiError("Animation.StopTrack.UnknownTrack", "Asked to stop unknkown track %d.\n", whichTrack);
          break;
      } // switch(whichTrack)
    } // if track not empty
  } // StopTrack()

  
} // namespace Cozmo
} // namespace Anki

