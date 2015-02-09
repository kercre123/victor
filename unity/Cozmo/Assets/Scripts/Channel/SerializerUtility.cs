using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Utility methods that serialize little-endian.
/// </summary>
public static class SerializerUtility {

	[ThreadStatic]
	private static float[] floatBuffer = new float[1];

	public static void Serialize(byte[] buffer, ref int index, byte value)
	{
		buffer [index++] = value;
	}
	
	public static void Serialize(byte[] buffer, ref int index, ushort value)
	{
		buffer [index++] = (byte)value;
		buffer [index++] = (byte)(value >> 8);
	}
	
	public static void Serialize(byte[] buffer, ref int index, uint value)
	{
		buffer [index++] = (byte)value;
		buffer [index++] = (byte)(value >> 8);
		buffer [index++] = (byte)(value >> 16);
		buffer [index++] = (byte)(value >> 24);
	}

	public static void Serialize(byte[] buffer, ref int index, int value)
	{
		Serialize (buffer, ref index, (uint)value);
	}

	public static void Serialize(byte[] buffer, ref int index, byte[] value)
	{
		Array.Copy (value, 0, buffer, index, value.Length);
		index += value.Length;
	}
	
	public static void Serialize(byte[] buffer, ref int index, float value)
	{
		floatBuffer [0] = value;
		Buffer.BlockCopy (floatBuffer, 0, buffer, index, 4);

		if (!BitConverter.IsLittleEndian) {
			Swap(buffer, index, index + 3);
			Swap(buffer, index + 1, index + 2);
		}

		index += 4;
	}
	
	public static void Deserialize(byte[] buffer, ref int index, out byte value)
	{
		value = buffer [index++];
	}
	
	public static void Deserialize(byte[] buffer, ref int index, out ushort value)
	{
		uint b0 = buffer [index++];
		uint b1 = buffer [index++];
		value = (ushort)(b0 | (b1 << 8));
	}
	
	public static void Deserialize(byte[] buffer, ref int index, out uint value)
	{
	    uint b0 = buffer [index++];
		uint b1 = buffer [index++];
		uint b2 = buffer [index++];
		uint b3 = buffer [index++];
		value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
	}
	
	public static void Deserialize(byte[] buffer, ref int index, out int value)
	{
		uint unsigned;
		Deserialize (buffer, ref index, out unsigned);
		value = (int)unsigned;
	}
	
	public static void Deserialize(byte[] buffer, ref int index, byte[] value)
	{
		Array.Copy (buffer, index, value, 0, value.Length);
		index += value.Length;
	}
	
	public static void Deserialize(byte[] buffer, ref int index, out float value)
	{
		if (!BitConverter.IsLittleEndian) {
			Swap(buffer, index, index + 3);
			Swap(buffer, index + 1, index + 2);
		}

		Buffer.BlockCopy (buffer, index, floatBuffer, 0, 4);
		value = floatBuffer [0];

		if (!BitConverter.IsLittleEndian) {
			Swap(buffer, index, index + 3);
			Swap(buffer, index + 1, index + 2);
		}

		index += 4;
	}

	private static void Swap(byte[] buffer, int indexA, int indexB)
	{
		byte swap = buffer [indexA];
		buffer [indexA] = buffer [indexB];
		buffer [indexB] = swap;
	}
}

