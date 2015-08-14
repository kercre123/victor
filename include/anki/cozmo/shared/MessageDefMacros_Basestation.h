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

// This include gives us the QUOTE macro we use below
#include "anki/common/constantsAndMacros.h"

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
#undef MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE
#undef MESSAGE_CLASS_GETID_MODE
#undef MESSAGE_CLASS_GETSIZE_MODE
#undef MESSAGE_CLASS_GETBYTES_MODE
#undef MESSAGE_TABLE_DEFINITION_MODE
#undef MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE
#undef MESSAGE_ENUM_DEFINITION_MODE
#undef MESSAGE_PROCESS_METHODS_MODE
#undef MESSAGE_UI_REG_CALLBACK_METHODS_MODE
#undef MESSAGE_UI_PROCESS_METHODS_MODE
#undef MESSAGE_CREATE_JSON_MODE
#undef MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE
#undef MESSAGE_CREATEFROMJSON_LADDER_MODE
#undef MESSAGE_STRUCT_DEFINITION_MODE // For simple C-struct definitions
#undef MESSAGE_CLASS_DEFAULT_CONSTRUCTOR_MODE

#define MESSAGE_CLASS_DEFINITION_MODE         0
#define MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE 1
#define MESSAGE_CLASS_GETID_MODE              2
#define MESSAGE_CLASS_GETSIZE_MODE            3
#define MESSAGE_CLASS_GETBYTES_MODE           4
#define MESSAGE_TABLE_DEFINITION_MODE         5
#define MESSAGE_TABLE_DEFINITION_NO_FUNC_MODE 6
#define MESSAGE_ENUM_DEFINITION_MODE          7
#define MESSAGE_PROCESS_METHODS_MODE          8
#define MESSAGE_UI_REG_CALLBACK_METHODS_MODE  9
#define MESSAGE_UI_PROCESS_METHODS_MODE       10
#define MESSAGE_CREATE_JSON_MODE              11
#define MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE   12
#define MESSAGE_CREATEFROMJSON_LADDER_MODE    13
#define MESSAGE_STRUCT_DEFINITION_MODE        14
#define MESSAGE_PROCESS_METHODS_NO_WRAPPERS_MODE 15
#define MESSAGE_CLASS_DEFAULT_CONSTRUCTOR_MODE 16

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else // We have a message definition mode set

#if !defined(MESSAGE_BASECLASS_NAME) && MESSAGE_DEFINITION_MODE != MESSAGE_STRUCT_DEFINITION_MODE
#error MESSAGE_BASECLASS_NAME must be defined
#endif

// Helper macros
#undef GET_MESSAGE_CLASSNAME
#undef GET_QUOTED_MESSAGE_CLASSNAME
#undef GET_DISPATCH_FCN_NAME
#undef GET_MESSAGE_ID

//#define QUOTE(__ARG__) #__ARG__
//#define STR(__ARG__) QUOTE(__ARG__)

#define GET_MESSAGE_CLASSNAME(__MSG_TYPE__) Message##__MSG_TYPE__
#define GET_QUOTED_MESSAGE_CLASSNAME(__MSG_TYPE__) QUOTE(GET_MESSAGE_CLASSNAME(__MSG_TYPE__))
#define GET_DISPATCH_FCN_NAME(__MSG_TYPE__) ProcessPacketAs_Message##__MSG_TYPE__
#define GET_REGISTER_FCN_NAME(__MSG_TYPE__) RegisterCallbackForMessage##__MSG_TYPE__
#define GET_CALLBACK_FCN_NAME(__MSG_TYPE__) CallbackForMessage##__MSG_TYPE__
#define GET_MESSAGE_ID(__MSG_TYPE__) __MSG_TYPE__##_ID

// Time-stamped message definiton (for now) just uses regular start-message
// macro and adds a special timestamp member at the beginning.
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
ADD_MESSAGE_MEMBER(TimeStamp_t, timestamp)

//
// Define inherited Message class
// (Note that members are public, for simplicity)
//
//   For example:
//
//      class MessasgeFooBar : public Message
//      {
//      public:
//        MessageFooBar();
//        MessageFooBar(const u8 *buffer);
//        MessageFooBar(const Json::Value& root);
//
//        static u8 GetSize();
//
//        void GetBytes(u8* buffer) const;
//
//        Json::Value CreateJson() const;
//        Result FillFromJson(const Json::Value& root);
//
//        f32 foo;
//        u16 bar;
//      };

// OLD:
//        inline f32 get_foo() const { return foo; }
//        inline u16 get_bar() const { return bar; }
//
//      protected:
//          f32 foo;
//          u16 bar;
//      };
//

#if MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_DEFINITION_MODE

#ifndef JSON_CONFIG_H_INCLUDED
#error Json header must be included to use MESSAGE_CLASS_DEFINITION_MODE.
#endif

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
class GET_MESSAGE_CLASSNAME(__MSG_TYPE__) : public MESSAGE_BASECLASS_NAME \
{ \
public: \
  GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(); \
  GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer); \
  GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const Json::Value& root) { FillFromJson(root); } \
  virtual u8 GetID() const override; \
  virtual u16 GetSize() const override; \
  virtual u16 GetPaddedSize() const override; \
  virtual void GetBytes(u8* buffer) const override; \
  virtual Json::Value CreateJson() const override; \
  virtual Result FillFromJson(const Json::Value& root) override;

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
__TYPE__ __NAME__;
/*
protected: __TYPE__ __NAME__; \
public:    inline __TYPE__ get_##__NAME__() const { return __NAME__; }
  */

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
std::array<__TYPE__,__LENGTH__> __NAME__;

/*
protected: __TYPE__ __NAME__[__LENGTH__]; \
public: \
inline const __TYPE__* get_##__NAME__() const { return __NAME__; } \
inline u8 get_##__NAME__##Length() const { return __LENGTH__; }
*/
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) };

//
// Define Default Message Class Constructor
//
//   MessageFooBar::MessageFooBar()
//   {
//     _foo = {};
//     _bar = {};
//   }
//

#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_DEFAULT_CONSTRUCTOR_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GET_MESSAGE_CLASSNAME(__MSG_TYPE__)() {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) __NAME__ = {};

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __NAME__.fill({});


//memcpy(__NAME__, buffer, __LENGTH__*sizeof(__TYPE__));

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the constructor function

//
// Define Message Class Constructors from u8* buffers
//
// For example:
//
//    MessageFooBar::MessageFooBar(const u8* buffer);
//    {
//      {
//        f32 tmp;
//        memcpy(&tmp, buffer, sizeof(tmp));
//        foo = tmp;
//      }
//      buffer += sizeof(f32);
//      {
//        u16 tmp;
//        memcpy(&tmp, buffer, sizeof(tmp));
//        bar = tmp;
//      }
//      buffer += sizeof(u16);
//    }
//
// See here for why I'm using the tmp variables above:
//   http://www.splinter.com.au/what-do-do-with-excarmdaalign-on-an-iphone-ap/
//
//
// OLD Method using reinterpret_cast -- Dangerous, can cause memory alignment problems
//    MessageFooBar::MessageFooBar(const u8* buffer);
//    {
//      foo = *(reinterpret_cast<const f32*>(buffer));
//      buffer += sizeof(f32);
//      bar = *(reinterpret_cast<const u16*>(buffer));
//    }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_BUFFER_CONSTRUCTOR_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(const u8* buffer) { 

// These
//#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
//__NAME__ = *(reinterpret_cast<const __TYPE__*>(buffer)); \
//buffer += sizeof(__TYPE__);

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
{ __TYPE__ tmp; memcpy(&tmp, buffer, sizeof(tmp)); __NAME__ = tmp; } \
buffer += sizeof(__TYPE__);

//#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
//std::copy(reinterpret_cast<const __TYPE__*>(buffer), \
//          reinterpret_cast<const __TYPE__*>(buffer)+__LENGTH__, \
//          __NAME__.begin()); \
//buffer += __LENGTH__*sizeof(__TYPE__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
for(s32 i=0; i<__LENGTH__; ++i) { \
  memcpy(&(__NAME__[i]), buffer, sizeof(__TYPE__)); \
  buffer += sizeof(__TYPE__); \
}


//memcpy(__NAME__, buffer, __LENGTH__*sizeof(__TYPE__));

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the constructor function


//
// Define Message GetID() method
//
// For example:
//
//   u8 MessageFooBar::GetID() const {
//     return FooBar_ID; // via GET_MESSAGE_ID()
//   }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_GETID_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
u8 GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetID() const { \
  return GET_MESSAGE_ID(__MSG_TYPE__); \
}
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)

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
u16 GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetSize() const { \
return

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0; } \
u16 GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetPaddedSize() const { \
u16 size = GetSize(); \
u16 remainder = size % 2; \
if (size == 0) { \
  size = 1; \
} else if (remainder > 0) { \
  ++size; \
} \
return size; \
}
//
// Define GetBytes() method
//
// For example:
//
//    void MessageFooBar::GetBytes(u8* buffer) const {
//      memcpy(buffer, &foo, sizeof(foo));
//      buffer += sizeof(f32);
//      memcpy(buffer, &bar, sizeof(bar));
//      buffer += sizeof(u16);
//    }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_GETBYTES_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetBytes(u8* buffer) const {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
memcpy(buffer, &__NAME__, sizeof(__NAME__)); \
buffer += sizeof(__TYPE__);
//*(reinterpret_cast<__TYPE__*>(buffer)) = __NAME__; \ OLD!

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
memcpy(buffer, &(__NAME__[0]), __LENGTH__*sizeof(__TYPE__)); \
buffer += __LENGTH__*sizeof(__TYPE__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } // close the GetBytes() function



//
// Define entry in LookupTable
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_TABLE_DEFINITION_MODE


#ifndef MESSAGE_HANDLER_CLASSNAME
#error Must define a name for the message handling class when in MESSAGE_TABLE_DEFINITION_MODE. (e.g. MessageHandler)
#endif

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
{__PRIORITY__, //GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::GetSize(), &MessageHandler::GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) sizeof(__TYPE__) +
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __LENGTH__*sizeof(__TYPE__) +
#define END_MESSAGE_DEFINITION(__MSG_TYPE__) 0, &MESSAGE_HANDLER_CLASSNAME::GET_DISPATCH_FCN_NAME(__MSG_TYPE__)},
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
//   Result ProcessMessage(Robot* robot, const MessageFooBar& msg);
//
//   Result MessageHandler::ProcessBufferAs_MessageFooBar(Robot* robot,
//                                                        const u8* buffer)
//   {
//      const MessageFooBar msg(buffer);
//      return ProcessMessage(robot, msg);
//   }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_PROCESS_METHODS_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
Result ProcessMessage(Robot* robot, const GET_MESSAGE_CLASSNAME(__MSG_TYPE__)& msg); \
Result GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(Robot* robot, const u8* buffer) \
{ \
  const GET_MESSAGE_CLASSNAME(__MSG_TYPE__) msg(buffer); \
  return ProcessMessage(robot, msg); \
}

#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)


//
// This mode is the same as the above, but does not generated the wrapper which
// takes in the u8* buffer -- and uses a slightly different prototype:
//
//  void ProcessMessage(const MessageFooBar& msg);
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_PROCESS_METHODS_NO_WRAPPERS_MODE

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void ProcessMessage(const GET_MESSAGE_CLASSNAME(__MSG_TYPE__)& msg); \

#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)


//inline void GET_REGISTER_FCN_NAME(__MSG_TYPE__)(void(*fPtr)(RobotID_t, GET_MESSAGE_CLASSNAME(__MSG_TYPE__) const&))

#elif MESSAGE_DEFINITION_MODE == MESSAGE_UI_REG_CALLBACK_METHODS_MODE
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
void GET_REGISTER_FCN_NAME(__MSG_TYPE__)(std::function<void(GET_MESSAGE_CLASSNAME(__MSG_TYPE__) const&)> callbackFcn) \
{ \
  GET_CALLBACK_FCN_NAME(__MSG_TYPE__) = callbackFcn; \
}
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)



#elif MESSAGE_DEFINITION_MODE == MESSAGE_UI_PROCESS_METHODS_MODE
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
std::function<void(GET_MESSAGE_CLASSNAME(__MSG_TYPE__)const&)> GET_CALLBACK_FCN_NAME(__MSG_TYPE__) = nullptr; \
Result GET_DISPATCH_FCN_NAME(__MSG_TYPE__)(const u8* buffer) \
{ \
  if (nullptr != GET_CALLBACK_FCN_NAME(__MSG_TYPE__)) { \
    const GET_MESSAGE_CLASSNAME(__MSG_TYPE__) msg(buffer); \
    GET_CALLBACK_FCN_NAME(__MSG_TYPE__)(msg); \
    return RESULT_OK; \
  } \
  return RESULT_FAIL; \
}
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)




//
// Define Json Creation Functions
// (Note it's your responsibility to #include <json/json.h>
//
// For example:
//
//  Json:Value MessageFooBar::CreateJson() const {
//     Json::Value root;
//
//     root["Name"] = "FooBar";
//     root["foo"]  = this->foo;
//     root["bar"]  = this->bar;
//
//     return root;
//  }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CREATE_JSON_MODE

#ifndef JSON_CONFIG_H_INCLUDED
#error Json header must be included to use MESSAGE_CREATE_JSON_MODE.
#endif

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
Json::Value GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::CreateJson() const \
{ \
Json::Value root; \
root[QUOTE(Name)] = GET_QUOTED_MESSAGE_CLASSNAME(__MSG_TYPE__);

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) root[QUOTE(__NAME__)] = this->__NAME__;

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
for(int i=0; i<__LENGTH__; ++i) { \
root[QUOTE(__NAME__)].append(this->__NAME__[i]); \
}

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
return root; \
}

//
// Define Constructor from JSON file
// (Note it's your responsibility to #include <json/json.h> and JsonTools.h
//
// For example:
//
// Result MessageFooBar::FillFromJson(const Json::Value& root)
// {
//    if(not root.isMember("Name") || root["Name"].asString() != "FooBar") {
//      PRINT_NAMED_ERROR("MessageFooBar.FillFromJson", "No 'Name' member matching 'FooBar' found!\n");
//      return RESULT_FAIL;
//    }
//    if(not root.isMember("foo")) {
//      PRINT_NAMED_ERROR("MessageFooBar.FillFromJson", "No 'foo' member found!\n");
//      return RESULT_FAIL;
//    }
//    this->foo = root["foo"];
//    if(not root.isMember("bar")) {
//      PRINT_NAMED_ERROR("MessageFooBar.FillFromJson", "No 'bar' member found!\n");
//      return RESULT_FAIL;
//    }
//    this->bar = root["bar"];
//
//    return RESULT_OK;
// }
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE

#ifndef JSON_CONFIG_H_INCLUDED
#error Json header must be included to use MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE.
#endif

#ifndef _ANKICORETECH_COMMON_JSONTOOLS_H_
#error JsonTools header must be included to use MESSAGE_CLASS_JSON_CONSTRUCTOR_MODE.
#endif

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
Result GET_MESSAGE_CLASSNAME(__MSG_TYPE__)::FillFromJson(const Json::Value& root) { \
if(not root.isMember(QUOTE(Name)) || root[QUOTE(Name)].asString() != GET_QUOTED_MESSAGE_CLASSNAME(__MSG_TYPE__)) { \
  PRINT_NAMED_ERROR(QUOTE(GET_MESSAGE_CLASSNAME(__MSG_TYPE__).FillFromJson), QUOTE(No matching 'Name' member found!\n)); \
  return RESULT_FAIL; \
}

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
if(not JsonTools::GetValueOptional(root, QUOTE(__NAME__), this->__NAME__)) { \
  PRINT_NAMED_ERROR(QUOTE(GET_MESSAGE_CLASSNAME(__MSG_TYPE__).FillFromJson), QUOTE(No '%s' member found!\n), QUOTE(__NAME__)); \
  return RESULT_FAIL; \
}

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
if(not JsonTools::GetArrayOptional(root, QUOTE(__NAME__), this->__NAME__)) { \
  PRINT_NAMED_ERROR(QUOTE(GET_MESSAGE_CLASSNAME(__MSG_TYPE__).FillFromJson), QUOTE(No '%s' member found!\n), QUOTE(__NAME__)); \
  return RESULT_FAIL; \
}

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) return RESULT_OK; }


//
//    Create an if/else ladder for calling the appropriate constructor for
//    each message in a Json file, using the Name field.
//
//    Assumes that:
//
//      const Json::Value&  jsonRoot;
//      Message*            msg;
//      std::string         msgType;
//
//    are already defined.
//
//    Then implements the following kind of if/else ladder:
//
//        if("MessageFoo" == msgType) {
//          
//          msg = new MessageFoo(jsonRoot);
//          
//        } else if("MessageBar" == msgType) {
//          
//          msg = new MessageBar(jsonRoot);
//          
//        } else
//
//    Which should be followed by this kind of block to complete the final "else":
//
//        {
//          PRINT_NAMED_WARNING("Message.CreateFromJson.UnknownMessageType",
//                              "Encountered unknown Message type '%s' in Json file.\n",
//                              msgType.c_str());
//        } // if/else kfMsgType matches each string
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_CREATEFROMJSON_LADDER_MODE

#ifndef JSON_CONFIG_H_INCLUDED
#error Json header must be included to use MESSAGE_CREATEFROMJSON_LADDER_MODE.
#endif

#ifndef _ANKICORETECH_COMMON_JSONTOOLS_H_
#error JsonTools header must be included to use MESSAGE_CREATEFROMJSON_LADDER_MODE.
#endif

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
if(QUOTE(GET_MESSAGE_CLASSNAME(__MSG_TYPE__)) == msgType) { \
  msg = new GET_MESSAGE_CLASSNAME(__MSG_TYPE__)(jsonRoot); \
} else 


#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)


//
// Define simple C-style typedef'd struct
//
// This is for creating a simple C-struct with the same name in places where
// COZMO_BASESTATION is defined. Note that the struct name will conflict with the
// class name (both are "MessageFooBar"), so use carefully.
//
//   For example:
//
//     typedef struct {
//        f32 foo;
//        u16 bar;
//     } MessageFooBar;
//
#elif MESSAGE_DEFINITION_MODE == MESSAGE_STRUCT_DEFINITION_MODE
// TODO: Is it possible, using a macro, to verify the type sizes are correctly ordered?
#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) typedef struct {

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) } GET_MESSAGE_CLASSNAME(__MSG_TYPE__);

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) __TYPE__ __NAME__;

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) __TYPE__ __NAME__[__LENGTH__];


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
