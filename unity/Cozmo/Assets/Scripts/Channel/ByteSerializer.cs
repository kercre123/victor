using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Determines which architecture to use for byte swapping.
/// </summary>
public enum ByteSwappingArchitecture
{
	LittleEndian,
	BigEndian,
	Native,
}

/// <summary>
/// Serializes bytes to a given architecture.
/// </summary>
public class ByteSerializer
{

	private float[] floatBuffer = new float[1];
	private double[] doubleBuffer = new double[1];

	protected byte[] buffer = null;
	private int index = 0;

	private bool useLittleEndian;
	private bool needSwap;

	public byte[] ByteBuffer
	{
		get {
			return buffer;
		}

		set {
			buffer = value;
			index = 0;
		}
	}

	public int Index { get { return index; } }

	public ByteSerializer(ByteSwappingArchitecture architecture)
	{
		switch(architecture)
		{
			case ByteSwappingArchitecture.LittleEndian:
				useLittleEndian = true;
				needSwap = !BitConverter.IsLittleEndian;
				break;
			case ByteSwappingArchitecture.BigEndian:
				useLittleEndian = false;
				needSwap = BitConverter.IsLittleEndian;
				break;
			case ByteSwappingArchitecture.Native:
				useLittleEndian = BitConverter.IsLittleEndian;
				needSwap = false;
				break;
		}
	}

	public void Clear()
	{
		ByteBuffer = null;
	}

	public void Serialize(byte value)
	{
		buffer[index] = value;
		index += 1;
	}

	public void Serialize(ushort value)
	{
		if(useLittleEndian)
		{
			buffer[index] = (byte)value;
			buffer[index + 1] = (byte)(value >> 8);
		} else
		{
			buffer[index] = (byte)(value >> 8);
			buffer[index + 1] = (byte)value;
		}
		index += 2;
	}

	public void Serialize(uint value)
	{
		if(useLittleEndian)
		{
			buffer[index] = (byte)value;
			buffer[index + 1] = (byte)(value >> 8);
			buffer[index + 2] = (byte)(value >> 16);
			buffer[index + 3] = (byte)(value >> 24);
		} else
		{
			buffer[index] = (byte)(value >> 24);
			buffer[index + 1] = (byte)(value >> 16);
			buffer[index + 2] = (byte)(value >> 8);
			buffer[index + 3] = (byte)value;
		}

		index += 4;
	}

	public void Serialize(int value)
	{
		Serialize((uint)value);
	}

	public void Serialize(ulong value)
	{
		uint i0 = (uint)value;
		uint i1 = (uint)(value >> 32);
		if(useLittleEndian)
		{
			Serialize(i0);
			Serialize(i1);
		} else
		{
			Serialize(i1);
			Serialize(i0);
		}
	}

	public void Serialize(float value)
	{
		floatBuffer[0] = value;
		Buffer.BlockCopy(floatBuffer, 0, buffer, index, 4);
		
		Swap4();
		
		index += 4;
	}

	public void Serialize(double value)
	{
		doubleBuffer[0] = value;
		Buffer.BlockCopy(doubleBuffer, 0, buffer, index, 8);
		
		Swap8();
		
		index += 8;
	}

	public void Serialize(byte[] value)
	{
		Array.Copy(value, 0, buffer, index, value.Length);
		index += value.Length;
	}

	public void Serialize(char[] value)
	{
		for(int i = 0; i < value.Length; ++i)
		{
			buffer[index + i] = (byte)value[i];
		}
		index += value.Length;
	}

	public void Deserialize(out byte value)
	{
		value = buffer[index];
		index += 1;
	}

	public void Deserialize(out ushort value)
	{
		uint b0 = buffer[index];
		uint b1 = buffer[index + 1];

		if(useLittleEndian)
		{
			value = (ushort)(b0 | (b1 << 8));
		} else
		{
			value = (ushort)(b1 | (b0 << 8));
		}

		index += 2;
	}

	public void Deserialize(out uint value)
	{
		uint b0 = buffer[index];
		uint b1 = buffer[index + 1];
		uint b2 = buffer[index + 2];
		uint b3 = buffer[index + 3];
		if(useLittleEndian)
		{
			value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
		} else
		{
			value = b3 | (b2 << 8) | (b1 << 16) | (b0 << 24);
		}

		index += 4;
	}

	public void Deserialize(out int value)
	{
		uint unsigned;
		Deserialize(out unsigned);
		value = (int)unsigned;
	}

	public void Deserialize(out ulong value)
	{
		uint i0;
		uint i1;
		if(useLittleEndian)
		{
			Deserialize(out i0);
			Deserialize(out i1);
		} else
		{
			Deserialize(out i1);
			Deserialize(out i0);
		}
		value = i0 | (i1 << 32);

		index += 8;
	}

	public void Deserialize(out float value)
	{
		Swap4();
		
		Buffer.BlockCopy(buffer, index, floatBuffer, 0, 4);
		value = floatBuffer[0];
		
		Swap4();
		
		index += 4;
	}

	
	public void Deserialize(out double value)
	{
		Swap8();
		
		Buffer.BlockCopy(buffer, index, doubleBuffer, 0, 8);
		value = doubleBuffer[0];
		
		Swap8();
		
		index += 8;
	}

	public void Deserialize(byte[] value)
	{
		Array.Copy(buffer, index, value, 0, value.Length);
		index += value.Length;
	}

	public void Deserialize(char[] value)
	{
		for(int i = 0; i < value.Length; ++i)
		{
			value[i] = (char)buffer[index + i];
		}
		index += value.Length;
	}

	private void Swap(int offsetA, int offsetB)
	{
		int indexA = index + offsetA;
		int indexB = index + offsetB;

		byte swap = buffer[indexA];
		buffer[indexA] = buffer[indexB];
		buffer[indexB] = swap;
	}

	private void Swap4()
	{
		if(needSwap)
		{
			Swap(0, 3);
			Swap(1, 2);
		}
	}

	private void Swap8()
	{
		if(needSwap)
		{
			Swap(0, 7);
			Swap(1, 6);
			Swap(2, 5);
			Swap(3, 4);
		}
	}

	public static int GetSerializationLength(byte value)
	{
		return 1;
	}

	public static int GetSerializationLength(ushort value)
	{
		return 2;
	}

	public static int GetSerializationLength(uint value)
	{
		return 4;
	}

	public static int GetSerializationLength(int value)
	{
		return 4;
	}

	public static int GetSerializationLength(ulong value)
	{
		return 8;
	}

	public static int GetSerializationLength(float value)
	{
		return 4;
	}

	public static int GetSerializationLength(double value)
	{
		return 8;
	}

	public static int GetSerializationLength(byte[] value)
	{
		return value.Length;
	}

	public static int GetSerializationLength(char[] value)
	{
		return value.Length;
	}
}

