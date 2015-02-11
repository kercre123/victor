using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// A message that can go over the wire. Keep serialization in sync with C++.
/// </summary>
public abstract class NetworkMessage {

	/// <summary>
	/// The ID of the message type.
	/// </summary>
	public abstract int ID { get; }

	/// <summary>
	/// Serializes the message in the given buffer, starting at and updating the given index.
	/// </summary>
	/// <param name="buffer">The buffer to serialize to.</param>
	/// <param name="index">The index specifying where to serialize to, updated to past the ending index.</param>
	public abstract void Serialize (ByteSerializer serializer);

	/// <summary>
	/// Deserializes a message of the current type from the given buffer, starting at and updating the given index.
	/// </summary>
	/// <param name="buffer">The buffer to deserialize from.</param>
	/// <param name="index">The index specifying where to deserialize from, updated to past the ending index..</param>
	public abstract void Deserialize (ByteSerializer serializer);

	public override string ToString ()
	{
		return GetType ().FullName;
	}
}
