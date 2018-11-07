/**
* File: alexaComponentTypes.h
*
* Author: ross
* Created: Nov 2 2018
*
* Description: Types related to alexa
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_AlexaComponentTypes_H__
#define __Engine_AiComponent_AlexaComponentTypes_H__
#pragma once

#include <unordered_map>

namespace Anki {
  
namespace AudioMetaData {
  namespace GameEvent {
    enum class GenericEvent : uint32_t;
  }
}
  
namespace Vector {

enum class AlexaUXState : uint8_t;
enum class AnimationTrigger : int32_t;

struct AlexaUXResponse
{
  AnimationTrigger animTrigger;
  AudioMetaData::GameEvent::GenericEvent audioEvent;
};

using AlexaResponseMap = std::unordered_map<AlexaUXState,AlexaUXResponse>;
  
}
}

#endif // __Engine_AiComponent_AlexaComponentTypes_H__
