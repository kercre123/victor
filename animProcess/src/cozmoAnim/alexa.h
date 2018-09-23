#ifndef ANIMPROCESS_COZMO_ALEXA_H
#define ANIMPROCESS_COZMO_ALEXA_H

#include "util/helpers/noncopyable.h"
#include <memory>


namespace alexaClientSDK{namespace capabilitiesDelegate {class CapabilitiesDelegate; }}

namespace Anki {
namespace Vector {

class Alexa : private Util::noncopyable
{
public:
  void Init();
private:
  std::shared_ptr<alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate> m_capabilitiesDelegate;
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXA_H
