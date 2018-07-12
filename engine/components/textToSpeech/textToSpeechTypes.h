#ifndef __Engine_Components_TextToSpeechTypes_H__
#define __Engine_Components_TextToSpeechTypes_H__

#include <cstdint>

namespace Anki {
namespace Cozmo {

using UtteranceId = uint8_t;
constexpr UtteranceId kInvalidUtteranceID = 0;

enum class UtteranceState
{
  Invalid,
  Generating,
  Ready,
  Playing,
  Finished
};

enum class UtteranceTriggerType
{
  Immediate,
  Manual,
  KeyFrame
};

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_Components_TextToSpeechTypes_H__
