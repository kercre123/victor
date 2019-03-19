/**
 * File: audioKeyFrameTypes.h
 *
 * Authors: Jordan Rivas
 * Created: 02/28/18
 *
 * Description:
 *   Structs to define Audio key frame types. These structs are used to load Animation data into Audio Key Frames.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Anki_Cozmo_AudioKeyFrameTypes_H__
#define __Anki_Cozmo_AudioKeyFrameTypes_H__

#include "clad/robotInterface/messageEngineToRobot.h"
#include <vector>

#ifdef USES_CPPLITE
#define CLAD(ns) CppLite::Anki::ns
#else
#define CLAD(ns) ns
#endif

namespace Anki {
namespace Util {
class RandomGenerator;
}
namespace Vector {
namespace AudioKeyFrameType {


enum class AudioRefTag {
  EventGroup,
  State,
  Switch,
  Parameter
};


struct AudioEventGroupRef {
  
  struct EventDef {
    CLAD(AudioMetaData)::GameEvent::GenericEvent AudioEvent;
    float Volume;
    float Probability;  // random play weight

    EventDef( CLAD(AudioMetaData)::GameEvent::GenericEvent audioEvent = CLAD(AudioMetaData)::GameEvent::GenericEvent::Invalid,
              float volume      = 1.0f,
              float probability = 1.0f )
    : AudioEvent( audioEvent )
    , Volume( volume )
    , Probability( probability ) {}
    bool operator==( const EventDef& other ) const;
  };
  
  CLAD(AudioMetaData)::GameObjectType GameObject;
  std::vector<EventDef> Events;

  AudioEventGroupRef( CLAD(AudioMetaData)::GameObjectType gameObject = CLAD(AudioMetaData)::GameObjectType::Invalid )
  : GameObject( gameObject ) {}
  
  AudioEventGroupRef& operator=( const AudioEventGroupRef& other );
  bool operator==( const AudioEventGroupRef& other ) const;

  void AddEvent( CLAD(AudioMetaData)::GameEvent::GenericEvent audioEvent, float volume, float probability );
  
  // Get an event from the group using probability values. If 'useProbability' is false or if randGen is null
  // the first event in the group will be returned.
  // If probabilty has determined not to play an event nullptr will be returned.
  const EventDef* RetrieveEvent(bool useProbability, Util::RandomGenerator* randGen) const;

};


struct AudioStateRef {
  CLAD(AudioMetaData)::GameState::StateGroupType StateGroup;
  CLAD(AudioMetaData)::GameState::GenericState State;
  
  AudioStateRef( CLAD(AudioMetaData)::GameState::StateGroupType stateGroup = CLAD(AudioMetaData)::GameState::StateGroupType::Invalid,
                 CLAD(AudioMetaData)::GameState::GenericState state = CLAD(AudioMetaData)::GameState::GenericState::Invalid )
  : StateGroup( stateGroup )
  , State( state ) {}
  
  bool operator==( const AudioStateRef& other ) const { 
    return (StateGroup == other.StateGroup) &&
           (State == other.State);
  }
};


struct AudioSwitchRef {
  CLAD(AudioMetaData)::SwitchState::SwitchGroupType SwitchGroup;
  CLAD(AudioMetaData)::SwitchState::GenericSwitch State;
  CLAD(AudioMetaData)::GameObjectType GameObject;
  
  AudioSwitchRef( CLAD(AudioMetaData)::SwitchState::SwitchGroupType switchGroup = CLAD(AudioMetaData)::SwitchState::SwitchGroupType::Invalid,
                  CLAD(AudioMetaData)::SwitchState::GenericSwitch state = CLAD(AudioMetaData)::SwitchState::GenericSwitch::Invalid,
                  CLAD(AudioMetaData)::GameObjectType gameObject = CLAD(AudioMetaData)::GameObjectType::Invalid )
  : SwitchGroup( switchGroup )
  , State( state )
  , GameObject( gameObject ) {}

  bool operator==( const AudioSwitchRef& other ) const { 
    return (SwitchGroup == other.SwitchGroup) &&
           (State == other.State) &&
           (GameObject == other.GameObject);
  }
};


struct AudioParameterRef {
  CLAD(AudioMetaData)::GameParameter::ParameterType Parameter;
  float Value;
  uint32_t Time_ms;
  CLAD(AudioEngine)::Multiplexer::CurveType Curve;
  CLAD(AudioMetaData)::GameObjectType GameObject;
  
  AudioParameterRef( CLAD(AudioMetaData)::GameParameter::ParameterType parameter = CLAD(AudioMetaData)::GameParameter::ParameterType::Invalid,
                     float value = 0.0f,
                     uint32_t time_ms = 0,
                     CLAD(AudioEngine)::Multiplexer::CurveType curve = CLAD(AudioEngine)::Multiplexer::CurveType::Linear,
                     CLAD(AudioMetaData)::GameObjectType gameObject = CLAD(AudioMetaData)::GameObjectType::Invalid )
  : Parameter( parameter )
  , Value( value )
  , Time_ms( time_ms )
  , Curve( curve )
  , GameObject( gameObject ) {}

  bool operator==( const AudioParameterRef& other ) const { 
    return (Parameter == other.Parameter) &&
           (Value == other.Value) &&
           (Time_ms == other.Time_ms) &&
           (Curve == other.Curve) &&
           (GameObject == other.GameObject);
  }
};


struct AudioRef {
  AudioRefTag Tag;
  union {
    AudioEventGroupRef  EventGroup;
    AudioStateRef       State;
    AudioSwitchRef      Switch;
    AudioParameterRef   Parameter;
  };
  
  AudioRef( AudioEventGroupRef&& eventGroupRef )
  : Tag( AudioRefTag::EventGroup )
  , EventGroup( std::move(eventGroupRef) ) {}
  
  AudioRef( AudioStateRef&& stateRef )
  : Tag( AudioRefTag::State )
  , State( std::move(stateRef) ) {}
  
  AudioRef( AudioSwitchRef&& switchRef )
  : Tag( AudioRefTag::Switch )
  , Switch( std::move(switchRef) ) {}
  
  AudioRef( AudioParameterRef&& parameterRef )
  : Tag( AudioRefTag::Parameter )
  , Parameter( std::move(parameterRef) ) {}
  
  ~AudioRef();
  
  AudioRef( const AudioRef& other );
  AudioRef& operator=( const AudioRef& other );
  bool operator==( const AudioRef& other ) const;

};


} // namespace AudioKeyFrame
} // namespace Vector
} // namespace Anki

#undef CLAD

#endif // __Anki_Cozmo_AudioKeyFrameTypes_H__
