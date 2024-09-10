/*
 * File: StreamingWaveDataInstance.h
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


#ifndef __AnkiAudio_AudioTools_StandardWaveDataContainer_H__
#define __AnkiAudio_AudioTools_StandardWaveDataContainer_H__

#include "audioEngine/plugins/wavePortalFxTypes.h"

#include <mutex>
#include <queue>


class AkAudioBuffer;

namespace Anki {
namespace AudioEngine {
struct StandardWaveDataContainer;


class StreamingWaveDataInstance {

public:

  StreamingWaveDataInstance();

  // Add audio to back of streaming buffer
  // Return False if audio stream config does not match the first audio stream
  bool AppendAudioDataStream( PlugIns::AudioDataStream&& audioDataStream );

  // Helper to append standard wave data container
  // Give Audio Data Ownership to StreamingWaveDataInstance
  // StandardWaveDataContainer audio data is ownership is released when moved into DataInstance's queue
  // Return False if audio stream config does not match the first audio stream
  bool AppendStandardWaveData( StandardWaveDataContainer&& audioData );

  // Provider needs to Notify plugin when all data has been added
  void DoneProducingData() { _isReceivingData = false; }

  void SetIsPluginActive( bool isActive ) { _isPluginActive = isActive; }

  // Playback of stream has began
  // Note: Plugin has shared pointer to this instance, safe to clear provider's pointer;
  bool IsPlayingStream() const { return _isPlayingData; }

  bool IsPluginActive() const { return _isPluginActive; }

  // Return true is Data Stream has >= 1 audioDataStream
  bool HasAudioData() const { return !_audioDataQueue.empty(); }

  // Audio Playback (mono = 1, stereo = 2)
  uint16_t GetNumberOfChannels() const { return _numberOfChannels; }

  // Samples per sec
  uint32_t GetSampleRate() const { return _sampleRate; }

  // Samples received
  // Note: A frame is a single slice of samples for all channels.
  //       Example: A single channel stream frame consist of 1 sample, a two channel stream frame is 2 samples
  uint32_t GetNumberOfFramesReceived() const { return _numberOfFramesReceived; }

  uint32_t GetNumberOfFramesPlayed() const { return _numberOfFramesPlayed; }

  // Amount Time received
  float GetApproximateTimeReceived_sec() const { return (_numberOfFramesReceived / static_cast<float>(_sampleRate)); }

  // Return true when all data stream has been received and played back
  bool WriteToPluginBuffer( AkAudioBuffer* inOut_buffer );


private:

  enum BufferState {
    Waiting = 0,  // Waiting for data to be added to queue
    HasData,      // Current Stream has data
    Completed     // All data has been played
  };

  // Data Instance stream settings
  uint16_t _numberOfChannels = 0;
  uint32_t _sampleRate       = 0;
  // Total frame count added to streaming queue
  uint32_t _numberOfFramesReceived  = 0;
  uint32_t _numberOfFramesPlayed    = 0;
  // Run state
  bool        _isReceivingData  = true;
  bool        _isPlayingData    = false;
  bool        _isPluginActive   = false;
  size_t      _playheadIdx      = 0;
  BufferState _bufferState      = BufferState::Waiting;
  // Data
  std::mutex _lock;
  std::queue<PlugIns::AudioDataStream> _audioDataQueue;
  const PlugIns::AudioDataStream* _currentStream = nullptr;

  // Update buffer state and set current stream if data is available
  void UpdateCurrentStream();
  // Check current stream state. If all data from the _currentStream has been played clear the stream, pop data off
  // queue and attempt to update current stream.
  // Return true if the current stream is still valid or is able to be set to the next stream in queue
  bool CheckCurrentStreamState();
  // Write continuous data into wwise buffer from current stream
  size_t WriteContinuousData( AkAudioBuffer* inOut_buffer, size_t bufferPlayheadIdx );
};

} // Audio Engine
} // Anki

#endif // __AnkiAudio_AudioTools_StandardWaveDataContainer_H__
