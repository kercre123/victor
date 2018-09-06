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

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/audio/engineRobotAudioInput.h"
#include "cozmoAnim/audio/cozmoAudioController.h"
#include "cozmoAnim/robotDataLoader.h"

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
    PRINT_NAMED_WARNING("ShowAudioStreamStateManager.SetPendingTriggerResponseWithGetIn.ExisitingResponse",
                        "Already have pending trigger reponse, overridding");
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
    PRINT_NAMED_WARNING("ShowAudioStreamStateManager.SetPendingTriggerResponseWithoutGetIn.ExisitingResponse",
                        "Already have pending trigger reponse, overridding");
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


} // namespace Vector
} // namespace Anki
