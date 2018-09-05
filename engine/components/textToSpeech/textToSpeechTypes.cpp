
#include "textToSpeechTypes.h"

namespace Anki {
namespace Vector {

const char * EnumToString(UtteranceState state)
{
  #define SWITCHCASE(enumval) \
    case UtteranceState::enumval: return #enumval;

  switch (state) {
    SWITCHCASE(Invalid);
    SWITCHCASE(Generating);
    SWITCHCASE(Ready);
    SWITCHCASE(Playing);
    SWITCHCASE(Finished);
  }

  #undef SWITCHCASE
  return "Unknown";
}

const char * EnumToString(UtteranceTriggerType triggerType)
{
  #define SWITCHCASE(enumval) \
    case UtteranceTriggerType::enumval: return #enumval;

  switch (triggerType) {
    SWITCHCASE(Immediate);
    SWITCHCASE(Manual);
    SWITCHCASE(KeyFrame);
  }

  #undef SWITCHCASE
  return "Unknown";
}

} // namespace Vector
} // namespace Anki
