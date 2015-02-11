using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public partial class AdvertisementRegistrationMsg : NetworkMessage {
	public ushort port;
	public byte[] ip = new byte[19];
	public byte id;
	public byte protocol;
	public byte enableAdvertisement;

	public override int ID {
		get {
			return -1;
		}
	}

	public override int SerializationLength
	{
		get {
			return
				ByteSerializer.GetSerializationLength(this.port) +
				ByteSerializer.GetSerializationLength(this.ip) +
				ByteSerializer.GetSerializationLength(this.id) +
				ByteSerializer.GetSerializationLength(this.protocol) +
				ByteSerializer.GetSerializationLength(this.enableAdvertisement) +
				0;
		}
	}

	public override void Serialize(ByteSerializer serializer)
	{
		serializer.Serialize (this.port);
		serializer.Serialize (this.ip);
		serializer.Serialize (this.id);
		serializer.Serialize (this.protocol);
		serializer.Serialize (this.enableAdvertisement);
	}

	public override void Deserialize(ByteSerializer serializer)
	{
		serializer.Deserialize (out this.port);
		serializer.Deserialize (this.ip);
		serializer.Deserialize (out this.id);
		serializer.Deserialize (out this.protocol);
		serializer.Deserialize (out this.enableAdvertisement);
	}
}

