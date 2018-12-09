/**
 * File: alexaAudioFactory.h
 *
 * Author: Brad Neuman
 * Created: 2018-12-09
 *
 * Description: An Audio factory implementation to pass in to the AVS SDK to provide local audio files
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __AnimProcess_Src_CozmoAnim_Alexa_Media_AlexaAudioFactory_H__
#define __AnimProcess_Src_CozmoAnim_Alexa_Media_AlexaAudioFactory_H__
#pragma once

#include <Audio/AudioFactory.h>
#include <memory>

namespace Anki {
namespace Vector {

namespace AlexaSDKAudio = alexaClientSDK::avsCommon::sdkInterfaces::audio;

// the main factory actually contains multiple factories for different types of audio. For now we are just
// customizing Alerts, but if we wanted to customize the others we could do so here
class AlexaCustomAlertsAudioFactory;

class AlexaAudioFactory : public AlexaSDKAudio::AudioFactoryInterface {
public:

  AlexaAudioFactory();

  std::shared_ptr<AlexaSDKAudio::AlertsAudioFactoryInterface> alerts() const override;
  std::shared_ptr<AlexaSDKAudio::NotificationsAudioFactoryInterface> notifications() const override;
  std::shared_ptr<AlexaSDKAudio::CommunicationsAudioFactoryInterface> communications() const override;

private:

  std::shared_ptr<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory> _defaultFactory;
  std::shared_ptr<AlexaCustomAlertsAudioFactory> _customAlertsFactory;  
};


// Use custom responses for the alerts audio factory
class AlexaCustomAlertsAudioFactory : public AlexaSDKAudio::AlertsAudioFactoryInterface
{
public:

  // TODO:(bn) VIC-12393 use actual custom audio files here
  
  // take in the default factory so we can use those audio samples where desired
  explicit AlexaCustomAlertsAudioFactory(
    std::shared_ptr<AlexaSDKAudio::AlertsAudioFactoryInterface> defaultFactory);

  std::function<std::unique_ptr<std::istream>()> alarmDefault() const override;
  std::function<std::unique_ptr<std::istream>()> alarmShort() const override;
  std::function<std::unique_ptr<std::istream>()> timerDefault() const override;
  std::function<std::unique_ptr<std::istream>()> timerShort() const override;
  std::function<std::unique_ptr<std::istream>()> reminderDefault() const override;
  std::function<std::unique_ptr<std::istream>()> reminderShort() const override;

private:
  
  std::shared_ptr<AlexaSDKAudio::AlertsAudioFactoryInterface> _defaultFactory;
};

}
}

#endif
