#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include "util/helpers/noncopyable.h"
#include <memory>
#include <functional>
#include <unordered_set>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include "clad/types/alexaUXState.h"

namespace alexaClientSDK{
  namespace capabilitiesDelegate {class CapabilitiesDelegate; }
  namespace capabilityAgents { namespace aip { class AudioProvider; } }
  namespace avsCommon {
    namespace utils{ namespace mediaPlayer { class MediaPlayerInterface; } }
  }
}

namespace Anki {
namespace Vector {

class AlexaClient;
class AlexaMicrophone;
class AlexaSpeaker;
class AnimContext;
class AlexaAlertsManager;
  class KeywordObserver;
  
  namespace RobotInterface {
    struct MicData;
    struct AlexaAlerts;
  }
  
class Alexa : private Util::noncopyable
            , public std::enable_shared_from_this<Alexa>
            , public alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface
            , public alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface
{
public:
  
  using OnStateChangedCallback = std::function<void(AlexaUXState)>;
  using OnAlertChangedCallback = std::function<void(RobotInterface::AlexaAlerts)>;
  void Init(const AnimContext* context,
            const OnStateChangedCallback& onStateChanged,
            const OnAlertChangedCallback& onAlertsChanged);
  void Update();
  
  void ButtonPress();
  void TriggerWord(int from_ms, int to_ms);
  
  void ProcessAudio( int16_t* data, size_t size);
  
  AlexaUXState GetState() const { return _uxState; }
  
  bool IsIdle() const { return _uxState == AlexaUXState::Idle; }
  
  void StopForegroundActivity();
  
  void CancelAlerts(const std::vector<int>& alertIDs);
  
protected:
  // callbacks that affect ux state
  
  virtual void onDialogUXStateChanged(DialogUXState newState) override;
  virtual void   onPlaybackStarted (SourceId id) override;
  virtual void   onPlaybackFinished (SourceId id) override;
  virtual void   onPlaybackStopped(SourceId id) override;
  virtual void   onPlaybackError (SourceId id, const alexaClientSDK::avsCommon::utils::mediaPlayer::ErrorType
 &type, std::string error) override;
  // todo: more methods, like pausing
  
private:
  void CheckForStateChange();
  
  void OnDirective(const std::string& directive, const std::string& payload);
  
  OnStateChangedCallback _onStateChanged;
  // anki state
  AlexaUXState _uxState = AlexaUXState::Idle;
  
  // alexa component states
  DialogUXState _dialogState = DialogUXState::IDLE;
  std::unordered_set<SourceId> _playingSources;
  
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> m_tapToTalkAudioProvider;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> m_wakeWordAudioProvider;
  
  std::shared_ptr<KeywordObserver> m_keywordObserver;
  
  std::shared_ptr<AlexaClient> m_client;
  
  std::shared_ptr<AlexaMicrophone> m_microphone;
  
  std::shared_ptr<AlexaSpeaker> m_TTSSpeaker;
  std::shared_ptr<AlexaSpeaker> m_alertsSpeaker;
  std::shared_ptr<AlexaSpeaker> m_audioSpeaker;
  
  std::shared_ptr<AlexaAlertsManager> _alertsManager;
  
  const AnimContext* _context = nullptr;
  float _lastPlayDirective = -1.0f;
  float _timeToSetIdle = -1.0f;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
