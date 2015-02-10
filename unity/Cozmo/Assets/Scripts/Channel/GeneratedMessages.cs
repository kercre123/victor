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
	SampleMsg,
}

public partial class SampleMsg : NetworkMessage {
	public u16 port;
	public byte[] ip = new u8[18];
	public u8 id;
	public u8 protocol;
	public u8 enableAdvertisement;
	
	public override int ID {
		get {
			return (int)NetworkMessageID.SampleMsg;
		}
	}
}

public partial class SampleMsg {
	
	public override void Serialize(byte[] buffer, ref int index)
	{
		SerializerUtility.Serialize (buffer, ref index, this.port);
		SerializerUtility.Serialize (buffer, ref index, this.ip);
		SerializerUtility.Serialize (buffer, ref index, this.id);
		SerializerUtility.Serialize (buffer, ref index, this.protocol);
		SerializerUtility.Serialize (buffer, ref index, this.enableAdvertisement);
	}
}

public partial class SampleMsg {
	
	public override void Deserialize(byte[] buffer, ref int index)
	{
		SerializerUtility.Deserialize (buffer, ref index, out this.port);
		SerializerUtility.Deserialize (buffer, ref index, this.ip);
		SerializerUtility.Deserialize (buffer, ref index, out this.id);
		SerializerUtility.Deserialize (buffer, ref index, out this.protocol);
		SerializerUtility.Deserialize (buffer, ref index, out this.enableAdvertisement);
	}
}

public static class NetworkMessageCreation {

	public static NetworkMessage Allocate(int messageTypeID) {
		switch ((NetworkMessageID)messageTypeID) {
		default:
			return null;
		case NetworkMessageID.SampleMsg:
			return new SampleMsg();
		}
	}
}



