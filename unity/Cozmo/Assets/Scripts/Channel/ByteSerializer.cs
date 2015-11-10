using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Determines which architecture to use for byte swapping.
/// </summary>
public enum ByteSwappingArchitecture {
  LittleEndian,
  BigEndian,
  Native,
}

/// <summary>
/// Serializes bytes to a given architecture.
/// </summary>
public class ByteSerializer {

  private float[] _FloatBuffer = new float[1];
  private double[] _DoubleBuffer = new double[1];

  protected byte[] _Buffer = null;
  private int _Index = 0;

  private bool _UseLittleEndian;
  private bool _NeedSwap;

  public byte[] ByteBuffer {
    get {
      return _Buffer;
    }

    set {
      _Buffer = value;
      _Index = 0;
    }
  }

  public int Index { get { return _Index; } }

  public ByteSerializer(ByteSwappingArchitecture architecture) {
    switch (architecture) {
    case ByteSwappingArchitecture.LittleEndian:
      _UseLittleEndian = true;
      _NeedSwap = !BitConverter.IsLittleEndian;
      break;
    case ByteSwappingArchitecture.BigEndian:
      _UseLittleEndian = false;
      _NeedSwap = BitConverter.IsLittleEndian;
      break;
    case ByteSwappingArchitecture.Native:
      _UseLittleEndian = BitConverter.IsLittleEndian;
      _NeedSwap = false;
      break;
    }
  }

  public void Clear() {
    ByteBuffer = null;
  }

  public void Serialize(byte value) {
    _Buffer[_Index] = value;
    _Index += 1;
  }

  public void Serialize(ushort value) {
    if (_UseLittleEndian) {
      _Buffer[_Index] = (byte)value;
      _Buffer[_Index + 1] = (byte)(value >> 8);
    }
    else {
      _Buffer[_Index] = (byte)(value >> 8);
      _Buffer[_Index + 1] = (byte)value;
    }
    _Index += 2;
  }

  public void Serialize(uint value) {
    if (_UseLittleEndian) {
      _Buffer[_Index] = (byte)value;
      _Buffer[_Index + 1] = (byte)(value >> 8);
      _Buffer[_Index + 2] = (byte)(value >> 16);
      _Buffer[_Index + 3] = (byte)(value >> 24);
    }
    else {
      _Buffer[_Index] = (byte)(value >> 24);
      _Buffer[_Index + 1] = (byte)(value >> 16);
      _Buffer[_Index + 2] = (byte)(value >> 8);
      _Buffer[_Index + 3] = (byte)value;
    }

    _Index += 4;
  }

  public void Serialize(int value) {
    Serialize((uint)value);
  }

  public void Serialize(ulong value) {
    uint i0 = (uint)value;
    uint i1 = (uint)(value >> 32);
    if (_UseLittleEndian) {
      Serialize(i0);
      Serialize(i1);
    }
    else {
      Serialize(i1);
      Serialize(i0);
    }
  }

  public void Serialize(float value) {
    _FloatBuffer[0] = value;
    Buffer.BlockCopy(_FloatBuffer, 0, _Buffer, _Index, 4);
    
    Swap4();
    
    _Index += 4;
  }

  public void Serialize(double value) {
    _DoubleBuffer[0] = value;
    Buffer.BlockCopy(_DoubleBuffer, 0, _Buffer, _Index, 8);
    
    Swap8();
    
    _Index += 8;
  }

  public void Serialize(byte[] value) {
    Array.Copy(value, 0, _Buffer, _Index, value.Length);
    _Index += value.Length;
  }

  public void Serialize(char[] value) {
    for (int i = 0; i < value.Length; ++i) {
      _Buffer[_Index + i] = (byte)value[i];
    }
    _Index += value.Length;
  }

  public void Deserialize(out byte value) {
    value = _Buffer[_Index];
    _Index += 1;
  }

  public void Deserialize(out ushort value) {
    uint b0 = _Buffer[_Index];
    uint b1 = _Buffer[_Index + 1];

    if (_UseLittleEndian) {
      value = (ushort)(b0 | (b1 << 8));
    }
    else {
      value = (ushort)(b1 | (b0 << 8));
    }

    _Index += 2;
  }

  public void Deserialize(out uint value) {
    uint b0 = _Buffer[_Index];
    uint b1 = _Buffer[_Index + 1];
    uint b2 = _Buffer[_Index + 2];
    uint b3 = _Buffer[_Index + 3];
    if (_UseLittleEndian) {
      value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
    else {
      value = b3 | (b2 << 8) | (b1 << 16) | (b0 << 24);
    }

    _Index += 4;
  }

  public void Deserialize(out int value) {
    uint unsigned;
    Deserialize(out unsigned);
    value = (int)unsigned;
  }

  public void Deserialize(out ulong value) {
    uint i0;
    uint i1;
    if (_UseLittleEndian) {
      Deserialize(out i0);
      Deserialize(out i1);
    }
    else {
      Deserialize(out i1);
      Deserialize(out i0);
    }
    value = i0 | (i1 << 32);

    _Index += 8;
  }

  public void Deserialize(out float value) {
    Swap4(); 
    
    Buffer.BlockCopy(_Buffer, _Index, _FloatBuffer, 0, 4);
    value = _FloatBuffer[0];
    
    Swap4();
    
    _Index += 4;
  }

  
  public void Deserialize(out double value) {
    Swap8();
    
    Buffer.BlockCopy(_Buffer, _Index, _DoubleBuffer, 0, 8);
    value = _DoubleBuffer[0];
    
    Swap8();
    
    _Index += 8;
  }

  public void Deserialize(byte[] value) {
    Array.Copy(_Buffer, _Index, value, 0, value.Length);
    _Index += value.Length;
  }

  public void Deserialize(char[] value) {
    for (int i = 0; i < value.Length; ++i) {
      value[i] = (char)_Buffer[_Index + i];
    }
    _Index += value.Length;
  }

  private void Swap(int offsetA, int offsetB) {
    int indexA = _Index + offsetA;
    int indexB = _Index + offsetB;

    byte swap = _Buffer[indexA];
    _Buffer[indexA] = _Buffer[indexB];
    _Buffer[indexB] = swap;
  }

  private void Swap4() {
    if (_NeedSwap) {
      Swap(0, 3);
      Swap(1, 2);
    }
  }

  private void Swap8() {
    if (_NeedSwap) {
      Swap(0, 7);
      Swap(1, 6);
      Swap(2, 5);
      Swap(3, 4);
    }
  }

  public static int GetSerializationLength(byte value) {
    return 1;
  }

  public static int GetSerializationLength(ushort value) {
    return 2;
  }

  public static int GetSerializationLength(uint value) {
    return 4;
  }

  public static int GetSerializationLength(int value) {
    return 4;
  }

  public static int GetSerializationLength(ulong value) {
    return 8;
  }

  public static int GetSerializationLength(float value) {
    return 4;
  }

  public static int GetSerializationLength(double value) {
    return 8;
  }

  public static int GetSerializationLength(byte[] value) {
    return value.Length;
  }

  public static int GetSerializationLength(char[] value) {
    return value.Length;
  }
}

