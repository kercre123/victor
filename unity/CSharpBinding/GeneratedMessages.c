
#define COZMO_CSHARP

#undef MESSAGE_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

using s8 = System.SByte;
using s16 = System.Int16;
using s32 = System.Int32;
using s64 = System.Int64;
using u8 = System.Byte;
using u16 = System.UInt16;
using u32 = System.UInt32;
using u64 = System.UInt64;
using f32 = System.Single;
using f64 = System.Double;

public enum NetworkMessageID {
  NoMessage = 0,
#define MESSAGE_DEFINITION_MODE MESSAGE_ENUM_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
}

#define MESSAGE_DEFINITION_MODE MESSAGE_CLASS_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

#define MESSAGE_DEFINITION_MODE MESSAGE_SERIALIZE_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

#define MESSAGE_DEFINITION_MODE MESSAGE_DESERIALIZE_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"

public static class NetworkMessageCreation {
  
  public static NetworkMessage Allocate(int messageTypeID) {
    switch ((NetworkMessageID)messageTypeID) {
      default:
        return null;
#define MESSAGE_DEFINITION_MODE MESSAGE_ALLOCATE_DEFINITION_MODE
#include "anki/cozmo/game/comms/messaging/UiMessageDefinitions.h"
    }
  }
}



