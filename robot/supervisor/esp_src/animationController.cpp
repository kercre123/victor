#include "animationController.h"
#include "clad/types/animationKeyFrames.h"
#include "anki/cozmo/robot/esp.h"
#include "rtip.h"
#include "face.h"
extern "C" {
#include "foregroundTask.h"
}

#define DEBUG_ANIMATION_CONTROLLER 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    
    // Streamed animation will not play until we've got this many _audio_ keyframes
    // buffered.
    static const s32 ANIMATION_PREROLL_LENGTH = 7;
    
    // Circular byte buffer for keyframe messages
    u8 _keyFrameBuffer[KEYFRAME_BUFFER_SIZE];
    s32 _currentBufferPos;
    s32 _lastBufferPos;
    
    s32 _numAudioFramesBuffered; // NOTE: Also counts EndOfAnimationFrames...
    s32 _numBytesPlayed = 0;

    u8  _currentTag = 0;
    
    bool _isBufferStarved;
    bool _haveReceivedTerminationFrame;
    bool _isPlaying;
    bool _bufferFullMessagePrintedThisTick;
    
    s32 _tracksToPlay;
    
    int _tracksInUse = 0;
    
    s16 _audioReadInd;
    s16 _screenReadInd;
    bool _playSilence;
    
    
#   if DEBUG_ANIMATION_CONTROLLER
    TimeStamp_t _currentTime_ms;
#   endif

  } // "private" members
    
  Result Init()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Initializing AnimationController\n");
#   endif
    
    _tracksToPlay = ENABLE_ALL_TRACKS;
    
    _tracksInUse  = 0;
        
    Clear();
    
    return RESULT_OK;
  }
  
  static s32 GetNumBytesAvailable()
  {
    if(_lastBufferPos >= _currentBufferPos) {
      return sizeof(_keyFrameBuffer) - (_lastBufferPos - _currentBufferPos);
    } else {
      return _currentBufferPos - _lastBufferPos;
    }
  }
  
  static s32 GetNumBytesInBuffer()
  {
    if(_lastBufferPos >= _currentBufferPos) {
      return (_lastBufferPos - _currentBufferPos);
    } else {
      return sizeof(_keyFrameBuffer) - (_currentBufferPos - _lastBufferPos);
    }
  }
  
  s32 GetTotalNumBytesPlayed() {
    return _numBytesPlayed;
  }
  
  void ClearNumBytesPlayed() {
    _numBytesPlayed = 0;
  }
  
  void Clear()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Clearing AnimationController\n");
#   endif
    
    _numBytesPlayed += GetNumBytesInBuffer();
    //PRINT("CLEAR NumBytesPlayed %d (%d)\n", _numBytesPlayed, GetNumBytesInBuffer());
    
    _currentBufferPos = 0;
    _lastBufferPos = 0;
    _currentTag = 0;
    
    _numAudioFramesBuffered = 0;

    _haveReceivedTerminationFrame = false;
    _isPlaying = false;
    _isBufferStarved = false;
    _bufferFullMessagePrintedThisTick = false;

    _audioReadInd  = 0;
    _screenReadInd = 0;
    
    if(_tracksInUse) {
      // In case we are aborting an animation, stop any tracks that were in use
      // (For now, this just means motor-based tracks.) Note that we don't
      // stop tracks we weren't using, in case we were, for example, playing
      // a head animation while driving a path.
      if(_tracksInUse & HEAD_TRACK) {
        // Don't have an API for this on Espressif right now, PUNT
        //HeadController::SetAngularVelocity(0);
      }
      if(_tracksInUse & LIFT_TRACK) {
        // DOn't have an API for this on the Espressif right now, PUNT
        //LiftController::SetAngularVelocity(0);
      }
      if(_tracksInUse & BODY_TRACK) {
        RobotInterface::EngineToRobot msg;
        msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
        msg.animBodyMotion.speed = 0;
        msg.animBodyMotion.curvatureRadius_mm = 0;
        RTIP::SendMessage(msg);
      }
    }
      
    _tracksInUse = 0;
    
#   if DEBUG_ANIMATION_CONTROLLER
    _currentTime_ms = 0;
#   endif
  }

  static inline RobotInterface::EngineToRobot::Tag PeekBufferTag()
  {
    return _keyFrameBuffer[_currentBufferPos];
  }
  
  static u32 GetFromBuffer(u8* data, u32 numBytes)
  {
    if(_currentBufferPos + numBytes < sizeof(_keyFrameBuffer)) {
      // There's enough room from current position to end of buffer to just
      // copy directly
      memcpy(data, _keyFrameBuffer + _currentBufferPos, numBytes);
      _currentBufferPos += numBytes;
    } else {
      // Copy the first chunk from whatever remains from current position to end of
      // the buffer
      const s32 firstChunk = sizeof(_keyFrameBuffer) - _currentBufferPos;
      memcpy(data, _keyFrameBuffer + _currentBufferPos, firstChunk);
      
      // Copy the remaining data starting at the beginning of the buffer
      memcpy(data+firstChunk, _keyFrameBuffer, numBytes - firstChunk);
      _currentBufferPos = numBytes-firstChunk;
    }
    
    // Increment total number of bytes played since startup
    _numBytesPlayed += numBytes;
    //PRINT("NumBytesPlayed %d (%d) (%d)\n", _numBytesPlayed, numBytes, *((u32*)data));
        
    return numBytes;
  }
  
  static s32 GetFromBuffer(RobotInterface::EngineToRobot* msg)
  {
    s32 readSoFar;
    memset(msg, 0, sizeof(RobotInterface::EngineToRobot)); // Memset 0, presumably sets all size fields to 0
    readSoFar  = GetFromBuffer(msg->GetBuffer(), RobotInterface::EngineToRobot::MIN_SIZE); // Read in enough to know what it is
    readSoFar += GetFromBuffer(msg->GetBuffer() + readSoFar, msg->Size() - readSoFar); // Read in the minimum size for the type to get length fields
    readSoFar += GetFromBuffer(msg->GetBuffer() + readSoFar, msg->Size() - readSoFar); // Read in anything left now that we know how big minimum fields are
    return readSoFar;
  }
  
  Result BufferKeyFrame(const RobotInterface::EngineToRobot& msg)
  {
    const s32 numBytesAvailable = GetNumBytesAvailable();
    const s32 numBytesNeeded = msg.Size();
    u32 numBytes;
    if(numBytesAvailable < numBytesNeeded) {
      // Only print the error message if we haven't already done so this tick,
      // to prevent spamming that could clog reliable UDP
      if(!_bufferFullMessagePrintedThisTick) {
        PRINT("ERROR AnimationController.BufferKeyFrame.BufferFull",
                  "%d bytes available, %d needed.\n",
                  numBytesAvailable, numBytesNeeded);
        _bufferFullMessagePrintedThisTick = true;
      }
      return RESULT_FAIL;
    }
    
    numBytes = msg.Size();

    assert(numBytes < sizeof(_keyFrameBuffer));
    
    if(_lastBufferPos + numBytes < sizeof(_keyFrameBuffer)) {
      // There's enough room from current end position to end of buffer to just
      // copy directly
      memcpy(_keyFrameBuffer + _lastBufferPos, msg.GetBuffer(), numBytes);
      _lastBufferPos += numBytes;
    } else {
      // Copy the first chunk into whatever fits from current position to end of
      // the buffer
      const s32 firstChunk = sizeof(_keyFrameBuffer) - _lastBufferPos;
      memcpy(_keyFrameBuffer + _lastBufferPos, msg.GetBuffer(), firstChunk);
      
      // Copy the remaining data starting at the beginning of the buffer
      memcpy(_keyFrameBuffer, msg.GetBuffer()+firstChunk, numBytes - firstChunk);
      _lastBufferPos = numBytes-firstChunk;
     }
    switch(msg.tag) {
      case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
        _haveReceivedTerminationFrame = true;
      case RobotInterface::EngineToRobot::Tag_animAudioSample:
      case RobotInterface::EngineToRobot::Tag_animAudioSilence:
        ++_numAudioFramesBuffered;
        break;
      default:
        break;
    }
    
    assert(_lastBufferPos >= 0 && _lastBufferPos < (s32)sizeof(_keyFrameBuffer));
    
    return RESULT_OK;
  }
    
  bool IsBufferFull()
  {
    return GetNumBytesAvailable() > 0;
  }
  
  bool IsPlaying()
  {
    return _isPlaying;
  }
  
  static inline bool IsReadyToPlay()
  {
    
    bool ready = false;
    
    if(_isPlaying) {
      // If we are already in progress playing something, we are "ready to play"
      // until we run out of keyframes in the buffer
      // Note that we need at least two "frames" in the buffer so we can always
      // read from the current one to the next one without reaching end of buffer.
      ready = _numAudioFramesBuffered > 1;

      // Report every time the buffer goes from having a sufficient number of audio frames to not.
      if (!ready) {
        if (!_isBufferStarved) {
          _isBufferStarved = true;
          PRINT("AnimationController.IsReadyToPlay.BufferStarved\n");
        }
      } else {
        _isBufferStarved = false;
      }
      
    } else {
      // Otherwise, wait until we get enough frames to start
      ready = (_numAudioFramesBuffered > ANIMATION_PREROLL_LENGTH || _haveReceivedTerminationFrame);
      if(ready) {
        _isPlaying = true;
        _isBufferStarved = false;
        
#       if DEBUG_ANIMATION_CONTROLLER
        _currentTime_ms = 0;
#       endif
      }
    }
    
    //assert(_currentFrame <= _lastFrame);
    
    return ready;
  } // IsReadyToPlay()
  
  extern "C" u32 PumpAudioData()
  {
    if (IsReadyToPlay())
    {
      if (_audioReadInd == 0)
      {
        // Next thing in the buffer should be audio or silence:
        RobotInterface::EngineToRobot::Tag msgID = PeekBufferTag();
        RobotInterface::EngineToRobot msg;
        
        // If the next message is not audio, then delete it until it is.
        while(msgID != RobotInterface::EngineToRobot::Tag_animAudioSilence &&
              msgID != RobotInterface::EngineToRobot::Tag_animAudioSample) {
          PRINT("Expecting either audio sample or silence next in animation buffer. (Got %d instead). Dumping message. (FYI AudioSample_ID = %d)\n", msgID, RobotInterface::EngineToRobot::Tag_animAudioSample);
          GetFromBuffer(&msg);
          msgID = PeekBufferTag();
        }
        
        if (msgID == RobotInterface::EngineToRobot::Tag_animAudioSilence)
        {
          GetFromBuffer(&msg);
          _playSilence = true;
          _audioReadInd = AUDIO_BYTES_PER_DROP;
          return 0;
        }
        else if (msgID == RobotInterface::EngineToRobot::Tag_animAudioSample)
        {
          u32 ret;
          GetFromBuffer((u8*)&ret, 1); // Get the audio sample header out
          _playSilence = false;
          GetFromBuffer((u8*)&ret, AUDIO_BYTES_PER_DROP); // Get the first AUDIO_BYTES_PER_DROP from the buffer
          _audioReadInd = AUDIO_BYTES_PER_DROP;
          return ret;
        }
        else
        {
          PRINT("FAIL: Expecting either audio sample or silence next in animation buffer. (Got %d instead)\n", msgID);
          return 0;
        }
      }
      else
      {
        u32 ret = 0;
        if (!_playSilence) GetFromBuffer((u8*)&ret, AUDIO_BYTES_PER_DROP); // Get the next AUDIO_BYTES_PER_DROP from the buffer
        _audioReadInd += AUDIO_BYTES_PER_DROP;
        if (_audioReadInd >= AUDIO_SAMPLE_SIZE)
        {
          _audioReadInd = 0;
          --_numAudioFramesBuffered;
          foregroundTaskPost(Update, 0); // Done with audio message, grab next thing from buffer
#         if DEBUG_ANIMATION_CONTROLLER
                _currentTime_ms += 33;
#         endif
        }
        return ret;
      }
    }
    else
    {
      return 0;
    }
  }

  extern "C" bool Update(unsigned int param)
  {
    if(IsReadyToPlay()) { // If IsReadyToPlay() returns true, we are ready to move to the next keyframe
      RobotInterface::EngineToRobot msg;
      RobotInterface::EngineToRobot::Tag msgID;
            
      // Keep reading until we hit another audio type
      bool nextAudioFrameFound = false;
      bool terminatorFound = false;
      while(!nextAudioFrameFound && !terminatorFound)
      {
        if(_currentBufferPos == _lastBufferPos) {
          // We should not be here if there isn't at least another audio sample,
          // silence, or end-of-animation keyframe in the buffer to find.
          // (Note that IsReadyToPlay() checks for there being at least _two_
          //  keyframes in the buffer, where a "keyframe" is considered an
          //  audio sample (or silence) or an end-of-animation indicator.)
          PRINT("Ran out of animation buffer after getting audio/silence.\n");
          return false;
        }
        
        msgID = PeekBufferTag();
        
        switch(msgID)
        {
          case RobotInterface::EngineToRobot::Tag_animAudioSample:
          case RobotInterface::EngineToRobot::Tag_animAudioSilence:
          {
            nextAudioFrameFound = true;
            break;
          }
          
          case RobotInterface::EngineToRobot::Tag_animStartOfAnimation:
          {
            GetFromBuffer(&msg);
            _currentTag = msg.animStartOfAnimation.tag;
            RTIP::SendMessage(msg);
#           if DEBUG_ANIMATION_CONTROLLER
            PRINT("AnimationController: StartOfAnimation w/ tag=%d\n", _currentTag);
#           endif
            break;
          } 
          
          case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
          {
#           if DEBUG_ANIMATION_CONTROLLER
            PRINT("AnimationController[t=%dms(%d)] hit EndOfAnimation\n", _currentTime_ms, HAL::GetTimeStamp());
#           endif
            GetFromBuffer(&msg);
            terminatorFound = true;
            _tracksInUse = 0;
            break;
          }
          
          case RobotInterface::EngineToRobot::Tag_animHeadAngle:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & HEAD_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] requesting head angle of %ddeg over %.2fsec\n",
                    _currentTime_ms, HAL::GetTimeStamp(),
                    msg.animHeadAngle.angle_deg, static_cast<f32>(msg.animHeadAngle.time_ms)*.001f);
#             endif
              RTIP::SendMessage(msg);
              _tracksInUse |= HEAD_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animLiftHeight:
          {
            GetFromBuffer(&msg);              
            if(_tracksToPlay & LIFT_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] requesting lift height of %dmm over %.2fsec\n",
                    _currentTime_ms, HAL::GetTimeStamp(),
                    msg.animLiftHeight.height_mm, static_cast<f32>(msg.animLiftHeight.time_ms)*.001f);
#             endif
              
              RTIP::SendMessage(msg);
              _tracksInUse |= LIFT_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animBackpackLights:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & BACKPACK_LIGHTS_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] setting backpack LEDs.\n",
                    _currentTime_ms, HAL::GetTimeStamp());
#             endif
              
              RTIP::SendMessage(msg);
              _tracksInUse |= BACKPACK_LIGHTS_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animFaceImage:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & FACE_IMAGE_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] setting face frame.\n",
                    _currentTime_ms, HAL::GetTimeStamp());
#             endif
  
              Face::Update(msg.animFaceImage);
              
              _tracksInUse |= FACE_IMAGE_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animFacePosition:
          {
            GetFromBuffer(&msg);
            
            if(_tracksToPlay & FACE_POS_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] setting face position to (%d,%d).\n",
                    _currentTime_ms, HAL::GetTimeStamp(), msg.animFacePosition.xCen, msg.animFacePosition.yCen);
#             endif
              
              Face::Move(msg.animFacePosition.xCen, msg.animFacePosition.yCen);
              
              _tracksInUse |= FACE_POS_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animBlink:
          {
            GetFromBuffer(&msg);
            
            if(_tracksToPlay & BLINK_TRACK) {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] Blinking.\n",
                    _currentTime_ms, HAL::GetTimeStamp());
#             endif
              
              if(msg.animBlink.blinkNow) {
                Face::Blink();
              } else {
                // Face::EnableBlink(msg.animBlink.enable)
              }
              _tracksInUse |= BLINK_TRACK;
            }
            break;
          }
            
          case RobotInterface::EngineToRobot::Tag_animBodyMotion:
          {
            GetFromBuffer(&msg);
            
            if(_tracksToPlay & BODY_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] setting body motion to radius=%d, speed=%d\n",
                    _currentTime_ms, HAL::GetTimeStamp(), msg.animBodyMotion.curvatureRadius_mm,
                    msg.animBodyMotion.speed);
#               endif

              _tracksInUse |= BODY_TRACK;
              
              RTIP::SendMessage(msg);
            }
            break;
          }
            
          default:
          {
            PRINT("Unexpected message type %d in animation buffer!\n", msgID);
            return 0;
          }
            
        } // switch
      } // while(!nextAudioFrameFound && !terminatorFound)
      
      if(terminatorFound) {
        _isPlaying = false;
        _haveReceivedTerminationFrame = false;
        --_numAudioFramesBuffered;
#       if DEBUG_ANIMATION_CONTROLLER
        PRINT("Reached animation %d termination frame (%d frames still buffered, curPos/lastPos = %d/%d).\n",
              _currentTag, _numAudioFramesBuffered, _currentBufferPos, _lastBufferPos);
#       endif
        _currentTag = 0;
      }

    } // if(IsReadyToPlay())
    
    return false;
  } // Update()
  
  void EnableTracks(u8 whichTracks)
  {
    _tracksToPlay |= whichTracks;
  }
  
  void DisableTracks(u8 whichTracks)
  {
    _tracksToPlay &= ~whichTracks;
  }
  
  u8 GetCurrentTag()
  {
    return _currentTag;
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
