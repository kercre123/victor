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

#include "engine/components/mics/voiceMessageTypes.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include "util/logging/logging.h"
#include "util/signals/signalHolder.h"

#include <functional>
#include <string>
#include <tuple>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {

class Robot;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class VoiceMessageSystem : public Util::SignalHolder
{
public:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  VoiceMessageSystem();
  ~VoiceMessageSystem();

  void Initialize( Robot* robot );


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Message Access API

  enum class SortType
  {
    None,
    OldestToNewest,
    NewestToOldest,
  };

  // are there any messages for the specified user?
  bool HasUnreadMessages( const std::string& name ) const;

  // return a list of all of the messages for the specified user
  VoiceMessageList GetUnreadMessages( const std::string& name, SortType sortType = SortType::None ) const;
  VoiceMessageUserList GetUnreadMessages( SortType sortType = SortType::None ) const;

  // removes the specified message from disk if it exists
  void DeleteMessage( VoiceMessageID id );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Recording/Playback

  using OnCompleteCallback = std::function<void()>;

  // attempts to start a recording for the specified user
  // returns Success and begins recording, or returns the error code on failure
  RecordMessageResult StartRecordingMessage( const std::string& name, uint32_t maxDuration_ms = 0, OnCompleteCallback = {} );

  // playback the specified voice message; returns the either Success or the error code
  EVoiceMessageError PlaybackRecordedMessage( VoiceMessageID messageId, OnCompleteCallback = {} );


private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Constants / Structs

  using MessageIndex = size_t;
  using FileHandle = std::string;

  static constexpr MessageIndex kNumMessageSlots    = 10;
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
    bool operator==( const MessageRecord& other ) const { return ( id == other.id ); }

    VoiceMessageID              id;
    EMessageStatus              status;
    std::string                 recipientName;
    FileHandle                  fileHandle;
  };

  // store info for any currently playing message
  struct ActiveMessage
  {
    const MessageRecord*        messageRecord = nullptr;
    OnCompleteCallback          callback;
  };


  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal functions

  uint32_t GetDefaultRecordingMessageDurationMS() const;

  MessageIndex GetFreeMessageIndex() const;
  void CreateNewMessageInternal( MessageIndex index, const std::string& recipient );
  void DeleteMessageInternal( MessageRecord& message );

  inline MessageRecord& GetMessageRecordAtIndex( MessageIndex index );
  inline const MessageRecord& GetMessageRecordAtIndex( MessageIndex index ) const;

  MessageIndex GetMessageIndex( VoiceMessageID id ) const;

  std::string GetDefaultFilename( MessageIndex index ) const;
  FileHandle GetDefaultFileHandle( MessageIndex index ) const;

  void OnRecordingComplete( const std::string& path );
  void OnPlaybackComplete( const std::string& path );

  // Internal helpers

  bool IsSameRecipient( const std::string& lhs, const std::string& rhs ) const;
  

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
  ActiveMessage             _currentRecording;
  ActiveMessage             _currentPlayback;

  static VoiceMessageID     s_uniqueMessageID;
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
