/***********************************************************************************************************************
 *
 *  Types to be used with the VoiceMessageSystem
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 5/24/2018
 *
 **********************************************************************************************************************/

#ifndef __Engine_Components_VoiceMessageTypes_H_
#define __Engine_Components_VoiceMessageTypes_H_

#include <string>
#include <vector>


namespace Anki {
namespace Vector {

using VoiceMessageID = uint32_t;
static constexpr VoiceMessageID kInvalidVoiceMessageId = 0;

using VoiceMessageList = std::vector<VoiceMessageID>;
struct VoiceMessageUser
{
  std::string       recipient; // note: this should be an object, not a string
  VoiceMessageList  messages;
};

using VoiceMessageUserList = std::vector<VoiceMessageUser>;

// error "codes" when attempting to record/playback messages
enum class EVoiceMessageError : uint8_t
{
  Success,
  InvalidMessage,     // generic "something went wrong"
  MailboxFull,
  MessageAlreadyActive,
};

struct RecordMessageResult
{
  VoiceMessageID      id;
  EVoiceMessageError  error;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_Components_VoiceMessageTypes_H_
