/*
 * MessageDefMacros_Basestation.h 
 *
 *
 * Author: Andrew Stein
 * Date:   12/16/2013
 *
 *
 * Description:  Contains the macros used to create actual code for using the
 *               message definitions in MessageDefinitions.h on the Basestation.
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
#if defined(COZMO_ROBOT) || !defined(COZMO_BASESTATION)
#error If MessageDefMacros_Basestation.h is included, COZMO_ROBOT should not \
       be defined and COZMO_BASESTATION should.
#endif

// Define the available modes
#undef MESSAGE_CLASS_DEFINITION_MODE
#undef MESSAGE_CLASS_CONSTRUCTOR_MODE
#undef MESSAGE_CLASS_GETSIZE_MODE
#undef MESSAGE_CLASS_GETBYTES_MODE
#undef MESSAGE_TABLE_DEFINITION_MODE
#undef MESSAGE_ENUM_DEFINITION_MODE
#undef MESSAGE_PROCESS_METHODS_MODE

#define MESSAGE_CLASS_DEFINITION_MODE        0
#define MESSAGE_CLASS_CONSTRUCTOR_MODE       1
#define MESSAGE_CLASS_GETSIZE_MODE           2
#define MESSAGE_CLASS_GETBYTES_MODE          3
#define MESSAGE_TABLE_DEFINITION_MODE        4
#define MESSAGE_ENUM_DEFINITION_MODE         5
#define MESSAGE_PROCESS_METHODS_MODE         6

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else // We have a message definition mode set

// Helper macros
#undef GET_MESSAGE_CLASSNAME
#undef GET_DISPATCH_FCN_NAME
#undef GET_MESSAGE_ID
#define GET_MESSAGE_CLASSNAME(__MSG_TYPE__) Message##__MSG_TYPE__
#define GET_DISPATCH_FCN_NAME(__MSG_TYPE__) ProcessPacketAs_Message##__MSG_TYPE__
#define GET_MESSAGE_ID(__MSG_TYPE__) __MSG_TYPE__##_ID

// Time-stamped message definiton (for now) just uses regular start-message
// macro and adds a special timestamp member at the beginning.
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ADD_MESSAGE_MEMBER(TimeStamp_t, timestamp)

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
//        void GetBytes(u8* buffer) const;
//
//        inline f32 get_foo() const { return foo; }
//        inline u16 get_bar() const { return bar; }
//
//      protected:
//          f32 foo;
//          u16 bar;
//      };
//

#if MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_DEFINITION_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
class GET_MESSAGE_CLASSNAME(__MSG_TYPE__) : public Message \
{ \
public: \
  GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer); \
  static u8 GetSize(); \
  virtual void GetBytes(u8* buffer) const;

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
protected: __TYPE__ __NAME__; \
public:    inline __TYPE__ get_##__NAME__() const { return __NAME__; }
    
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
protected: __TYPE__ __NAME__[__LENGTH__]; \
public: \
inline const __TYPE__* get_##__NAME__() const { return __NAME__; } \
inline u8 get_##__NAME__##Length() const { return __LENGTH__; }

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) };


//
// Define Message Class Constructors
//
// For example:
//
//    MessageFooBar::MessageFooBar(const u8* buffer);
//    {
//      foo = *(reinterpret_cast<const f32*>(buffer));
//      buffer += sizeof(f32);
//      bar = *(reinterpret_cast<const u16*>(buffer));
//    }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_CONSTRUCTOR_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer) { 

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
__NAME__ = *(reinterpret_cast<const __TYPE__*>(buffer)); \
buffer += sizeof(__TYPE__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
memcpy(__NAME__, buffer, __LENGTH__*sizeof(__TYPE__)); \
buffer += __LENGTH__*sizeof(__TYPE__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the constructor function


//
// Define Message GetSize() method
//
// For example:
//
//   u8 MessageFooBar::GetSize() const {
//     return sizeof(f32) + sizeof(u16) + 0;
//   }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_GETSIZE_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
u8 GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetSize() { \
return

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0; } 


//
// Define GetBytes() method
//
// For example:
//
//    void MessageFooBar::GetBytes(u8* buffer) const {
//      *(reinterpret_cast<f32*>(buffer)) = foo;
//      buffer += sizeof(f32);
//      *(reinterpret_cast<u16*>(buffer)) = bar;
//      buffer += sizeof(u16);
//    }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_GETBYTES_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetBytes(u8* buffer) const {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
*(reinterpret_cast<__TYPE__*>(buffer)) = __NAME__; \
buffer += sizeof(__TYPE__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
memcpy(buffer, __NAME__, __LENGTH__*sizeof(__TYPE__)); \
buffer += __LENGTH__*sizeof(__TYPE__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the GetBytes() function



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
// Create ProcessBufferAs_MessageX wrapper in MessageHandler class, which
// just calls the corresponding ProcessMessage(MessageX method)
//
//  Creates both of these for each message type:
//
//   ReturnCode ProcessMessage(Robot* robot, const MessageFooBar& msg);
//
//   inline ReturnCode MessageHandler::ProcessBufferAs_MessageFooBar(Robot* robot,
//                                                                   const u8* buffer)
//   {
//      const MessageFooBar msg(buffer);
//      return ProcessMessage(robot, msg);
//   }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_PROCESS_METHODS_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ReturnCode ProcessMessage(Robot* robot, const GET_MESSAGE_CLASSNAME(__MSG_TYPE__)& msg); \
inline ReturnCode GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(Robot* robot, const u8* buffer) \
{ \
  const GET_MESSAGE_CLASSNAME(__MSG_TYPE__) msg(buffer); \
  return ProcessMessage(robot, msg); \
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

#endif // #ifndef MESSAGE_DEFINITION_MODE
