/**
* File: cozmoAnim/showAudioStreamStateManager.cpp
*
* Author: Kevin M. Karol
* Created: 8/3/2018
*
* Description: Communicates the current state of cloud audio streaming to the user
* and ensures expectations of related animation components are met (e.g. motion/lack there of when streaming)
*
* Copyright: Anki, Inc. 2018
**/

#include "cozmoAnim/showAudioStreamStateManager.h"

#include "clad/types/alexaTypes.h"
#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/robotDataLoader.h"
#include "util/string/stringUtils.h"

#include "audioEngine/audioTypeTranslator.h"

namespace Anki {
namespace Vector {

ShowAudioStreamStateManager::ShowAudioStreamStateManager(const AnimContext* context)
: _context(context)
{

}


ShowAudioStreamStateManager::~ShowAudioStreamStateManager()
{

}

void ShowAudioStreamStateManager::Update()
{
  {
    std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
    if(_havePendingTriggerResponse)
    {
      if(_pendingTriggerResponseHasGetIn)
      {
        StartTriggerResponseWithGetIn(_responseCallback);
      }
      else
      {
        StartTriggerResponseWithoutGetIn(_responseCallback);
      }

      _havePendingTriggerResponse = false;
      _responseCallback = nullptr;
    }
  }
}

void ShowAudioStreamStateManager::SetTriggerWordResponse(const RobotInterface::SetTriggerWordResponse& msg)
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  _postAudioEvent = msg.postAudioEvent;
  _shouldTriggerWordStartStream = msg.shouldTriggerWordStartStream;
  _shouldTriggerWordSimulateStream = msg.shouldTriggerWordSimulateStream;
  _getInAnimationTag = msg.getInAnimationTag;
  _getInAnimName = std::string(msg.getInAnimationName, msg.getInAnimationName_length);
}

void ShowAudioStreamStateManager::SetPendingTriggerResponseWithGetIn(OnTriggerAudioCompleteCallback callback)
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  if(_havePendingTriggerResponse)
  {
    PRINT_NAMED_WARNING("ShowAudioStreamStateManager.SetPendingTriggerResponseWithGetIn.ExistingResponse",
                        "Already have pending trigger response, overriding");
  }
  _havePendingTriggerResponse = true;
  _pendingTriggerResponseHasGetIn = true;
  _responseCallback = callback;
}

void ShowAudioStreamStateManager::SetPendingTriggerResponseWithoutGetIn(OnTriggerAudioCompleteCallback callback)
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  if(_havePendingTriggerResponse)
  {
    PRINT_NAMED_WARNING("ShowAudioStreamStateManager.SetPendingTriggerResponseWithoutGetIn.ExistingResponse",
                        "Already have pending trigger response, overriding");
  }
  _havePendingTriggerResponse = true;
  _pendingTriggerResponseHasGetIn = false;
  _responseCallback = callback;
}


void ShowAudioStreamStateManager::StartTriggerResponseWithGetIn(OnTriggerAudioCompleteCallback callback)
{
  if(!HasValidTriggerResponse()){
    if(callback){
      callback(false);
    }
    return;
  }

  auto* anim = _context->GetDataLoader()->GetCannedAnimation(_getInAnimName);
  if((_streamer != nullptr) && (anim != nullptr)){
    _streamer->SetStreamingAnimation(_getInAnimName, _getInAnimationTag);
  }else{
    PRINT_NAMED_ERROR("ShowAudioStreamStateManager.StartTriggerResponseWithGetIn.NoValidGetInAnimation",
                      "Animation not found for get in %s", _getInAnimName.c_str());
  }
  StartTriggerResponseWithoutGetIn(std::move(callback));
}


void ShowAudioStreamStateManager::StartTriggerResponseWithoutGetIn(OnTriggerAudioCompleteCallback callback)
{
  using namespace AudioEngine;

  if(!HasValidTriggerResponse()){
    if(callback){
      callback(false);
    }
    return;
  }

  Audio::CozmoAudioController* controller = _context->GetAudioController();
  if(nullptr != controller){
    AudioCallbackContext* audioCallbackContext = nullptr;
    if(callback){
      audioCallbackContext = new AudioCallbackContext();
      audioCallbackContext->SetCallbackFlags( AudioCallbackFlag::Complete );
      audioCallbackContext->SetExecuteAsync( false ); // Execute callbacks synchronously (on main thread)
      audioCallbackContext->SetEventCallbackFunc([callbackFunc = std::move(callback)]
                                                 (const AudioCallbackContext* thisContext, const AudioCallbackInfo& callbackInfo)
      {
        callbackFunc(true);
      });
    }

    controller->PostAudioEvent(ToAudioEventId(_postAudioEvent.audioEvent),
                               ToAudioGameObject(_postAudioEvent.gameObject),
                               audioCallbackContext);
  }
  else
  {
    // even though we don't have a valid audio controller, we still had a valid trigger response so return true
    if(callback){
      callback(true);
    }
  }
}


bool ShowAudioStreamStateManager::HasValidTriggerResponse()
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  return _postAudioEvent.audioEvent != AudioMetaData::GameEvent::GenericEvent::Invalid;
}


bool ShowAudioStreamStateManager::ShouldStreamAfterTriggerWordResponse()
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  return HasValidTriggerResponse() && _shouldTriggerWordStartStream;
}

bool ShowAudioStreamStateManager::ShouldSimulateStreamAfterTriggerWord()
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  return HasValidTriggerResponse() && _shouldTriggerWordSimulateStream;
}

void ShowAudioStreamStateManager::SetAlexaUXResponses(const RobotInterface::SetAlexaUXResponses& msg)
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex); // HasAnyAlexaResponse may be called off thread
  
  _alexaResponses.clear();
  const std::string csvResponses{msg.csvGetInAnimNames, msg.csvGetInAnimNames_length};
  const std::vector<std::string> animNames = Util::StringSplit(csvResponses, ',');
  int maxAnims = 3;
  if( !ANKI_VERIFY(animNames.size() == 3,
                   "ShowAudioStreamStateManager.SetAlexaUXResponses.UnexpectedCnt",
                   "Expecting 3 anim names, received %zu",
                   animNames.size()) )
  {
    maxAnims = std::min((int)animNames.size(), 3);
  }
  static_assert( sizeof(msg.postAudioEvents) / sizeof(msg.postAudioEvents[0]) == 3, "Expected 3 elems" );
  static_assert( sizeof(msg.getInAnimTags) / sizeof(msg.getInAnimTags[0]) == 3, "Expected 3 elems" );
  for( int i=0; i<maxAnims; ++i ) {
    AlexaInfo info;
    info.state = static_cast<AlexaUXState>(i);
    info.audioEvent = msg.postAudioEvents[i];
    info.getInAnimTag = msg.getInAnimTags[i];
    info.getInAnimName = animNames[i];
    _alexaResponses.push_back( std::move(info) );
  }
}
  
bool ShowAudioStreamStateManager::HasAnyAlexaResponse() const
{
  std::lock_guard<std::recursive_mutex> lock(_triggerResponseMutex);
  
  for( const auto& info : _alexaResponses ) {
    if( info.getInAnimTag != 0 ) {
      return true;
    }
  }
  return false;
}
  
bool ShowAudioStreamStateManager::HasValidAlexaUXResponse(AlexaUXState state) const
{
  for( const auto& info : _alexaResponses ) {
    if( info.state == state ) {
      // unlike wake word responses, which are valid if there is an audio event, Alexa UX responses are valid if a
      // nonzero anim tag was provided.
      return (info.getInAnimTag != 0);
    }
  }
  return false;
}
  
bool ShowAudioStreamStateManager::StartAlexaResponse(AlexaUXState state)
{
  const AlexaInfo* response = nullptr;
  for( const auto& info : _alexaResponses ) {
    // unlike wake word responses, which are valid if there is an audio event, Alexa UX responses are valid if a
    // nonzero anim tag was provided.
    if( (info.state == state) && (info.getInAnimTag != 0) ) {
      response = &info;
    }
  }
  
  if( response == nullptr ) {
    return false;
  }
  
  if( !response->getInAnimName.empty() ) {
    // TODO: (VIC-11516) it's possible that the UX state went back to idle for just a short while, in
    // which case the engine could be playing the get-out from the previous UX state, or worse, is
    // still in the looping animation for that ux state. it would be nice if the get-in below only
    // plays if the eyes are showing.
    
    auto* anim = _context->GetDataLoader()->GetCannedAnimation( response->getInAnimName );
    if( ANKI_VERIFY( (_streamer != nullptr) && (anim != nullptr),
                     "ShowAudioStreamStateManager.StartAlexaResponse.NoValidGetInAnim",
                     "Animation not found for get in %s", response->getInAnimName.c_str() ) )
    {
      _streamer->SetStreamingAnimation( response->getInAnimName, response->getInAnimTag );
    }
  }
  
  Audio::CozmoAudioController* controller = _context->GetAudioController();
  if( ANKI_VERIFY(nullptr != controller, "ShowAudioStreamStateManager.StartAlexaResponse.NullAudioController",
                  "The CozmoAudioController is null so the audio event cannot be played" ) )
  {
    using namespace AudioEngine;
    controller->PostAudioEvent( ToAudioEventId( response->audioEvent.audioEvent ),
                                ToAudioGameObject( response->audioEvent.gameObject ) );
    
  }
  
  return true;
}

} // namespace Vector
} // namespace Anki
