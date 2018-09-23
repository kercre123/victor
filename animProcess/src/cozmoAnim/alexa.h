#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include "util/helpers/noncopyable.h"
#include <memory>


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
  
class Alexa : private Util::noncopyable
{
public:
  void Init();
  
  void ButtonPress();
private:
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;
  
  std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioProvider> m_tapToTalkAudioProvider;
  
  std::shared_ptr<AlexaClient> m_client;
  
  std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> m_TTSSpeaker;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
