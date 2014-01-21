/*
 * messageDefMacros.h (robot)
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


// =============================================================================
// ================================ ROBOT ======================================
// =============================================================================

#if 0
#pragma mark --- ROBOT MESSAGE MACROS ---
#endif

#ifdef COZMO_ROBOT

#ifdef COZMO_BASESTATION
#error Only one of COZMO_BASESTATION or COZMO_ROBOT should be defined!
#endif

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
// First Mode: Define typedef'd struct and ProcessMessage dispatch function(s)
//
#if MESSAGE_DEFINITION_MODE == MESSAGE_STRUCT_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) typedef struct {

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } GET_STRUCT_TYPENAME(__MSG_TYPE__); \
void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const __MSG_TYPE__& msg); \
inline void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer) { \
GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(*reinterpret_cast<const GET_STRUCT_TYPENAME(__MSG_TYPE__)*>(buffer)); \
}

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


// =============================================================================
// ============================= BASESTATION ===================================
// =============================================================================

#if 0
#pragma mark --- BASESTATION MESSAGE MACROS ---
#endif

#elif defined(COZMO_BASESTATION)

// TODO: Make these basestation-specific
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
// First Mode: Define typedef'd struct and ProcessMessage dispatch function(s)
//
#if MESSAGE_DEFINITION_MODE == MESSAGE_STRUCT_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) typedef struct {

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } GET_STRUCT_TYPENAME(__MSG_TYPE__); \
void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const __MSG_TYPE__& msg); \
inline void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer) { \
GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(*reinterpret_cast<const GET_STRUCT_TYPENAME(__MSG_TYPE__)*>(buffer)); \
}

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


#else
#error Either COZMO_ROBOT or COZMO_BASESTATION should be defined!
#endif // #if defined(COZMO_ROBOT) or defined(COZMO_BASESTATION)

#endif // #ifndef MESSAGE_DEFINITION_MODE
