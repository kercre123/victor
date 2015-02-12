/*
 * MessageDefMacros_CSharp.h
 *
 *
 * Author: Greg Nagel
 * Date:   2/10/2015
 *
 *
 * Description:  Contains the macros used to create actual code for using the
 *               message definitions in MessageDefinitions.h on the CSharp plugin.
 *
 * Copyright 2015, Anki, Inc.
 */

// Explicitly undefine so we can redefine without warnings from previous includes
#undef START_MESSAGE_DEFINITION
#undef START_TIMESTAMPED_MESSAGE_DEFINITION
#undef ADD_MESSAGE_MEMBER
#undef ADD_MESSAGE_MEMBER_ARRAY
#undef END_MESSAGE_DEFINITION


#ifndef MESSAGE_DEFINITION_MODE

// Make sure the Robot vs. Basestation #defines are as expected
#if !defined(COZMO_CSHARP)
#error If MessageDefMacros_CSharp.h is included, COZMO_CSHARP should be defined
#endif

#define MESSAGE_ENUM_DEFINITION_MODE          0
#define MESSAGE_CLASS_DEFINITION_MODE         1
#define MESSAGE_LENGTH_DEFINITION_MODE        2
#define MESSAGE_SERIALIZE_DEFINITION_MODE     3
#define MESSAGE_DESERIALIZE_DEFINITION_MODE   4
#define MESSAGE_ALLOCATE_DEFINITION_MODE      5

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__)
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#else // We have a message definition mode set

#define START_TIMESTAMPED_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY) \
ADD_MESSAGE_MEMBER(u32, timestamp)

#if MESSAGE_DEFINITION_MODE == MESSAGE_ENUM_DEFINITION_MODE

// SampleMsg,

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) __MSG_TYPE__,
#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

#elif MESSAGE_DEFINITION_MODE == MESSAGE_CLASS_DEFINITION_MODE

//public partial class SampleMsg : NetworkMessage {
//  public u16 port;
//  public byte[] ip = new u8[18];
//  public u8 id;
//  public u8 protocol;
//  public u8 enableAdvertisement;
//  
//  public override int ID {
//    get {
//      return (int)NetworkMessageID.SampleMsg;
//    }
//  }
//}

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
public partial class __MSG_TYPE__ : NetworkMessage {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
  public __TYPE__ __NAME__;

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
  public __TYPE__[] __NAME__ = new __TYPE__[__LENGTH__];

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
  public override int ID { \
    get { \
      return (int)NetworkMessageID.__MSG_TYPE__; \
    } \
  } \
}

#elif MESSAGE_DEFINITION_MODE == MESSAGE_LENGTH_DEFINITION_MODE

//public partial class SampleMsg {
//  public override int SerializationLength
//  {
//    get {
//      return
//      ByteSerializer.GetSerializationLength(this.port) +
//      ByteSerializer.GetSerializationLength(this.ip) +
//      ByteSerializer.GetSerializationLength(this.id) +
//      ByteSerializer.GetSerializationLength(this.protocol) +
//      ByteSerializer.GetSerializationLength(this.enableAdvertisement) +
//      0;
//    }
//  }
//}

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
public partial class __MSG_TYPE__ { \
  public override int SerializationLength \
  { \
    get { \
      return

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
        ByteSerializer.GetSerializationLength(this.__NAME__) +

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
        ByteSerializer.GetSerializationLength(this.__NAME__) +

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
        0; \
    } \
  } \
}

#elif MESSAGE_DEFINITION_MODE == MESSAGE_SERIALIZE_DEFINITION_MODE

//public partial class SampleMsg {
//  public override void Serialize(ByteSerializer serializer)
//  {
//    serializer.Serialize(this.port);
//    serializer.Serialize(this.ip);
//    serializer.Serialize(this.id);
//    serializer.Serialize(this.protocol);
//    serializer.Serialize(this.enableAdvertisement);
//  }
//}

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
public partial class __MSG_TYPE__ { \
  public override void Serialize(ByteSerializer serializer) \
  {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
    serializer.Serialize(this.__NAME__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
    serializer.Serialize(this.__NAME__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
  } \
}

#elif MESSAGE_DEFINITION_MODE == MESSAGE_DESERIALIZE_DEFINITION_MODE

//public partial class SampleMsg {
//  public override void Deserialize(ByteSerializer serializer)
//  {
//    serializer.Deserialize(out this.port);
//    serializer.Deserialize(this.ip);
//    serializer.Deserialize(out this.id);
//    serializer.Deserialize(out this.protocol);
//    serializer.Deserialize(out this.enableAdvertisement);
//  }
//}

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
public partial class __MSG_TYPE__ { \
  public override void Deserialize(ByteSerializer serializer) \
  {

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__) \
    serializer.Deserialize(out this.__NAME__);

#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__) \
    serializer.Deserialize(this.__NAME__);

#define END_MESSAGE_DEFINITION(__MSG_TYPE__) \
  } \
}

#elif MESSAGE_DEFINITION_MODE == MESSAGE_ALLOCATE_DEFINITION_MODE

//      case NetworkMessageID.SampleMsg:
//        return new SampleMsg();

#define START_MESSAGE_DEFINITION(__MSG_TYPE__, __PRIORITY__) \
      case NetworkMessageID.__MSG_TYPE__: \
        return new __MSG_TYPE__();

#define ADD_MESSAGE_MEMBER(__TYPE__, __NAME__)
#define ADD_MESSAGE_MEMBER_ARRAY(__TYPE__, __NAME__, __LENGTH__)
#define END_MESSAGE_DEFINITION(__MSG_TYPE__)

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
