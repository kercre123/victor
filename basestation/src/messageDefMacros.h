/*
 * messageDefMacros.h (basestation)
 *
 *
 * Author: Andrew Stein
 * Date:   12/16/2013
 *
 *
 * Description:  Contains the macros used to create actual code for using the
 *               message definitions in MessageDefinitions.h, which is shared
 *               between basestation and robot.  The manner in which those
 *               definitions implement underlying code, however, can vary 
 *               between the two platforms.  Thus these macros should be 
 *               defined for each platform separately.
 *
 * Copyright 2013, Anki, Inc.
 */
 
 // TODO: reimplement these macros in a basestation-specific way
 
#ifndef MESSAGE_DEFINITION_MODE

#define MESSAGE_STRUCT_DEFINITION_MODE 0
#define MESSAGE_TABLE_DEFINITION_MODE 1
#define MESSAGE_ENUM_DEFINITION_MODE 2

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else

// Explicitly undefine so we can redefine without warnings from previous includes
#undef START_MESSAGE_DEFINITION
#undef START_TIMESTAMPED_MESSAGE_DEFINITION
#undef ADD_MESSAGE_MEMBER
#undef END_MESSAGE_DEFINITION

// Helper macros
#define GET_DISPATCH_FCN_NAME(__MSG_TYPE__) Process##__MSG_TYPE__##Message
#define GET_STRUCT_TYPENAME(__MSG_TYPE__) __MSG_TYPE__
#define GET_MESSAGE_ID(__MSG_TYPE__) __MSG_TYPE__##_ID

// Time-stamped message definiton (for now) just uses regular start-message
// macro and adds a special timestamp member at the beginning.
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ADD_MESSAGE_MEMBER(TimeStamp, timestamp)

//
// First Mode: Define typedef'd struct
//
#if MESSAGE_DEFINITION_MODE == MESSAGE_STRUCT_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer); \
typedef struct {

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } GET_STRUCT_TYPENAME(__MSG_TYPE__);
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) __TYPE__ __NAME__;


//
// Second Mode: Define entry in LookupTable
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
{__PRIORITY__,
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) + 0, GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},

//
// Third Mode: Define enumerated message ID
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_ENUM_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) GET_MESSAGE_ID(__MSG_TYPE__),
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)


//
// Unrecognized mode
//
#else
#error Invalid value for MESSAGE_DEFINITION_MODE
#endif

#endif // #ifndef MESSAGE_DEFINITION_MODE