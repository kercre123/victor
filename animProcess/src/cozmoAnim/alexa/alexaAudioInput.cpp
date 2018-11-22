/**
 * File: alexaAudioInput.cpp
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

#include "cozmoAnim/alexa/alexaAudioInput.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;
using avsCommon::avs::AudioInputStream;
#define LOG_CHANNEL "Alexa"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<AlexaAudioInput> AlexaAudioInput::Create( std::shared_ptr<AudioInputStream> stream )
{
  if( !stream ) {
    LOG_ERROR("AlexaAudioInput.Create.InvalidStream", "Input stream is invalid");
    return nullptr;
  }
  
  std::unique_ptr<AlexaAudioInput> alexaAudioInput( new AlexaAudioInput{stream} );
  if( !alexaAudioInput->Initialize() ) {
    LOG_ERROR("AlexaAudioInput.Create.CouldNotInit", "Failed to initialize AlexaAudioInput");
    return nullptr;
  }
  return alexaAudioInput;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaAudioInput::AlexaAudioInput( std::shared_ptr<AudioInputStream> stream )
: _audioInputStream{ std::move(stream) }
, _streaming{ false }
, _totalNumSamples{ 0 }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaAudioInput::Initialize()
{
  _writer = _audioInputStream->createWriter( AudioInputStream::Writer::Policy::NONBLOCKABLE );
  if( !_writer ) {
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaAudioInput::startStreamingMicrophoneData() {
  LOG_INFO("Alexa.AlexaAudioInput.StartStreaming", "");
  _streaming = true;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AlexaAudioInput::stopStreamingMicrophoneData() {
  LOG_INFO("Alexa.AlexaAudioInput.StopStreaming", "");
  _streaming = false;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaAudioInput::AddSamples( const AudioUtil::AudioSample* data, size_t size )
{
  if( !_streaming || (_writer == nullptr) ) {
    return;
  }
  
  _totalNumSamples += size;
  
  _writer->write( data, size );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::unique_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Reader> AlexaAudioInput::GetReader()
{
  if( _audioInputStream ) {
    return _audioInputStream->createReader( AudioInputStream::Reader::Policy::NONBLOCKING );
  }
  return {};
}

}  // namespace Vector
}  // namespace Anki
