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

#if defined(COZMO_ROBOT)

#define MESSAGE_STRUCT_DEFINITION_MODE 0
#define MESSAGE_TABLE_DEFINITION_MODE 1
#define MESSAGE_ENUM_DEFINITION_MODE 2

#elif defined(COZMO_BASESTATION)

#define MESSAGE_CLASS_DEFINITION_MODE 0
#define MESSAGE_CLASS_CONSTRUCTOR_MODE 1
#define MESSAGE_TABLE_DEFINITION_MODE 2
#define MESSAGE_ENUM_DEFINITION_MODE 3
#define MESSAGE_ROBOT_PROCESSOR_METHOD_MODE 4
#define MESSAGE_PROCESS_BUFFER_METHODS_MODE 5

#else
#error Either COZMO_ROBOT or COZMO_BASESTATION should be defined!
#endif

// Explicitly undefine so we can redefine without warnings from previous includes
#undef START_MESSAGE_DEFINITION
#undef START_TIMESTAMPED_MESSAGE_DEFINITION
#undef ADD_MESSAGE_MEMBER
#undef ADD_MESSAGE_MEMBER_ARRAY
#undef END_MESSAGE_DEFINITION

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else

// Explicitly undefine so we can redefine without warnings from previous includes
#undef START_MESSAGE_DEFINITION
#undef START_TIMESTAMPED_MESSAGE_DEFINITION
#undef ADD_MESSAGE_MEMBER
#undef ADD_MESSAGE_MEMBER_ARRAY
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
// First Mode: Define typedef'd struct and the prototype for a ProcessMessage
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
//     void ProcessFooBarMessage(const FooBar& msg);
//
//   It also creates this inline wrapper, which you should not need to use
//   directly (this gets called by the main ProcessMessage function):
//
//     inline void ProcessFooBarMessage(const u8* buffer) {
//        ProcessFooBarMessage(*reinterpret_cast<const FooBar*>(buffer));
//     }
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

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __TYPE__ __NAME__[__LENGTH__];


//
// Second Mode: Define entry in LookupTable
//
//   For example:
//      {<priority>, sizeof(f32)+sizeof(u16)+0, ProcessFooBarMessage}
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
{__PRIORITY__, sizeof(GET_STRUCT_TYPENAME(__MSG_TYPE__)), GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
//#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
//#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +
//#define END_MESSAGE_DEFINITION(__MSG_TYPE__) + 0, GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) 
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)


//
// Third Mode: Define enumerated message ID
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

// Helper macros
#define GET_MESSAGE_CLASSNAME(__MSG_TYPE__) Message##__MSG_TYPE__
#define GET_DISPATCH_FCN_NAME(__MSG_TYPE__) ProcessBufferAs_Message##__MSG_TYPE__
#define GET_MESSAGE_ID(__MSG_TYPE__) __MSG_TYPE__##_ID

// Time-stamped message definiton (for now) just uses regular start-message
// macro and adds a special timestamp member at the beginning.
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ADD_MESSAGE_MEMBER(TimeStamp, timestamp)

//
// Define inherited Message class
//
//   For example:
//
//      class MessasgeFooBar : public Message
//      {
//      public:
//        MessageFooBar(const u8 *buffer);
//
//        static u8 GetSize() { return sizeof(MemberStruct); }
//        
//      protected:
//        struct MemberStruct {
//          f32 foo;
//          u16 bar;
//        } members_;
//      };
//
#if MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
class GET_MESSAGE_CLASSNAME(__MSG_TYPE__) : public Message \
{ \
public: \
  GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer); \
  static u8 GetSize() { return sizeof(MemberStruct); } \
\
protected: \
  struct MemberStruct {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) __TYPE__ __NAME__;
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __TYPE__ __NAME__[__LENGTH__];

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
  } members_; \
};


//
// Define Message Class Constructors
//
// For example:
//
//    MessageFooBar::MessageFooBar(const u8* buffer);
//    {
//      members_.foo = *(reinterpret_cast<const f32*>(buffer));
//      buffer += sizeof(f32);
//      members_.bar = *(reinterpret_cast<const u16*>(buffer));
//    }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_CONSTRUCTOR_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer) { 

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
members_.__NAME__ = *(reinterpret_cast<const __TYPE__*>(buffer)); \
buffer += sizeof(__TYPE__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
memcpy(members_.__NAME__, buffer, __LENGTH__*sizeof(__TYPE__)); \
buffer += __LENGTH__*sizeof(__TYPE__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the constructor function


//
// Define entry in LookupTable
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
{__PRIORITY__, GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetSize(), &MessageHandler::GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

//
// Define enumerated message ID
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_ENUM_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) GET_MESSAGE_ID(__MSG_TYPE__),
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)

//
// Define robot message processor prototypes
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_ROBOT_PROCESSOR_METHOD_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ReturnCode ProcessMessage(const GET_MESSAGE_CLASSNAME(__MSG_TYPE__)& msg);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)


//
// Create ProcessBufferAs_MessageX methods in MessageHandler class
//
//  Creates one of these for each message type:
//
//   ReturnCode ProcessBufferAs_MessageSetMotion(const RobotID_t robotID, const u8* buffer)
//   {
//      Robot *robot = robotMgr_->getRobotByID(robotID);
//
//      const MessageSetMotion msg(buffer);
//
//      return robot->ProcessMessage(msg);
//   }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_PROCESS_BUFFER_METHODS_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ReturnCode GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const RobotID_t robotID, const u8* buffer) \
{ \
  Robot *robot = robotMgr_->getRobotByID(robotID); \
  const GET_MESSAGE_CLASSNAME(__MSG_TYPE__) msg(buffer); \
  return robot->ProcessMessage(msg); \
}

#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)

//
// Unrecognized mode
//
#else
#error Invalid value for MESSAGE_DEFINITION_MODE
#endif

// Leave definition mode undefined for next include of MessageDefinitions.h
#undef MESSAGE_DEFINITION_MODE

#else
#error Either COZMO_ROBOT or COZMO_BASESTATION should be defined!
#endif // #if defined(COZMO_ROBOT) or defined(COZMO_BASESTATION)

#endif // #ifndef MESSAGE_DEFINITION_MODE
