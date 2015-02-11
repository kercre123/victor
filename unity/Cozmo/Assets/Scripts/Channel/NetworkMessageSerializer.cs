using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public class NetworkMessageSerializationException : Exception {
	
	public NetworkMessageSerializationException()
		: base()
	{
	}
	
	public NetworkMessageSerializationException(string message)
		: base(message)
	{
	}
	
	public NetworkMessageSerializationException(string message, Exception innerException)
		: base(message, innerException)
	{
	}
}

/// <summary>
/// A helper class that takes care of common message serializing.
/// </summary>
public static class NetworkMessageSerializer {

	public static void Serialize(ByteSerializer serializer, NetworkMessage message)
	{
		try {
			byte id = (byte)message.ID;
			serializer.Serialize(id);
			message.Serialize(serializer);
		}
		catch (IndexOutOfRangeException e) {
			throw new NetworkMessageSerializationException("Failed to serialize " + message.GetType ().FullName + ". Buffer not large enough.", e);
		}
	}

	public static void Deserialize(ByteSerializer serializer, int length, out NetworkMessage message)
	{
		byte id;
		try {
			serializer.Deserialize (out id);
		}
		catch (IndexOutOfRangeException e) {
			throw new NetworkMessageSerializationException("Failed to deserialize network message. Not enough data to even specify an id.", e);
		}

		message = NetworkMessageCreation.Allocate((int)id);
		if (message == null) {
			throw new NetworkMessageSerializationException("Failed to deserialize network message. Unknown message ID " + id.ToString() + ".");
		}

		try {
			message.Deserialize(serializer);
		}
		catch (IndexOutOfRangeException e) {
			throw new NetworkMessageSerializationException("Failed to deserialize network message of type " + message.GetType ().FullName + ". Not enough data to specify the full message.", e);
		}

		if (serializer.Index != length) {
			throw new NetworkMessageSerializationException("Failed to deserialize network message of type " + message.GetType ().FullName + ". Message too long.");
		}
	}
}

