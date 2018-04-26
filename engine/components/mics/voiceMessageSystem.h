/***********************************************************************************************************************
 *
 *  VoiceMessageSystem
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 4/4/2018
 *
 *  Description
 *  + System to handle the access / recording / playback of voice messages left by the user
 *
 **********************************************************************************************************************/

#ifndef __Engine_Components_VoiceMessageSystem_H_
#define __Engine_Components_VoiceMessageSystem_H_

#include "util/logging/logging.h"
#include <string>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class VoiceMessageSystem
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  VoiceMessageSystem();
  ~VoiceMessageSystem();

  void Initialize( Robot* robot );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Message Access API

  using MessageID = uint32_t;
  static constexpr MessageID kInvalidMessageID = 0;

  bool CanRecordNewMessage() const;
  void DeleteMessage( MessageID id );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Playback/Recording API

  uint32_t GetRecordingMessageDurationMS() const;
  MessageID StartRecordingMessage( const std::string& name = "" );

  bool HasUnreadMessages() const;

  void PlaybackRecordedMessage( MessageID messageId );
  void PlaybackFirstUnreadMessage();


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants / Structs

  using MessageIndex = size_t;
  using FileHandle = std::string;

  static constexpr MessageIndex kNumMessageSlots    = 4;
  static constexpr MessageIndex kInvalidSlot        = kNumMessageSlots;

  enum class EMessageStatus : uint8_t
  {
    Invalid,
    Unread,
    Read,
  };

  // Metadata that accompanies a saved message
  struct MessageRecord
  {
    MessageRecord();

    bool HasMessage() const { return ( status != EMessageStatus::Invalid ); }

    MessageID                   id;
    EMessageStatus              status;
    std::string                 recipientName;
    FileHandle                  fileHandle;
    int64_t                     date;
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal functions

  MessageIndex GetFreeMessageIndex() const;
  bool CreateNewMessageInternal( MessageIndex index, const std::string& recipient );
  void DeleteMessageInternal( MessageRecord& message );

  inline MessageRecord& GetMessageRecordAtIndex( MessageIndex index );
  inline const MessageRecord& GetMessageRecordAtIndex( MessageIndex index ) const;

  MessageIndex GetMessageIndex( MessageID id ) const;

  std::string GetDefaultFilename( MessageIndex index ) const;
  FileHandle GetDefaultFileHandle( MessageIndex index ) const;
  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // I/O Helpers

  void LoadMessageSystem();
  void SaveMessageSystem();
  void ReadMessageRecordFromJson( const Json::Value& value, MessageRecord& message ) const;
  void WriteMessageRecordToJson( const MessageRecord& message, Json::Value& value ) const;

  bool VerifyAndSyncMessageRecords();


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Debug

  void Debug_PrintMessageRecords() const;


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Member Data

  Robot*                    _robot;

  std::string               _saveFolder;
  MessageRecord             _messages[kNumMessageSlots];

  static MessageID          s_uniqueMessageID;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageRecord& VoiceMessageSystem::GetMessageRecordAtIndex( MessageIndex index )
{
  DEV_ASSERT( ( ( index >= 0 ) && ( index < kNumMessageSlots ) ), "VoiceMessageSystem" );
  return _messages[index];
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VoiceMessageSystem::MessageRecord& VoiceMessageSystem::GetMessageRecordAtIndex( MessageIndex index ) const
{
  DEV_ASSERT( ( ( index >= 0 ) && ( index < kNumMessageSlots ) ), "VoiceMessageSystem" );
  return _messages[index];
}

} // namespace Cozmo
} // namespace Anki

#endif // __Engine_Components_VoiceMessageSystem_H_
