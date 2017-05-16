/*
 * File: audioEngineClient.h
 *
 * Author: Jordan Rivas
 * Created: 11/09/2015
 *
 * Description: This is a subclass of AudioClient which provides communication between itself and an
 *              AudioEngineClientConnection by means of AudioEngineMessageHandler. It provides core audio functionality
 *              by broadcasting post messages and subscribing to callback messages.
 *
 * Copyright: Anki, Inc. 2015
 *
 */

#ifndef __Basestation_Audio_AudioEngineClient_H__
#define __Basestation_Audio_AudioEngineClient_H__

#include "audioEngine/multiplexer/audioMuxClient.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include <vector>


namespace Anki {
namespace Cozmo {
namespace Audio {
  
class AudioEngineMessageHandler;
  
class AudioEngineClient : public AudioEngine::Multiplexer::AudioMuxClient
{
public:
  
  
  AudioEngineClient();
  virtual ~AudioEngineClient();
  
  void SetMessageHandler( AudioEngineMessageHandler* messageHandler );
  
  // Perform event
  // Provide a callback lambda to get all event callbacks; Duration, Marker, Complete & Error.
  using MuxCallbackIdType = AudioEngine::Multiplexer::AudioMuxClient::CallbackIdType;
  virtual MuxCallbackIdType PostEvent( const AudioMetaData::GameEvent::GenericEvent event,
                                       const AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid,
                                       const AudioEngine::Multiplexer::AudioMuxClient::CallbackFunc& callback = nullptr ) override;
  
  virtual void StopAllEvents( const AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid ) override;
  
  virtual void PostGameState( const AudioMetaData::GameState::StateGroupType gameStateGroup,
                              const AudioMetaData::GameState::GenericState gameState ) override;
  
  virtual  void PostSwitchState( const AudioMetaData::SwitchState::SwitchGroupType switchGroup,
                                 const AudioMetaData::SwitchState::GenericSwitch switchState,
                                 const AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid ) override;
  
  virtual void PostParameter( const AudioMetaData::GameParameter::ParameterType parameter,
                              const float parameterValue,
                              const AudioMetaData::GameObjectType gameObject = AudioMetaData::GameObjectType::Invalid,
                              const int32_t timeInMilliSeconds = 0,
                              const AudioEngine::Multiplexer::CurveType = AudioEngine::Multiplexer::CurveType::Linear ) const override;
  
  virtual void PostMusicState( const AudioMetaData::GameState::GenericState musicState,
                               const bool interrupt = false,
                               const uint32_t minDuration_ms = 0 ) override;

protected:
  
  AudioEngineMessageHandler* _messageHandler = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;

};

} // Audio
} // Cozmo
} // Anki

#endif /* __Basestation_Audio_AudioEngineClient_H__ */
