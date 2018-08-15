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
#include "cozmoAnim/robotDataLoader.h"

namespace Anki {
namespace Vector {

ShowAudioStreamStateManager::ShowAudioStreamStateManager(const AnimContext* context)
: _context(context)
{
  
}


ShowAudioStreamStateManager::~ShowAudioStreamStateManager()
{

}


void ShowAudioStreamStateManager::SetTriggerWordResponse(const RobotInterface::SetTriggerWordResponse& msg)
{
  _postAudioEvent = msg.postAudioEvent;
  _shouldTriggerWordStartStream = msg.shouldTriggerWordStartStream;
  _getInAnimationTag = msg.getInAnimationTag;
  _getInAnimName = std::string(msg.getInAnimationName, msg.getInAnimationName_length);
}


void ShowAudioStreamStateManager::StartTriggerResponseWithGetIn()
{
  if(!HasValidTriggerResponse()){
    return;
  }

  auto* anim = _context->GetDataLoader()->GetCannedAnimation(_getInAnimName);
  if((_streamer != nullptr) && (anim != nullptr)){
    _streamer->SetStreamingAnimation(_getInAnimName, _getInAnimationTag);
  }else{
    PRINT_NAMED_ERROR("ShowAudioStreamStateManager.StartTriggerResponseWithGetIn.NoValidGetInAnimation",
                      "Animation not found for get in %s", _getInAnimName.c_str());
  }
  StartTriggerResponseWithoutGetIn();
}

void ShowAudioStreamStateManager::StartTriggerResponseWithoutGetIn()
{
  if(!HasValidTriggerResponse()){
    return;
  }
  _audioInput->HandleMessage(_postAudioEvent);
}


bool ShowAudioStreamStateManager::HasValidTriggerResponse() const
{
  return _postAudioEvent.audioEvent != AudioMetaData::GameEvent::GenericEvent::Invalid;
}


bool ShowAudioStreamStateManager::ShouldStreamAfterTriggerWordResponse() const 
{ 
  return HasValidTriggerResponse() && _shouldTriggerWordStartStream;
}


} // namespace Vector
} // namespace Anki
