/**
 * File: alexaImpl.h
 *
 * Author: ross
 * Created: Oct 16 2018
 *
 * Description: Component that integrates with the Alexa Voice Service (AVS) SDK
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

#ifndef ANIMPROCESS_COZMO_ALEXAIMPL_H
#define ANIMPROCESS_COZMO_ALEXAIMPL_H
#pragma once

#include "audioUtil/audioDataTypes.h"
#include "util/helpers/noncopyable.h"

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <CBLAuthDelegate/CBLAuthRequesterInterface.h>

#include <functional>
#include <list>
#include <string>

namespace alexaClientSDK{
  namespace capabilitiesDelegate {class CapabilitiesDelegate; }
  namespace capabilityAgents { namespace aip { class AudioProvider; } }
  namespace avsCommon {
    namespace utils{ namespace mediaPlayer { class MediaPlayerInterface; } }
  }
}


namespace Anki {
namespace Vector {

class AlexaAudioInput;
enum class AlexaAuthState : uint8_t;
class AlexaClient;
class AlexaKeywordObserver;
class AlexaMediaPlayer;
class AlexaObserver;
class AnimContext;

class AlexaImpl : private Util::noncopyable
{
public:
  
  AlexaImpl();
  
  ~AlexaImpl();
  
  bool Init(const AnimContext* context);
  
  void Update();
  
  void StopForegroundActivity();
  
  // Adds samples to the mic stream buffer. Should be ok to call on another thread
  void AddMicrophoneSamples( const AudioUtil::AudioSample* const samples, size_t nSamples );
  
  void NotifyOfTapToTalk();
  
  void NotifyOfWakeWord( long from_ms, long to_ms );
  
  // Callback setters
  
  // this callback should not call AuthDelegate methods
  using OnAlexaAuthStateChanged = std::function<void(AlexaAuthState, const std::string&, const std::string&)>;
  void SetOnAlexaAuthStateChanged( const OnAlexaAuthStateChanged& callback ) { _onAlexaAuthStateChanged = callback; }
  
private:
  
  std::vector<std::shared_ptr<std::istream>> GetConfigs() const;
  
  void OnDirective(const std::string& directive, const std::string& payload);
  
  void SetAuthState( AlexaAuthState state, const std::string& url, const std::string& code );
  
  // things we care about called by AlexaObserver
  void OnDialogUXStateChanged( alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState state );
  void OnRequestAuthorization( const std::string& url, const std::string& code );
  void OnAuthStateChange( alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::State newState,
                          alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface::Error newError );
  
  
  const AnimContext* _context = nullptr;
  std::string _alexaFolder;
  
  AlexaAuthState _currAuthState;
  
  std::shared_ptr<AlexaClient> _client;
  
  std::shared_ptr<AlexaObserver> _observer;
  
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> _capabilitiesDelegate;
  
  std::shared_ptr<AlexaMediaPlayer> _ttsMediaPlayer;
  std::shared_ptr<AlexaMediaPlayer> _alertsMediaPlayer;
  std::shared_ptr<AlexaMediaPlayer> _audioMediaPlayer;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> _tapToTalkAudioProvider;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> _wakeWordAudioProvider;
  std::shared_ptr<AlexaKeywordObserver> _keywordObserver;
  std::shared_ptr<AlexaAudioInput> _microphone;
  
  // callbacks
  OnAlexaAuthStateChanged _onAlexaAuthStateChanged;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAIMPL_H
