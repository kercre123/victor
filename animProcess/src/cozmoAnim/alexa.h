#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include "util/helpers/noncopyable.h"
#include <memory>
#include <functional>
//#include <AVSCommon/Utils/MediaPlayer/MediaPlayerObserverInterface.h>
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
  class KeywordObserver;
  
  namespace RobotInterface {
    struct MicData;
  }
  
class Alexa : private Util::noncopyable
            , public std::enable_shared_from_this<Alexa>
            //, public alexaClientSDK::avsCommon::sdkInterfaces::MediaPlayerObserverInterface
            , public alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface
{
public:
  
  using OnStateChangedCallback = std::function<void(AlexaUXState)>;
  void Init(const AnimContext* context, const OnStateChangedCallback& onStateChanged);
  void Update();
  
  void ButtonPress();
  void TriggerWord(int from_ms, int to_ms);
  
  void ProcessAudio( int16_t* data, size_t size);
  
  AlexaUXState GetState() const { return _uxState; }
  
  bool IsIdle() const { return _uxState == AlexaUXState::Idle; }
  
protected:
  virtual void onDialogUXStateChanged(DialogUXState newState) override;
  
private:
  
  OnStateChangedCallback _onStateChanged;
  
  AlexaUXState _uxState = AlexaUXState::Idle;
  
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> m_tapToTalkAudioProvider;
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> m_wakeWordAudioProvider;
  
  std::shared_ptr<KeywordObserver> m_keywordObserver;
  
  std::shared_ptr<AlexaClient> m_client;
  
  std::shared_ptr<AlexaMicrophone> m_microphone;
  
  std::shared_ptr<AlexaSpeaker> m_TTSSpeaker;
  std::shared_ptr<AlexaSpeaker> m_alertsSpeaker;
  std::shared_ptr<AlexaSpeaker> m_audioSpeaker;
  
  const AnimContext* _context = nullptr;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
