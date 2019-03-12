/**
 * File: alexaPlaybackRecognizerComponent.cpp
 *
 * Author: Jordan Rivas
 * Created: 02/01/19
 *
 * Description: Run "Alexa" speech recognizer on the audio engine's master output. The recognizer is provided by the
 *              SpeechRecognizerSystem and ran on a worker thread. The SpeechRecognizerSystem responds when there is
 *              a trigger detected in the playback stream.
 *
 * Copyright: Anki, Inc. 2019
 *
 */


#include "cozmoAnim/alexa/media/alexaPlaybackRecognizerComponent.h"

#include "audioEngine/plugins/akAlsaSinkPlugIn.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/micData/micDataSystem.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerPryonLite.h"
#include "cozmoAnim/speechRecognizer/speechRecognizerSystem.h"
#include "speex/speex_resampler.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"

#define LOG_CHANNEL "SpeechRecognizer"


namespace Anki {
namespace Vector {
  
AlexaPlaybackRecognizerComponent::AlexaPlaybackRecognizerComponent( const Anim::AnimContext* context,
                                                                    SpeechRecognizerSystem& speechRecognizerSystem )
: _context( context )
, _speechRecSys( speechRecognizerSystem )
, _processAudioThread( std::thread( &AlexaPlaybackRecognizerComponent::ProcessAudioLoop, this ) )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaPlaybackRecognizerComponent::~AlexaPlaybackRecognizerComponent()
{
  _isActive = false;
  _isInitialized = false;

  // Clear sink plugin callback
  _sinkPlugin->SetPlaybackBufferCallback( nullptr );

  // Stop Worker thread
  _processThreadStop = true;
  _pendingLocaleUpdate = false;
  _processAudioThreadCondition.notify_all();
  _processAudioThread.join();

  // Release resampler
  if ( _resamplerState != nullptr ) {
    speex_resampler_destroy( _resamplerState );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaPlaybackRecognizerComponent::Init()
{
  _isInitialized = false;
  // Setup objects
  if ( _context == nullptr ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent._context.IsNull", "");
    return false;
  }
  const auto* audioCtr = _context->GetAudioController();
  if ( audioCtr == nullptr ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent.audioCtr.IsNull", "");
    return false;
  }
  const auto* const pluginInterface = audioCtr->GetPluginInterface();
  if ( pluginInterface == nullptr ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent.pluginInterface.IsNull", "");
    return false;
  }
  _sinkPlugin = pluginInterface->GetAlsaSinkPlugIn();
  if ( _sinkPlugin == nullptr ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent.alsaSinkPlugin.IsNull", "");
    return false;
  }
  if ( !_speechRecSys._alexaPlaybackTrigger ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent._speechRecSys._alexaPlaybackTrigger.IsNull", "");
    return false;
  }
  _recognizer = _speechRecSys._alexaPlaybackTrigger->recognizer.get();
  if ( _recognizer == nullptr ) {
    LOG_ERROR("AlexaPlaybackRecognizerComponent._recognizer", "");
    return false;
  }
  
  // Setup resampler
  int error = 0;
  _resamplerState = speex_resampler_init( SinkPluginTypes::kAkAlsaSinkChannelCount,  // num channels
                                          SinkPluginTypes::kAkAlsaSinkSampleRate,    // in rate, int
                                          kRecognizerSampleRate,                     // out rate, int
                                          SPEEX_RESAMPLER_QUALITY_DEFAULT,           // quality 0-10
                                          &error );
  if( error != 0 ) {
    LOG_WARNING("AlexaPlaybackRecognizerComponent", "speex_resampler_init error %d", error);
    return false;
  }
  
  // Setup sink plugin callback
  _sinkPlugin->SetPlaybackBufferCallback( std::bind( &AlexaPlaybackRecognizerComponent::SinkPluginCallback,
                                                     this, std::placeholders::_1, std::placeholders::_2 ) );
  _isInitialized = true;
  return _isInitialized;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaPlaybackRecognizerComponent::SetRecognizerActivate( bool activate )
{
  // We expect that _isInitialized is true
  _isActive = activate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaPlaybackRecognizerComponent::PendingLocaleUpdate()
{
  // Set flag and notify worker thread
  _pendingLocaleUpdate = true;
  _processAudioThreadCondition.notify_all();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaPlaybackRecognizerComponent::SinkPluginCallback( const SinkPluginTypes::AudioChunk& chunk, bool hasData )
{
  // Check if Alexa is active and the callback period has audio data
  if ( !_isActive || !hasData ) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock( _audioEngOutputBufferMutex );
    if ( _audioEngOutBuffer.size() == _audioEngOutBuffer.capacity() ) {
      _audioEngOutBuffer.pop_front();
      LOG_WARNING("AlexaPlaybackRecognizerComponent.SinkPluginCallback", "_audioEngOutputBuffer is FULL!!");
    }
    // Copy data into local buffer
    auto bufferPeriod = _audioEngOutBuffer.push_back();
    memcpy( bufferPeriod, chunk, sizeof(SinkPluginTypes::AudioChunk) );
  }
  // Wake worker thread
  _processAudioThreadCondition.notify_all();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaPlaybackRecognizerComponent::ProcessAudioLoop()
{
  Anki::Util::SetThreadName( pthread_self(), "AlexPlaybackRec" );
  
  while ( !_processThreadStop ) {
    const SinkPluginTypes::AudioChunk* inData = nullptr;
    // Wait for audio engine thread to push data into buffer. Then get it out!
    {
      std::unique_lock<std::mutex> lock( _audioEngOutputBufferMutex );
      const auto waitFunc = [this] {
        return !_audioEngOutBuffer.empty() || _pendingLocaleUpdate || _processThreadStop;
      };
      _processAudioThreadCondition.wait( lock, waitFunc );
      if ( _processThreadStop ) {
        return;
      }
      
      // Get Pointers to all valid data
      // Note: It is possible that the pointer's data is overridden. We are okay with this, the buffer is static memory,
      //       the buffer is large enough to hold as much data we are concerned about and if the buffer is that far
      //       behind the data is useless to us.
      while ( !_audioEngOutBuffer.empty() ) {
        _validOutBufferPtrs.push_back( &_audioEngOutBuffer.front() );
        _audioEngOutBuffer.pop_front();
      }
    }
    
    // Process all data
    while ( !_validOutBufferPtrs.empty() ) {
      inData = _validOutBufferPtrs.front();
      _validOutBufferPtrs.pop_front();
      ProcessAudio( inData );
    }
    // Check if locale needs to be updated
    if ( _pendingLocaleUpdate && _isInitialized ) {
      auto* playbackTrigger = _speechRecSys._alexaPlaybackTrigger.get();
      ASSERT_NAMED(playbackTrigger != nullptr,
                   "AlexaPlaybackRecognizerComponent.ProcessAudioLoop.playbackTrigger.IsNull");
      _speechRecSys.ApplySpeechRecognizerLoacleUpdate( *playbackTrigger );
      _pendingLocaleUpdate = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaPlaybackRecognizerComponent::ProcessAudio( const SinkPluginTypes::AudioChunk* inData )
{
  // Resample to new buffer
  uint32_t numSamplesProcessed = SinkPluginTypes::kAkAlsaSinkBufferSize;
  uint32_t numSamplesWritten = kResamplerBufferSize;
  
  SinkPluginTypes::AkSinkAudioSample_t* outData = nullptr;
  if ( !_resampledAudioBuffer.push_back( outData, numSamplesWritten ) ) {
    // Thish should never happen because the previous call to ProcessAudio() should have made plenty of room
    LOG_WARNING("AlexaPlaybackRecognizerComponent.ProcessAudioLoop",
                "_resampledAudioBuffer doesn't have enough continous memory for push_back()!!!");
    
    // Pop data to make room
    _resampledAudioBuffer.pop_front( kResamplerBufferSize );
    if ( !_resampledAudioBuffer.push_back( outData, numSamplesWritten ) ) {
      LOG_ERROR("AlexaPlaybackRecognizerComponent.ProcessAudioLoop._resampledAudioBuffer.push_back.secondAttempt.Fail", "");
      return;
    }
  }
  
  speex_resampler_process_interleaved_int( _resamplerState,
                                           *inData, &numSamplesProcessed,
                                           outData, &numSamplesWritten );
  
  if ( numSamplesWritten > 0 ) {
    while ( (_resampledAudioBuffer.size() / kRecognizerBufferSize) > 0 ) {
      _recognizer->Update( &_resampledAudioBuffer.front(), kRecognizerBufferSize );
      _resampledAudioBuffer.pop_front( kRecognizerBufferSize );
    }
  }
  else {
    LOG_WARNING("AlexaPlaybackRecognizerComponent.ProcessAudioLoop", "Resample Error");
  }
}


} // namespace Vector
} // namespace Anki
