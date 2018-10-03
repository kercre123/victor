#ifndef ANIMPROCESS_COZMO_ALEXACLIENT_H
#define ANIMPROCESS_COZMO_ALEXACLIENT_H

#include "util/helpers/noncopyable.h"
#include <memory>

#include "cozmoAnim/alexaSpeechSynthesizer.h"

// lol
#include <ACL/AVSConnectionManager.h>
#include <ACL/Transport/MessageRouter.h>
#include <ACL/Transport/PostConnectSynchronizer.h>
#include <ACL/Transport/PostConnectSynchronizerFactory.h>
#include <ADSL/DirectiveSequencer.h>
#include <AFML/AudioActivityTracker.h>
#include <AFML/FocusManager.h>
#include <AFML/VisualActivityTracker.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <Alerts/AlertsCapabilityAgent.h>
#include <Alerts/Renderer/Renderer.h>
#include <Alerts/Storage/AlertStorageInterface.h>
#include <AudioPlayer/AudioPlayer.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AudioPlayerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/TemplateRuntimeObserverInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Bluetooth/Bluetooth.h>
#include <Bluetooth/BluetoothStorageInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <CertifiedSender/SQLiteMessageStorage.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>
#include <InteractionModel/InteractionModelCapabilityAgent.h>
#include <MRM/MRMCapabilityAgent.h>
#include <Notifications/NotificationsCapabilityAgent.h>
#include <Notifications/NotificationRenderer.h>
#include <PlaybackController/PlaybackController.h>
#include <PlaybackController/PlaybackRouter.h>
#include <RegistrationManager/RegistrationManager.h>
#include <Settings/Settings.h>
#include <Settings/SettingsStorageInterface.h>
#include <Settings/SettingsUpdatedEventSender.h>
#include <SpeakerManager/SpeakerManager.h>
#include <SpeechSynthesizer/SpeechSynthesizer.h>
#include <System/SoftwareInfoSender.h>
#include <System/UserInactivityMonitor.h>
#include <TemplateRuntime/TemplateRuntime.h>


namespace Anki {
namespace Vector {
  
  
  class AlexaClient : public alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface,
  private Util::noncopyable, public std::enable_shared_from_this<AlexaClient>
{
public:
  
  static std::shared_ptr<AlexaClient> create(
                                             std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
                                             std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
    std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
    alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
    connectionObservers,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker
  );
  
  void Connect(const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate);
  
  /**
   * Begins a wake word initiated Alexa interaction.
   *
   * @param wakeWordAudioProvider The audio provider containing the audio data stream along with its metadata.
   * @param beginIndex The begin index of the keyword found within the stream.
   * @param endIndex The end index of the keyword found within the stream.
   * @param keyword The keyword that was detected.
   * @param espData The ESP measurement data.
   * @param KWDMetadata Wake word engine metadata.
   * @return A future indicating whether the interaction was successfully started.
   */
  std::future<bool> notifyOfWakeWord(
                                     alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
                                     alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex,
                                     alexaClientSDK::avsCommon::avs::AudioInputStream::Index endIndex,
                                     std::string keyword,
                                     const alexaClientSDK::capabilityAgents::aip::ESPData espData = alexaClientSDK::capabilityAgents::aip::ESPData::getEmptyESPData(),
                                     std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr);
  
  std::future<bool> notifyOfTapToTalk(
                                      alexaClientSDK::capabilityAgents::aip::AudioProvider& tapToTalkAudioProvider,
                                      alexaClientSDK::avsCommon::avs::AudioInputStream::Index beginIndex = alexaClientSDK::capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX);
  
  void onCapabilitiesStateChange(
                                 alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::State newState,
                                 alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesObserverInterface::Error newError) override;
  
  void stopForegroundActivity();
  
  void addSpeakerManagerObserver(std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerObserverInterface> observer);
  
  /**
   * Adds an observer to be notified of Alexa dialog related UX state.
   *
   * @param observer The observer to add.
   */
  void addAlexaDialogStateObserver(
                                   std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);
private:
  AlexaClient(){}
  bool Init(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
    std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
    std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
    alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
    connectionObservers,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> ttsMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ttsSpeaker,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker
  );
  
  void addConnectionObserver(
                             std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);
  
  std::shared_ptr<alexaClientSDK::avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;
  
  /// The PostConnectFactory object used to create PostConnectInterface instances.
  std::shared_ptr<alexaClientSDK::acl::PostConnectSynchronizerFactory> m_postConnectSynchronizerFactory;
  
  /// The message router.
  std::shared_ptr<alexaClientSDK::acl::MessageRouter> m_messageRouter;
  
  /// The connection manager.
  std::shared_ptr<alexaClientSDK::acl::AVSConnectionManager> m_connectionManager;
  
  /// The certified sender.
  std::shared_ptr<alexaClientSDK::certifiedSender::CertifiedSender> m_certifiedSender;
  
  /// The exception sender.
  std::shared_ptr<alexaClientSDK::avsCommon::avs::ExceptionEncounteredSender> m_exceptionSender;
  
  /// The speech synthesizer.
  std::shared_ptr<alexaClientSDK::capabilityAgents::speechSynthesizer::SpeechSynthesizer> m_speechSynthesizer;
  //std::shared_ptr<alexaClientSDK::capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer> m_speechSynthesizer;
  
  /// The alerts capability agent.
  std::shared_ptr<alexaClientSDK::capabilityAgents::alerts::AlertsCapabilityAgent> m_alertsCapabilityAgent;
  
  /// The directive sequencer.
  std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::system::UserInactivityMonitor> m_userInactivityMonitor;
  
  /// The focus manager for audio channels.
  std::shared_ptr<alexaClientSDK::afml::FocusManager> m_audioFocusManager;
  
  /// The audio activity tracker.
  std::shared_ptr<alexaClientSDK::afml::AudioActivityTracker> m_audioActivityTracker;
  
  /// The audio input processor.
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioInputProcessor> m_audioInputProcessor;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::speakerManager::SpeakerManager> m_speakerManager;
  
  /// The playbackRouter.
  std::shared_ptr<alexaClientSDK::capabilityAgents::playbackController::PlaybackRouter> m_playbackRouter;
  
  /// The playbackController capability agent.
  std::shared_ptr<alexaClientSDK::capabilityAgents::playbackController::PlaybackController> m_playbackController;
  
  /// The audio player.
  std::shared_ptr<alexaClientSDK::capabilityAgents::audioPlayer::AudioPlayer> m_audioPlayer;
  
  //std::shared_ptr<AlexaDevDirectiveHandler> m_devDirectiveHandler;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
