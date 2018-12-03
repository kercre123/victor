/**
 * File: alexaAudioInput.h
 *
 * Author: ross
 * Created: Oct 20 2018
 *
 * Description: Passes data from microphones to alexa
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file uses the sdk, here's the SDK license
/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ANIMPROCESS_COZMO_ALEXA_ALEXAMICROPHONE_H
#define ANIMPROCESS_COZMO_ALEXA_ALEXAMICROPHONE_H

#include "audioUtil/audioDataTypes.h"

#include <AVSCommon/AVS/AudioInputStream.h>
#include <Audio/MicrophoneInterface.h>

#include <memory>
#include <atomic>

namespace Anki {
namespace Vector {
  

class AlexaAudioInput : public alexaClientSDK::applicationUtilities::resources::audio::MicrophoneInterface
{
public:
  
  static std::unique_ptr<AlexaAudioInput> Create( std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream );
  
  // Adds data to the stream buffer held by alexa. Should be ok to call on another thread
  void AddSamples( const AudioUtil::AudioSample* data, size_t size );
  
  uint64_t GetTotalNumSamples() const { return _totalNumSamples; }
  
  // Stops streaming from the microphone. Returns whether the stop was successful.
  // (note: the micDataSystem job is still there, but we just drop all incoming data)
  virtual bool stopStreamingMicrophoneData() override;
  
  // Starts streaming from the microphone. returns whether the start was successful.
  // (note: the micDataSystem job was added before this was called, but we just drop all incoming data)
  virtual bool startStreamingMicrophoneData() override;

  // For debug purposes, expose the reader for this mic data. Note that reading from this reader will drain it
  // (so for debugging purposes, we make a copy of this whole object)
  std::unique_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Reader> GetReader();
  
private:
  
  explicit AlexaAudioInput( std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream );
  
  bool Initialize();
  
  // The stream of audio data.
  const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> _audioInputStream;
  
  // The writer that will be used to writer audio data into the sds.
  std::unique_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Writer> _writer;
  
  std::atomic<bool> _streaming;
  std::atomic<uint64_t> _totalNumSamples;
  
};
  
} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_ALEXAMICROPHONE_H
