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

class AlexaClient : private Util::noncopyable, public std::enable_shared_from_this<AlexaClient>
{
public:
  
  static std::shared_ptr<AlexaClient> create(
                                             std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
                                             std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
    alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
    connectionObservers,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate
  );
  
  void Connect(const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate);
private:
  AlexaClient(){}
  bool Init(
    std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> deviceInfo,
    std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> customerDataManager,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
    std::shared_ptr<alexaClientSDK::certifiedSender::MessageStorageInterface> messageStorage,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface>>
    alexaDialogStateObservers,
    std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>>
    connectionObservers,
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateInterface> capabilitiesDelegate
  );
  
  
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
  //std::shared_ptr<alexaClientSDK::capabilityAgents::speechSynthesizer::SpeechSynthesizer> m_speechSynthesizer;
  std::shared_ptr<alexaClientSDK::capabilityAgents::speechSynthesizer::AlexaSpeechSynthesizer> m_speechSynthesizer;
  
  /// The directive sequencer.
  std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
