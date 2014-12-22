#if !defined(COZMO_BASESTATION)
#error COZMO_BASESTATION should be defined when using MessageDefinitionsG2U.h
#endif

#include "anki/cozmo/shared/MessageDefMacros_Basestation.h"

// ObjectVisionMarker for telling the UI that a VisionMarker from an object
//  with specified ID was seen at a particular location in the image
START_MESSAGE_DEFINITION(G2U_ObjectVisionMarker, 1)
ADD_MESSAGE_MEMBER(u32, objectID)
ADD_MESSAGE_MEMBER(u16, topLeft_x)
ADD_MESSAGE_MEMBER(u16, topLeft_y)
ADD_MESSAGE_MEMBER(u16, topRight_x)
ADD_MESSAGE_MEMBER(u16, topRight_y)
ADD_MESSAGE_MEMBER(u16, bottomLeft_x)
ADD_MESSAGE_MEMBER(u16, bottomLeft_y)
ADD_MESSAGE_MEMBER(u16, bottomRight_x)
ADD_MESSAGE_MEMBER(u16, bottomRight_y)
END_MESSAGE_DEFINITION(G2U_ObjectVisionMarker)


START_MESSAGE_DEFINITION(G2U_PlaySound, 1)
ADD_MESSAGE_MEMBER_ARRAY(char, animationFilename, 128)
ADD_MESSAGE_MEMBER(u8, numLoops)
ADD_MESSAGE_MEMBER(u8, volume)
END_MESSAGE_DEFINITION(G2U_PlaySound)

START_MESSAGE_DEFINITION(G2U_StopSound, 1)
END_MESSAGE_DEFINITION(G2U_StopSound)