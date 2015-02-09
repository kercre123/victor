using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public partial class AdvertisementRegistrationMsg : NetworkMessage {
	public ushort port;
	public byte[] ip = new byte[18];
	public byte id;
	public byte protocol;
	public byte enableAdvertisement;

	public override int ID {
		get {
			return -1;
		}
	}

	public override void Serialize(byte[] buffer, ref int index)
	{
		SerializerUtility.Serialize (buffer, ref index, this.port);
		SerializerUtility.Serialize (buffer, ref index, this.ip);
		SerializerUtility.Serialize (buffer, ref index, this.id);
		SerializerUtility.Serialize (buffer, ref index, this.protocol);
		SerializerUtility.Serialize (buffer, ref index, this.enableAdvertisement);
	}

	public override void Deserialize(byte[] buffer, ref int index)
	{
		SerializerUtility.Deserialize (buffer, ref index, out this.port);
		SerializerUtility.Deserialize (buffer, ref index, this.ip);
		SerializerUtility.Deserialize (buffer, ref index, out this.id);
		SerializerUtility.Deserialize (buffer, ref index, out this.protocol);
		SerializerUtility.Deserialize (buffer, ref index, out this.enableAdvertisement);
	}
}

