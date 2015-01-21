#if !defined(COZMO_BASESTATION)
#error COZMO_BASESTATION should be defined when using MessageDefinitionsG2U.h
#endif

#include "anki/cozmo/shared/MessageDefMacros_Basestation.h"



// ObjectVisionMarker for telling the UI that an object
//  with specified ID was seen at a particular location in the image
START_MESSAGE_DEFINITION(G2U_RobotObservedObject, 1)
ADD_MESSAGE_MEMBER(u32, robotID)
ADD_MESSAGE_MEMBER(u32, objectID)
ADD_MESSAGE_MEMBER(u16, topLeft_x)
ADD_MESSAGE_MEMBER(u16, topLeft_y)
ADD_MESSAGE_MEMBER(u16, width)
ADD_MESSAGE_MEMBER(u16, height)
END_MESSAGE_DEFINITION(G2U_RobotObservedObject)

START_MESSAGE_DEFINITION(G2U_DeviceDetectedVisionMarker, 1)
ADD_MESSAGE_MEMBER(uint32_t, markerType)
ADD_MESSAGE_MEMBER(float, x_upperLeft)
ADD_MESSAGE_MEMBER(float, y_upperLeft)
ADD_MESSAGE_MEMBER(float, x_lowerLeft)
ADD_MESSAGE_MEMBER(float, y_lowerLeft)
ADD_MESSAGE_MEMBER(float, x_upperRight)
ADD_MESSAGE_MEMBER(float, y_upperRight)
ADD_MESSAGE_MEMBER(float, x_lowerRight)
ADD_MESSAGE_MEMBER(float, y_lowerRight)
END_MESSAGE_DEFINITION(G2U_DeviceDetectedVisionMarker)

START_MESSAGE_DEFINITION(G2U_PlaySound, 1)
ADD_MESSAGE_MEMBER_ARRAY(char, soundFilename, 128)
ADD_MESSAGE_MEMBER(u8, numLoops)
ADD_MESSAGE_MEMBER(u8, volume)
END_MESSAGE_DEFINITION(G2U_PlaySound)

START_MESSAGE_DEFINITION(G2U_StopSound, 1)
END_MESSAGE_DEFINITION(G2U_StopSound)

/*
START_MESSAGE_DEFINITION(G2U_Bogus, 1)
END_MESSAGE_DEFINITION(G2U_Bogus)
*/

// Let the UI know that a robot is advertising as available
START_MESSAGE_DEFINITION(G2U_RobotAvailable, 1)
ADD_MESSAGE_MEMBER(u32, robotID)
END_MESSAGE_DEFINITION(G2U_RobotAvailable)

START_MESSAGE_DEFINITION(G2U_UiDeviceAvailable, 1)
ADD_MESSAGE_MEMBER(u32, deviceID)
END_MESSAGE_DEFINITION(G2U_UiDeviceAvailable)
