/*
 * File: StreamingWaveDataInstance.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/17/18
 *
 * Description: This class is used to cache and pass audio PCM data from producer to a consumer (Streaming Wave Portal
 *              Plugin). The producer appends either a AudioDataStream or StandardWaveDataContainer to the back of the
 *              queue while the consumer uses the data and pops them off the queue.  When the producer is done appending
 *              data it must call DoneProducingData() to notify the consumer the stream is complete.
 *
 * Note: This currently only supports single channel data
 *
 * Copyright: Anki, Inc. 2018
 *
 */


#include "audioEngine/audioTools/streamingWaveDataInstance.h"
#include "audioEngine/audioTools/standardWaveDataContainer.h"
#include "util/logging/logging.h"


// Compile Out
#ifndef EXCLUDE_ANKI_AUDIO_LIBS
#define USE_AUDIO_ENGINE 1
#include <AK/SoundEngine/Common/IAkPlugin.h>

#else
#define USE_AUDIO_ENGINE 0
#endif

namespace Anki {
namespace AudioEngine {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingWaveDataInstance::StreamingWaveDataInstance()
: _audioDataQueue()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWaveDataInstance::AppendAudioDataStream( PlugIns::AudioDataStream&& audioDataStream )
{
  // If this is the first data, set channels and sample rate to match.
  // Otherwise, make sure channel and sample rate do not change.
  if (_numberOfFramesReceived == 0) {
    _numberOfChannels = audioDataStream.numberOfChannels;
    _sampleRate = audioDataStream.sampleRate;
  } else if (audioDataStream.numberOfChannels != _numberOfChannels) {
    LOG_ERROR("StreamingWaveDataInstance.AppendAudioDataStream.InvalidNumberOfChannels",
      "Expected %d but got %d", _numberOfChannels, audioDataStream.numberOfChannels);
    return false;
  } else if (audioDataStream.sampleRate != _sampleRate) {
    LOG_ERROR("StreamingWaveDataInstance.AppendAudioDataStream.InvalidSampleRate",
      "Expected %d but got %d", _sampleRate, audioDataStream.sampleRate);
    return false;
  }

  // Track how many frames have been added to stream
  _numberOfFramesReceived += (audioDataStream.bufferSize / _numberOfChannels);

  std::lock_guard<std::mutex> lock(_lock);
  _audioDataQueue.push( std::move(audioDataStream) );
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWaveDataInstance::AppendStandardWaveData( StandardWaveDataContainer&& audioData )
{
  bool success = false;
#if USE_AUDIO_ENGINE
  using AudioDataStream = PlugIns::AudioDataStream;
  AudioDataStream dataStream = AudioDataStream( audioData.sampleRate,
                                                audioData.numberOfChannels,
                                                audioData.ApproximateDuration_ms(),
                                                static_cast<uint32_t>(audioData.bufferSize),
                                                std::unique_ptr<float[]>(audioData.audioBuffer) );
  success = AppendAudioDataStream( std::move(dataStream) );
  audioData.ReleaseAudioDataOwnership();
#endif
  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWaveDataInstance::WriteToPluginBuffer( AkAudioBuffer* inOut_buffer )
{
  // Return true when all data stream has been received and played back
  bool success = true;

#if USE_AUDIO_ENGINE

  if ( _bufferState == BufferState::Waiting ) {
    UpdateCurrentStream();
    // Check updated state
    if ( _bufferState == BufferState::Waiting ) {
      // Still waiting
      // NOTE: We are investigating why tts generator is taking so long to produce the last frame and reporting that it
      //       is done producing data. Because of this, this log may show up in situations where we have completed
      //       playing tts speech.
      LOG_WARNING("StreamingWaveDataInstance.WriteToPluginBuffer", "No Data, plugin will be starved");
      return false;
    }
  }

  size_t bufferPlayhead = 0;
  while ( (bufferPlayhead < inOut_buffer->MaxFrames()) && (_bufferState == BufferState::HasData) ) {

    bufferPlayhead += WriteContinuousData( inOut_buffer, bufferPlayhead );

    if ( !CheckCurrentStreamState() ) {
      // Current Stream is empty
      if ( _bufferState == BufferState::Waiting ) {
        LOG_WARNING("StreamingWaveDataInstance.WriteToPluginBuffer", "No Data, plugin will be starved");
      }
    }
  } // while ()

  // Tell wwise no more data when buffer has completely played all data
  inOut_buffer->eState = (_bufferState == BufferState::Completed) ? AK_NoMoreData : AK_DataReady;

  // Pad end of buffer
  if ( inOut_buffer->uValidFrames < inOut_buffer->MaxFrames() ) {
    // Pad end of buffer with zeros
    inOut_buffer->ZeroPadToMaxFrames();
  }

  success = ( inOut_buffer->eState == AK_NoMoreData );
#endif

  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingWaveDataInstance::UpdateCurrentStream()
{
  std::lock_guard<std::mutex> lock(_lock);
  if ( _audioDataQueue.empty() ) {
    _bufferState = _isReceivingData ? BufferState::Waiting : BufferState::Completed;
  }
  else {
    _currentStream = &_audioDataQueue.front();
    _playheadIdx = 0;
    _bufferState = BufferState::HasData;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWaveDataInstance::CheckCurrentStreamState()
{
  if ( _currentStream->bufferSize <= _playheadIdx ) {
    // Pop current frame
    {
      std::lock_guard<std::mutex> lock(_lock);
      // Sanity check, still current stream
      ASSERT_NAMED(_currentStream == &_audioDataQueue.front(),
                   "StreamingWaveDataInstance.CheckCurrentStreamState.ValidateCurrentStream");
      _currentStream = nullptr;
      _audioDataQueue.pop();
    }
    UpdateCurrentStream();
    return (_bufferState == BufferState::HasData); // If there is no data return false
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t StreamingWaveDataInstance::WriteContinuousData( AkAudioBuffer* inOut_buffer, size_t bufferPlayheadIdx )
{
  // TODO: Add Support for multiple channels
  size_t frameCount = 0;

#if USE_AUDIO_ENGINE
  // Set Min Frame Count only using the current stream
  const size_t bufferSize = inOut_buffer->MaxFrames() - bufferPlayheadIdx;
  size_t streamSize = _currentStream->bufferSize - _playheadIdx;
  frameCount = bufferSize < streamSize ? bufferSize : streamSize;
  inOut_buffer->uValidFrames = frameCount + bufferPlayheadIdx;
  ASSERT_NAMED(inOut_buffer->uValidFrames <= inOut_buffer->MaxFrames(),
               "StreamingWaveDataInstance.WriteContinuousData.InvalidValidFrames");

  // Copy data into current wwise stream
  // TODO: This is only for mono sources
  AkSampleType* destination = inOut_buffer->GetChannel(0) + bufferPlayheadIdx;
  AkSampleType* source = _currentStream->audioBuffer.get() + _playheadIdx;
  memcpy( destination, source, sizeof(AkSampleType) * frameCount );

  _playheadIdx += frameCount;
  _numberOfFramesPlayed += frameCount;
#endif

  return frameCount;
}


} // Audio Engine
} // Anki
