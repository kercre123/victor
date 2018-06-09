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

// #include "engine/components/mics/voiceMessageSystem.h"
#include "voiceMessageSystem.h"
#include "engine/robot.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotManager.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "json/json.h"

#include <algorithm>
#include <unordered_set>


#define DEBUG_VOICESYSTEM 0


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if REMOTE_CONSOLE_ENABLED
  namespace {
    VoiceMessageSystem*             sVMSystem      = nullptr;
  }

  void RecordMessageFor( ConsoleFunctionContextRef context )
  {
    if ( nullptr != sVMSystem )
    {
      const char* name = ConsoleArg_GetOptional_String( context, "name", nullptr );
      if ( nullptr != name )
      {
        sVMSystem->StartRecordingMessage( name );
      }
    }
  }
  CONSOLE_FUNC( RecordMessageFor, "VoiceMessage", const char* name );

  void PlaybackNextMessageFor( ConsoleFunctionContextRef context )
  {
    const char* name = ConsoleArg_GetOptional_String( context, "name", nullptr );
    if ( nullptr != name )
    {
      VoiceMessageList messages = sVMSystem->GetUnreadMessages( name, VoiceMessageSystem::SortType::OldestToNewest );
      if ( !messages.empty() )
      {
        sVMSystem->PlaybackRecordedMessage( messages.front() );
        sVMSystem->DeleteMessage( messages.front() );
      }
    }
  }
  CONSOLE_FUNC( PlaybackNextMessageFor, "VoiceMessage", const char* name );

  void DeleteMessagesFor( ConsoleFunctionContextRef context )
  {
    const char* name = ConsoleArg_GetOptional_String( context, "name", nullptr );
    if ( nullptr != name )
    {
      VoiceMessageList messages = sVMSystem->GetUnreadMessages( name );
      for ( const auto& message : messages )
      {
        sVMSystem->DeleteMessage( message );
      }
    }
  }
  CONSOLE_FUNC( DeleteMessagesFor, "VoiceMessage", const char* name );

#endif // REMOTE_CONSOLE_ENABLED


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  const std::string   kMessageFolder                                    = "messages";
  const std::string   kMessageFilenamePrefix                            = "message";
  const std::string   kMetadataFilename                                 = "message_metadata";
  const std::string   kMessageFileType                                  = ".wav";
  const float         kMessageRecordDuration                            = 10.0f;
}

VoiceMessageID VoiceMessageSystem::s_uniqueMessageID{ kInvalidVoiceMessageId };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageRecord::MessageRecord() :
  id( kInvalidVoiceMessageId ),
  status( EMessageStatus::Invalid )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::VoiceMessageSystem() :
  _robot( nullptr )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::~VoiceMessageSystem()
{
  _robot = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::Initialize( Robot* robot )
{
  #if REMOTE_CONSOLE_ENABLED
  {
    sVMSystem = this;
  }
  #endif

  using namespace Util;

  _robot = robot;

  // cache the name of our save directory
  const Data::DataPlatform* platform = _robot->GetContextDataPlatform();
  _saveFolder = platform->pathToResource( Data::Scope::Persistent, kMessageFolder );
  _saveFolder = FileUtils::AddTrailingFileSeparator( _saveFolder );

  // make sure our folder structure exists
  if ( FileUtils::DirectoryDoesNotExist( _saveFolder ) )
  {
    FileUtils::CreateDirectory( _saveFolder, false, true );
    PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "Creating message directory: %s", _saveFolder.c_str() );
  }

  LoadMessageSystem();

  // verify that the .wav file actually exists
  // if not, update our metadata to keep in consistent
  if ( VerifyAndSyncMessageRecords() )
  {
    Debug_PrintMessageRecords();
  }

  {
    using namespace RobotInterface;

    MessageHandler* messageHandler = _robot->GetRobotMessageHandler();
    AddSignalHandle( messageHandler->Subscribe( RobotToEngineTag::audioPlaybackEnd, [this]( const AnkiEvent<RobotToEngine>& event )
    {
      const AudioPlaybackEnd& audioEvent = event.GetData().Get_audioPlaybackEnd();
      OnPlaybackComplete( audioEvent.path );
    } ) );

    AddSignalHandle( messageHandler->Subscribe( RobotToEngineTag::micRecordingComplete, [this]( const AnkiEvent<RobotToEngine>& event )
    {
      const MicRecordingComplete& audioEvent = event.GetData().Get_micRecordingComplete();
      OnRecordingComplete( audioEvent.path );
    } ) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::SaveMessageSystem()
{
  Json::Value metadata;
  Json::Value messageArray( Json::ValueType::arrayValue );

  for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
  {
    Json::Value jsonMessage;
    jsonMessage["slot"] = static_cast<Json::UInt>(index);

    const MessageRecord& messageRecord = GetMessageRecordAtIndex( index );
    WriteMessageRecordToJson( messageRecord, jsonMessage );

    messageArray.append( jsonMessage );
  }

  metadata["message_data"] = messageArray;

  const std::string filename = ( _saveFolder + kMetadataFilename + ".json" );
  Util::FileUtils::WriteFile( filename, metadata.toStyledString() );

  Debug_PrintMessageRecords();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::LoadMessageSystem()
{
  // let's initialize all of our slots prior to load
  for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
  {
    _messages[index] = MessageRecord();
  }

  // read in our metadata
  const std::string filename = ( _saveFolder + kMetadataFilename + ".json" );
  const std::string fileContents = Util::FileUtils::ReadFile( filename );

  Json::Reader reader;
  Json::Value metadata;

  // if the file exists, we should have some content
  if ( !fileContents.empty() && reader.parse( fileContents, metadata ) )
  {
    // our json consists of an array of messages
    // load that into json then extract it into our message data structs
    const Json::Value& messageArray = metadata["message_data"];
    for ( const Json::Value& jsonMessage : messageArray )
    {
      const MessageIndex index = jsonMessage["slot"].asUInt();
      DEV_ASSERT( ( ( index >= 0 ) && ( index < kNumMessageSlots ) ), "VoiceMessageSystem" );

      MessageRecord& messageRecord = GetMessageRecordAtIndex( index );
      ReadMessageRecordFromJson( jsonMessage, messageRecord );

      // s_uniqueMessageID is simply a counter to give a unique id
      // make sure our id is at least as great as the highest stored id so make sure we don't duplicate ids
      if ( messageRecord.id > s_uniqueMessageID )
      {
        s_uniqueMessageID = messageRecord.id;
      }
    }
  }
  else
  {
    PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "Failed to load voice message metadata file: %s", filename.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::WriteMessageRecordToJson( const MessageRecord& message, Json::Value& value ) const
{
  value["handle"]     = message.fileHandle;
  value["name"]       = message.recipientName;
  value["id"]         = message.id;
  value["status"]     = static_cast<Json::UInt>(message.status);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::ReadMessageRecordFromJson( const Json::Value& value, MessageRecord& message ) const
{
  message.fileHandle      = value["handle"].asString();
  message.recipientName   = value["name"].asString();
  message.id              = value["id"].asUInt();
  message.status          = static_cast<EMessageStatus>(value["status"].asUInt());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VoiceMessageSystem::VerifyAndSyncMessageRecords()
{
  bool dataIsValid = true;

  for ( MessageRecord& message : _messages )
  {
    bool messageIsValid = true;

    // must have a file handle if we have a valid message status
    messageIsValid &= ( !message.HasMessage() || !message.fileHandle.empty() );
    // if we have a valid file handle, we must have a valid file on disk
    messageIsValid &= ( message.fileHandle.empty() || Util::FileUtils::FileExists( message.fileHandle ) );
    // I can't see how this is possible, but doesn't hurt to check
    messageIsValid &= ( !message.HasMessage() || ( message.id != kInvalidVoiceMessageId ) );

    if ( !messageIsValid )
    {
      PRINT_NAMED_ERROR( "VoiceMessageSystem", "Message records don't match files on disk (%s)", message.fileHandle.c_str() );
      DeleteMessageInternal( message );

      dataIsValid = false;
    }

    // note: we could also check for the case when we have a wav file with no metadata, but we'd have to default the metadata
    //       since we wont know anything about the message
  }

  // if we had to make some changes, save the metadata back to disk
  if ( !dataIsValid )
  {
    SaveMessageSystem();
  }

  return dataIsValid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uint32_t VoiceMessageSystem::GetDefaultRecordingMessageDurationMS() const
{
  return static_cast<uint32_t>(kMessageRecordDuration * 1000.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string VoiceMessageSystem::GetDefaultFilename( MessageIndex index ) const
{
  const std::string filename = kMessageFilenamePrefix + "_" + std::to_string( index );
  return ( _saveFolder + filename );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::FileHandle VoiceMessageSystem::GetDefaultFileHandle( MessageIndex index ) const
{
  const std::string filename = ( kMessageFilenamePrefix + "_" + std::to_string( index ) );
  const std::string intermediateFolder = Util::FileUtils::AddTrailingFileSeparator( _saveFolder + filename );
  return ( intermediateFolder + filename + kMessageFileType );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageIndex VoiceMessageSystem::GetMessageIndex( VoiceMessageID id ) const
{
  MessageIndex index = kInvalidSlot;

  for ( MessageIndex i = 0; i < kNumMessageSlots; ++i )
  {
    if ( id == _messages[i].id )
    {
      index = i;
      break;
    }
  }

  return index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageIndex VoiceMessageSystem::GetFreeMessageIndex() const
{
  MessageIndex freeSlot = kInvalidSlot;

  for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
  {
    if ( !_messages[index].HasMessage() )
    {
      freeSlot = index;
      break;
    }
  }

  return freeSlot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::CreateNewMessageInternal( MessageIndex index, const std::string& recipient )
{
  // this is where we're going to save our recording
  MessageRecord& message = GetMessageRecordAtIndex( index );

  if ( message.HasMessage() )
  {
    PRINT_NAMED_WARNING( "VoiceMessageSystem", "New voice message will overwrite existing unread message" );
    DeleteMessageInternal( message );
  }

  // wrap the id around if we've managed to get that far :)
  if ( s_uniqueMessageID == std::numeric_limits<VoiceMessageID>::max() )
  {
    s_uniqueMessageID = kInvalidVoiceMessageId;
  }

  {
    message.fileHandle      = GetDefaultFileHandle( index );
    message.recipientName   = Util::StringToLower( recipient ); // note: change this when we have the UserName class pulled out
    message.id              = ++s_uniqueMessageID;
    message.status          = EMessageStatus::Unread;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::DeleteMessage( VoiceMessageID id )
{
  const MessageIndex index = GetMessageIndex( id );
  if ( kInvalidSlot != index )
  {
    MessageRecord& message = GetMessageRecordAtIndex( index );
    DeleteMessageInternal( message );
  }

  SaveMessageSystem();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::DeleteMessageInternal( MessageRecord& message )
{
  PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "[Slot %d] Deleting message with id [%u]",
                  (int)GetMessageIndex( message.id ), message.id );

  if ( Util::FileUtils::FileExists( message.fileHandle ) )
  {
    Util::FileUtils::DeleteFile( message.fileHandle );
  }

  message = MessageRecord();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RecordMessageResult VoiceMessageSystem::StartRecordingMessage( const std::string& name,
                                                               uint32_t duration_ms,
                                                               OnCompleteCallback callback )
{
  using namespace RobotInterface;

  RecordMessageResult result;
  result.id = kInvalidVoiceMessageId;
  result.error = EVoiceMessageError::MailboxFull;

  const MessageIndex index = GetFreeMessageIndex();
  if ( kInvalidSlot != index )
  {
    if ( nullptr == _currentRecording.messageRecord )
    {
      CreateNewMessageInternal( index, name );
      const MessageRecord& messageRecord = GetMessageRecordAtIndex( index );

      // generate a filename (not the actual filename on disk since the anim process changes it)
      const std::string filename = GetDefaultFilename( index );

      // use default duration and location
      const uint32_t duration = ( ( duration_ms != 0 ) ? duration_ms : GetDefaultRecordingMessageDurationMS() );
      StartRecordingMicsProcessed micMessage( duration, filename );

      // send a message to the anim process to tell it to start recording
      _robot->SendMessage( EngineToRobot( std::move(micMessage) ) );

      _currentRecording.messageRecord = &messageRecord;
      _currentRecording.callback = std::move( callback );

      PRINT_CH_INFO( "VoiceMessage", "VoiceMessageSystem.Record",
                     "[Slot %d] Recording message with id [%u], filename [%s]",
                     (int)index, messageRecord.id, filename.c_str() );

      result.id = messageRecord.id;
      SaveMessageSystem();

      result.error = EVoiceMessageError::Success;
    }
    else
    {
      result.error = EVoiceMessageError::MessageAlreadyActive;
    }
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EVoiceMessageError VoiceMessageSystem::PlaybackRecordedMessage( VoiceMessageID messageId, OnCompleteCallback callback )
{
  EVoiceMessageError result = EVoiceMessageError::InvalidMessage;

  const MessageIndex index = GetMessageIndex( messageId );
  if ( kInvalidSlot != index )
  {
    if ( nullptr == _currentPlayback.messageRecord )
    {
      PRINT_CH_INFO( "VoiceMessage", "VoiceMessageSystem.Playback", "[Slot %d] Playing message with id [%u]", (int)index, messageId );

      MessageRecord& message = GetMessageRecordAtIndex( index );
      message.status = EMessageStatus::Read;

      RobotInterface::StartPlaybackAudio playbackMessage( message.fileHandle );
      _robot->SendMessage( RobotInterface::EngineToRobot( std::move(playbackMessage) ) );

      _currentPlayback.messageRecord = &message;
      _currentPlayback.callback = std::move( callback );

      // save out since our data has changed
      SaveMessageSystem();

      result = EVoiceMessageError::Success;
    }
    else
    {
      result = EVoiceMessageError::MessageAlreadyActive;
    }
  }
  else
  {
    PRINT_CH_INFO( "VoiceMessage", "VoiceMessageSystem.Playback", "Couldn't find message with id [%u]", messageId );
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::OnRecordingComplete( const std::string& path )
{
  if ( ( _currentRecording.messageRecord != nullptr ) && ( _currentRecording.messageRecord->fileHandle == path ) )
  {
    OnCompleteCallback callback = std::move( _currentRecording.callback );

    // delete this before we call the callback so that the user can fire off another recording from the callback
    _currentRecording = ActiveMessage();

    if ( callback )
    {
      callback();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::OnPlaybackComplete( const std::string& path )
{
  if ( ( _currentPlayback.messageRecord != nullptr ) && ( _currentPlayback.messageRecord->fileHandle == path ) )
  {
    OnCompleteCallback callback = std::move( _currentPlayback.callback );

    // delete this before we call the callback so that the user can fire off another recording from the callback
    _currentRecording = ActiveMessage();

    if ( callback )
    {
      callback();
    }

    _currentPlayback = ActiveMessage();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VoiceMessageSystem::IsSameRecipient( const std::string& lhs, const std::string& rhs ) const
{
  // note: convert to UserName object when implemented
  return Util::StringCaseInsensitiveEquals( lhs, rhs );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VoiceMessageSystem::HasUnreadMessages( const std::string& name ) const
{
  VoiceMessageList messages = GetUnreadMessages( name, SortType::None );
  return !messages.empty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageList VoiceMessageSystem::GetUnreadMessages( const std::string& name, SortType sortType ) const
{
  VoiceMessageList messages;

  const std::string recipient = Util::StringToLower( name );

  // grab all of the messages that are for the specified recipient
  for ( const auto& message : _messages )
  {
    const bool isUnread = ( EMessageStatus::Unread == message.status );
    const bool isRecipient = IsSameRecipient( recipient, message.recipientName );
    if ( isUnread && isRecipient )
    {
      messages.push_back( message.id );
    }
  }

  // luckily our message id's are guaranteed to be in chronological order, so simply sort by the id
  switch ( sortType )
  {
    case SortType::OldestToNewest:
      std::sort( messages.begin(), messages.end(), []( VoiceMessageID lhs, VoiceMessageID rhs )
      {
        return ( lhs < rhs );
      });
      break;

    case SortType::NewestToOldest:
      std::sort( messages.begin(), messages.end(), []( VoiceMessageID lhs, VoiceMessageID rhs )
      {
        return ( lhs > rhs );
      });
      break;

    case SortType::None:
      break;
  }

  return messages;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageUserList VoiceMessageSystem::GetUnreadMessages( SortType sortType ) const
{
  VoiceMessageUserList userMessages;

  // grab all of our unique names, then build our list of messages from that
  // note: we can easily speed this up by storing our messages by recipient, but with only 10 max messages
  //       this will be negligible
  std::vector<std::string> recipients;
  for ( const auto& message : _messages )
  {
    bool found = false;
    for ( const std::string& name : recipients )
    {
      if ( IsSameRecipient( name, message.recipientName ) )
      {
        found = true;
        break;
      }
    }

    if ( !found )
    {
      recipients.emplace_back( message.recipientName );
    }
  }

  // now recipients should contain all of our unique recipients
  for ( const std::string& name : recipients )
  {
    VoiceMessageUser user;

    user.recipient = name;
    user.messages = GetUnreadMessages( name, sortType );

    if ( !user.messages.empty() )
    {
      userMessages.push_back( user );
    }
  }

  return userMessages;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::Debug_PrintMessageRecords() const
{
  #if DEBUG_VOICESYSTEM
  {
    auto printMessage = []( int index, const MessageRecord& message )
    {
      if ( !message.HasMessage() )
      {
        PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "[Slot %d] Empty", index );
      }
      else
      {
        PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "[Slot %d] %s", index,
                       Util::FileUtils::GetFileName( message.fileHandle ).c_str() );
        PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "         Name: %s",
                        !message.recipientName.empty() ? message.recipientName.c_str() : "All" );
        PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "         ID: %u",
                        message.id );
        PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "         Status: %s",
                        ( message.status == EMessageStatus::Invalid ) ?
                        "Invalid" :
                        ( ( message.status == EMessageStatus::Unread ) ? "Unread" : "Read" ) );
      }
    };

    PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "********** Voice Message System **********" );
    for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
    {
      printMessage( (int)index, _messages[index] );
    }
    PRINT_CH_DEBUG( "VoiceMessage", "VoiceMessageSystem", "******************************************" );
  }
  #endif
}

} // namespace Cozmo
} // namespace Anki
