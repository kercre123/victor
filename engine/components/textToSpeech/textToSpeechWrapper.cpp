/***********************************************************************************************************************
 *
 *  TextToSpeechWrapper
 *  Victor / Engine
 *
 *  Created by Jarrod Hatfield on 5/21/2018
 *
 **********************************************************************************************************************/


#include "components/textToSpeech/textToSpeechWrapper.h"
#include "components/textToSpeech/textToSpeechCoordinator.h"

#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"


#define PRINT_DEBUG( format, ... ) \
  PRINT_CH_DEBUG( "TextToSpeech", "TextToSpeechWrapper", format, ##__VA_ARGS__ )

#define PRINT_INFO( format, ... ) \
  PRINT_CH_INFO( "TextToSpeech", "TextToSpeechWrapper", format, ##__VA_ARGS__ )


namespace Anki {
namespace Cozmo {

namespace {

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechWrapper::TextToSpeechWrapper( UtteranceTriggerType triggerType ) :
  _ttsCoordinator( nullptr ),
  _state( UtteranceState::Invalid ),
  _id( kInvalidUtteranceID ),
  _triggerType( triggerType )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechWrapper::~TextToSpeechWrapper()
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechWrapper::Initialize( TextToSpeechCoordinator& ttsCoordinator )
{
  _ttsCoordinator = &ttsCoordinator;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechWrapper::Reset()
{
  if ( nullptr != _ttsCoordinator )
  {
    CancelUtterance();
  }

  _state = UtteranceState::Invalid;
  _id = kInvalidUtteranceID;

  _readyCallback = nullptr;
  _finishedCallback = nullptr;

  // keep the _ttsCoordinator
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechWrapper::OnUtteranceUpdated( const UtteranceState& state )
{
  const UtteranceState previousState = _state;
  if ( state != previousState )
  {
    _state = state;
    PRINT_DEBUG( "Transition from %d state to %d state", (int)previousState, (int)_state );

    switch ( _state )
    {
      case UtteranceState::Invalid:
      {
        // if we're now invalid and have a "ready callback", then we must have failed during prep
        // if we have a "finished callback", we must have been playing, so call it
        if ( _readyCallback )
        {
          _readyCallback( false );
          _readyCallback = nullptr;
          _finishedCallback = nullptr;
        }
        else if ( _finishedCallback )
        {
          _finishedCallback( false );
          _finishedCallback = nullptr;
          _readyCallback = nullptr;
        }

        break;
      }

      case UtteranceState::Ready:
      {
        if ( _readyCallback )
        {
          _readyCallback( true );
          _readyCallback = nullptr;
        }

        break;
      }

      case UtteranceState::Finished:
      {
        if ( _finishedCallback )
        {
          _finishedCallback( true );
          _finishedCallback = nullptr;
        }

        break;
      }

      default:
      {
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechWrapper::SetUtteranceText( const std::string& text, OnUtteranceReadyCallback readyCallback, OnUtteranceCompleteCallback completeCallback )
{
  DEV_ASSERT( nullptr != _ttsCoordinator, "TextToSpeechWrapper.UnInitialized" );

  Reset();

  bool success = false;

  // we can only pre the utterance text from the Invalid state
  if ( UtteranceState::Invalid == _state )
  {
    PRINT_DEBUG( "Setting utterance text: %s", Util::HidePersonallyIdentifiableInfo( text.c_str()) );

    using AudioTTSProcessStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing;
    _id = _ttsCoordinator->CreateUtterance( text,
                                            _triggerType,
                                            AudioTTSProcessStyle::Default_Processed,
                                            1.0f,
                                            std::bind( &TextToSpeechWrapper::OnUtteranceUpdated, this, std::placeholders::_1 ));

    // if we were successful, set the callbcak
    if ( kInvalidUtteranceID != _id )
    {
      _readyCallback = std::move( readyCallback );
      _finishedCallback = std::move( completeCallback );
      success = true;
    }
    else if ( readyCallback )
    {
      readyCallback( false );
    }
  }

  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechWrapper::PlayUtterance( OnUtteranceCompleteCallback callback )
{
  DEV_ASSERT( nullptr != _ttsCoordinator, "TextToSpeechWrapper.UnInitialized" );
  DEV_ASSERT( UtteranceTriggerType::Manual == _triggerType, "TextToSpeechWrapper.InvalidTriggerType" );

  bool success = false;

  // we can only play the utterance after it has been generated
  if ( IsReadyToPlay() && ( kInvalidUtteranceID != _id ) )
  {
    PRINT_DEBUG( "Playing utterance id %d", (int)_id );

    if ( _ttsCoordinator->PlayUtterance( _id ) )
    {
      // if you double set the finished callback, you've probably done something wrong, however, things will still work fine
      ASSERT_NAMED_EVENT( !_finishedCallback,
                          "TextToSpeechWrapper.PlayUtterance",
                          "Specified OnUtteranceCompleteCallback when one was already set from SetUtteranceText(...)" );

      _finishedCallback = std::move( callback );
      success = true;
    }
    else if ( callback )
    {
      // fire off the failed callback
      callback( false );
    }
  }

  return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechWrapper::CancelUtterance()
{
  DEV_ASSERT( nullptr != _ttsCoordinator, "TextToSpeechWrapper.UnInitialized" );

  if ( ( UtteranceState::Invalid != _state ) && ( UtteranceState::Finished != _state ) && ( kInvalidUtteranceID != _id ) )
  {
    PRINT_DEBUG( "Cancelling utterance id %d", (int)_id );

    
    _ttsCoordinator->CancelUtterance( _id );
  }
}

} // namespace Cozmo
} // namespace Anki
