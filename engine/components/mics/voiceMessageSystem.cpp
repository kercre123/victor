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

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "json/json.h"

#include <chrono>


#define DEBUG_VOICESYSTEM 0


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if REMOTE_CONSOLE_ENABLED
  namespace {
    VoiceMessageSystem*             sVMSystem      = nullptr;
    VoiceMessageSystem::MessageID   sVMMessageID   = VoiceMessageSystem::kInvalidMessageID;
  }

  void RecordMessage( ConsoleFunctionContextRef context )
  {
    if ( nullptr != sVMSystem )
    {
      const char* name = ConsoleArg_GetOptional_String( context, "name", "" );
      sVMMessageID = sVMSystem->StartRecordingMessage( name );
    }
  }
  CONSOLE_FUNC( RecordMessage, "VoiceMessage", optional const char* name );

  void PlaybackMessage( ConsoleFunctionContextRef context )
  {
    const VoiceMessageSystem::MessageID id = ConsoleArg_GetOptional_UInt32( context, "messageId", sVMMessageID );
    if ( ( nullptr != sVMSystem ) && ( id != VoiceMessageSystem::kInvalidMessageID ) )
    {
      sVMSystem->PlaybackRecordedMessage( id );
    }
  }
  CONSOLE_FUNC( PlaybackMessage, "VoiceMessage", optional uint32 messageId );

  void PlayNextMessage( ConsoleFunctionContextRef context )
  {
    if ( nullptr != sVMSystem )
    {
      sVMSystem->PlaybackFirstUnreadMessage();
    }
  }
  CONSOLE_FUNC( PlayNextMessage, "VoiceMessage" );

  void DeleteMessage( ConsoleFunctionContextRef context )
  {
    const VoiceMessageSystem::MessageID id = ConsoleArg_GetOptional_UInt32( context, "messageId", sVMMessageID );
    if ( ( nullptr != sVMSystem ) && ( id != VoiceMessageSystem::kInvalidMessageID ) )
    {
      sVMSystem->DeleteMessage( id );
    }
  }
  CONSOLE_FUNC( DeleteMessage, "VoiceMessage", optional uint32 messageId );
#endif // REMOTE_CONSOLE_ENABLED


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  const std::string   kMessageFolder                                    = "messages";
  const std::string   kMessageFilenamePrefix                            = "message";
  const std::string   kMetadataFilename                                 = "message_metadata";
  const std::string   kMessageFileType                                  = ".wav";
  const float         kMessageRecordDuration                            = 10.0f;
}

VoiceMessageSystem::MessageID VoiceMessageSystem::s_uniqueMessageID{ kInvalidMessageID };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageRecord::MessageRecord() :
  id( kInvalidMessageID ),
  status( EMessageStatus::Invalid ),
  date( 0 )
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
    PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "Creating message directory: %s", _saveFolder.c_str() );
  }

  LoadMessageSystem();

  // verify that the .wav file actually exists
  // if not, update our metadata to keep in consistent
  if ( VerifyAndSyncMessageRecords() )
  {
    Debug_PrintMessageRecords();
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
    PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "Failed to load voice message metadata file: %s", filename.c_str() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::WriteMessageRecordToJson( const MessageRecord& message, Json::Value& value ) const
{
  value["handle"]     = message.fileHandle;
  value["name"]       = message.recipientName;
  value["date"]       = message.date;
  value["id"]         = message.id;
  value["status"]     = static_cast<Json::UInt>(message.status);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::ReadMessageRecordFromJson( const Json::Value& value, MessageRecord& message ) const
{
  message.fileHandle      = value["handle"].asString();
  message.recipientName   = value["name"].asString();
  message.date            = value["date"].asInt64();
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
    messageIsValid &= ( !message.HasMessage() || ( message.id != kInvalidMessageID ) );

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
uint32_t VoiceMessageSystem::GetRecordingMessageDurationMS() const
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
VoiceMessageSystem::MessageIndex VoiceMessageSystem::GetMessageIndex( MessageID id ) const
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
bool VoiceMessageSystem::CanRecordNewMessage() const
{
  const MessageIndex index = GetFreeMessageIndex();
  return ( index != kInvalidSlot );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VoiceMessageSystem::CreateNewMessageInternal( MessageIndex index, const std::string& recipient )
{
  // this is where we're going to save our recording
  MessageRecord& message = GetMessageRecordAtIndex( index );

  // note: move this deletion outside of creation and assert if we're overwriting a valid message.
  //       force the caller to explicitely delete the message prior to creating a new message.
  DeleteMessageInternal( message );

  // wrap the id around if we've managed to get that far :)
  if ( s_uniqueMessageID == std::numeric_limits<MessageID>::max() )
  {
    s_uniqueMessageID = kInvalidMessageID;
  }

  // note: system time doesn't persist between boots and may only be set when there's an internet connection.
  //       since we only use date to detemine creation order, simply store an incremented counter.
  //       actually, s_uniqueMessageID might already double as this
  {
    message.fileHandle      = GetDefaultFileHandle( index );
    message.recipientName   = recipient;
    message.date            = std::chrono::system_clock::now().time_since_epoch().count();
    message.id              = ++s_uniqueMessageID;
    message.status          = EMessageStatus::Unread;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::DeleteMessage( MessageID id )
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
  PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "[Slot %d] Deleting message with id [%u]",
                  (int)GetMessageIndex( message.id ), message.id );

  if ( Util::FileUtils::FileExists( message.fileHandle ) )
  {
    Util::FileUtils::DeleteFile( message.fileHandle );
  }

  message = MessageRecord();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VoiceMessageSystem::MessageID VoiceMessageSystem::StartRecordingMessage( const std::string& name )
{
  using namespace RobotInterface;

  MessageID newMessageId = kInvalidMessageID;

  const MessageIndex index = GetFreeMessageIndex();
  if ( kInvalidSlot != index )
  {
    // note: should probably do this on a callback
    if ( CreateNewMessageInternal( index, name ) )
    {
      const MessageRecord& messageRecord = GetMessageRecordAtIndex( index );

      // generate a filename (not the actual filename on disk since the anim process changes it)
      const std::string filename = GetDefaultFilename( index );

      // use default duration and location
      const uint32_t duration = GetRecordingMessageDurationMS();
      StartRecordingMicsProcessed micMessage( duration, filename );

      // send a message to the anim process to tell it to start recording
      _robot->SendMessage( EngineToRobot( std::move(micMessage) ) );

      PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "[Slot %d] Recording message with id [%u], filename [%s]", (int)index, messageRecord.id, filename.c_str() );

      newMessageId = messageRecord.id;
      SaveMessageSystem();
    }
  }

  return newMessageId;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::PlaybackRecordedMessage( MessageID messageId )
{
  const MessageIndex index = GetMessageIndex( messageId );
  if ( kInvalidSlot != index )
  {
    PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "[Slot %d] Playing message with id [%u]", (int)index, messageId );

    MessageRecord& message = GetMessageRecordAtIndex( index );
    message.status = EMessageStatus::Read;

    RobotInterface::StartPlaybackAudio playbackMessage( message.fileHandle );
    _robot->SendMessage( RobotInterface::EngineToRobot( std::move(playbackMessage) ) );

    // save out since our data has changed
    SaveMessageSystem();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VoiceMessageSystem::PlaybackFirstUnreadMessage()
{
  MessageID unreadId = kInvalidMessageID;
  int64_t unreadDate = std::numeric_limits<int64_t>::max();

  // walk through our messages and grab the earliest unread message
  for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
  {
    const MessageRecord& record = _messages[index];

    const bool messageIsUnread = ( record.status == EMessageStatus::Unread );
    const bool messageIsEarliest = ( record.date < unreadDate );
    if ( messageIsUnread && messageIsEarliest )
    {
      unreadId    = record.id;
      unreadDate  = record.date;
    }
  }

  if ( kInvalidMessageID != unreadId )
  {
    PlaybackRecordedMessage( unreadId );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VoiceMessageSystem::HasUnreadMessages() const
{
  bool hasMessage = false;

  for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
  {
    const MessageRecord& record = _messages[index];
    hasMessage |= ( record.status == EMessageStatus::Unread );
  }

  return hasMessage;
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
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "[Slot %d] Empty", index );
      }
      else
      {
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "[Slot %d] %s", index,
                       Util::FileUtils::GetFileName( message.fileHandle ).c_str() );
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "         Name: %s",
                        !message.recipientName.empty() ? message.recipientName.c_str() : "All" );
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "         ID: %u",
                        message.id );
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "         Status: %s",
                        ( message.status == EMessageStatus::Invalid ) ?
                        "Invalid" :
                        ( ( message.status == EMessageStatus::Unread ) ? "Unread" : "Read" ) );
        PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "         Date: %lld",
                        message.date );
      }
    };

    PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "********** Voice Message System **********" );
    for ( MessageIndex index = 0; index < kNumMessageSlots; ++index )
    {
      printMessage( (int)index, _messages[index] );
    }
    PRINT_CH_DEBUG( "MicData", "VoiceMessageSystem", "******************************************" );
  }
  #endif
}

} // namespace Cozmo
} // namespace Anki
