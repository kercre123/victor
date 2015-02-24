/*
 * MessageDefMacros_Robot.h 
 *
 *
 * Author: Andrew Stein
 * Date:   12/16/2013
 *
 *
 * Description:  Contains the macros used to create actual code for using the
 *               message definitions in MessageDefinitions.h on the robot.
 *
 * Copyright 2013, Anki, Inc.
 */


// Explicitly undefine so we can redefine without warnings from previous includes
#undef START_MESSAGE_DEFINITION
#undef START_TIMESTAMPED_MESSAGE_DEFINITION
#undef ADD_MESSAGE_MEMBER
#undef ADD_MESSAGE_MEMBER_ARRAY
#undef END_MESSAGE_DEFINITION


#ifndef MESSAGE_DEFINITION_MODE

// Make sure the Robot vs. Basestation #defines are as expected
#if !defined(COZMO_ROBOT) || defined(COZMO_BASESTATION)
#error If MessageDefMacros_Robot.h is included, COZMO_ROBOT should \
       be defined and COZMO_BASESTATION should not.
#endif

// Define the available modes
#undef MESSAGE_STRUCT_DEFINITION_MODE
#undef MESSAGE_TABLE_DEFINITION_MODE
#undef MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE
#undef MESSAGE_ENUM_DEFINITION_MODE
#undef MESSAGE_DISPATCH_DEFINITION_MODE
#undef MESSAGE_REG_CALLBACK_METHODS_MODE
#undef MESSAGE_PROCESS_METHODS_MODE
#undef MESSAGE_SIZE_TABLE_DEFINITION_MODE
#undef MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE

#define MESSAGE_STRUCT_DEFINITION_MODE   0
#define MESSAGE_TABLE_DEFINITION_MODE    1
#define MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE 2
#define MESSAGE_ENUM_DEFINITION_MODE     3
#define MESSAGE_DISPATCH_DEFINITION_MODE 4
#define MESSAGE_REG_CALLBACK_METHODS_MODE           5
#define MESSAGE_PROCESS_METHODS_MODE                6
#define MESSAGE_SIZE_TABLE_DEFINITION_MODE          7
#define MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE  8

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else

// Helper macros
#undef GET_DISPATCH_FCN_NAME
#undef GET_STRUCT_TYPENAME
#undef GET_QUOTED_STRUCT_TYPENAME
#undef GET_MESSAGE_ID

#define GET_DISPATCH_FCN_NAME(__MSG_TYPE__) Process##__MSG_TYPE__##Message
#define GET_STRUCT_TYPENAME(__MSG_TYPE__) __MSG_TYPE__
#define GET_MESSAGE_ID(__MSG_TYPE__) __MSG_TYPE__##_ID
#define GET_REGISTER_FCN_NAME(__MSG_TYPE__) RegisterCallbackForMessage##__MSG_TYPE__
#define GET_CALLBACK_FCN_NAME(__MSG_TYPE__) CallbackForMessage##__MSG_TYPE__

// Time-stamped message definiton (for now) just uses regular start-message
// macro and adds a special timestamp member at the beginning.
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ADD_MESSAGE_MEMBER(TimeStamp_t, timestamp)

//
// Define typedef'd struct and the prototype for a ProcessMessage
// dispatch function for each message.  It is your job to _implement_ that
// dispatch function.
//
//   For example:
//
//     typedef struct {
//        f32 foo;
//        u16 bar;
//     } FooBar;
//
#if MESSAGE_DEFINITION_MODE == MESSAGE_STRUCT_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) typedef struct {

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } GET_STRUCT_TYPENAME(__MSG_TYPE__);

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) __TYPE__ __NAME__;

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __TYPE__ __NAME__[__LENGTH__];


//
// Define dispatch function for each message struct
//
//   For example:
//     void ProcessFooBarMessage(const FooBar& msg);
//
//   It also creates this inline wrapper, which you should not need to use
//   directly (this gets called by the main ProcessMessage function):
//
//     inline void ProcessFooBarMessage(const u8* buffer) {
//        ProcessFooBarMessage(*reinterpret_cast<const FooBar*>(buffer));
//     }
//

#elif MESSAGE_DEFINITION_MODE == MESSAGE_DISPATCH_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?


#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const __MSG_TYPE__& msg); \
inline void GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer) { \
GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(*reinterpret_cast<const GET_STRUCT_TYPENAME(__MSG_TYPE__)*>(buffer)); \
}

#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)



// Register callback functions
#elif MESSAGE_DEFINITION_MODE == MESSAGE_REG_CALLBACK_METHODS_MODE
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_REGISTER_FCN_NAME(__MSG_TYPE__)(std::function<void(GET_STRUCT_TYPENAME(__MSG_TYPE__) const&)> callbackFcn);
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)


#elif MESSAGE_DEFINITION_MODE == MESSAGE_PROCESS_METHODS_MODE
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
std::function<void(GET_STRUCT_TYPENAME(__MSG_TYPE__)const&)> GET_CALLBACK_FCN_NAME(__MSG_TYPE__) = nullptr; \
void GET_REGISTER_FCN_NAME(__MSG_TYPE__)(std::function<void(GET_STRUCT_TYPENAME(__MSG_TYPE__) const&)> callbackFcn) \
{ \
  GET_CALLBACK_FCN_NAME(__MSG_TYPE__) = callbackFcn; \
} \
Result GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer) \
{ \
  if (nullptr != GET_CALLBACK_FCN_NAME(__MSG_TYPE__)) { \
    GET_CALLBACK_FCN_NAME(__MSG_TYPE__)(*reinterpret_cast<const GET_STRUCT_TYPENAME(__MSG_TYPE__)*>(buffer)); \
    return RESULT_OK; \
  } \
  return RESULT_FAIL; \
}
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)


//
// Define entry in LookupTable
//
//   For example:
//      {<priority>, sizeof(f32)+sizeof(u16)+0, ProcessFooBarMessage}
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
{__PRIORITY__, //sizeof(GET_STRUCT_TYPENAME(__MSG_TYPE__)), GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0, GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
//#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
//#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
//#define END_MESSAGE_DEFINITION(__MSG_TYPE__)


//
// Define entry in LookupTable without message handler function pointer
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) {__PRIORITY__,
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0, 0},

//
// Define enumerated message ID
//
//   For example:
//      FooBar_ID,
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_ENUM_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) GET_MESSAGE_ID(__MSG_TYPE__),
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)

//
// Define entry in message size LookupTable
//
//   For example:
//      {<priority>, sizeof(f32)+sizeof(u16)+0}
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_SIZE_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) { __PRIORITY__,
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0},

//
// Define entry in dispatch function array
//
//   For example:
//      ProcessFooBarMessage,
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) GET_DISPATCH_FCN_NAME(__MSG_TYPE__),

//
// Unrecognized mode
//
#else
#error Invalid value for MESSAGE_DEFINITION_MODE
#endif


// Leave definition mode undefined for next include of MessageDefinitions.h
#ifndef SKIP_MESSAGE_DEFINITION_MODE_UNDEF
#undef MESSAGE_DEFINITION_MODE
#endif

#endif // #ifndef MESSAGE_DEFINITION_MODE
