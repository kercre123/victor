using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public partial class AdvertisementRegistrationMsg
{
  public ushort port;
  public byte[] ip = new byte[18];
  public byte id;
  public byte protocol;
  public byte enableAdvertisement;
  public byte oneShot;

  public AdvertisementRegistrationMsg()
  {
  }

  /**** Pack ****/
  public System.IO.Stream Pack(System.IO.Stream stream)
  {
    System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
    writer.Write(port);
    writer.Write(ip);
    writer.Write(id);
    writer.Write(protocol);
    writer.Write(enableAdvertisement);
    writer.Write(oneShot);
    return stream;
  }

  /**** Unpack ****/
  public System.IO.Stream Unpack(System.IO.Stream stream)
  {
    System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
    port = reader.ReadUInt16();
    for(int i = 0; i < ip.Length; ++i)
    {
      ip[i] = reader.ReadByte();
    }
    id = reader.ReadByte();
    protocol = reader.ReadByte();
    enableAdvertisement = reader.ReadByte();
    oneShot = reader.ReadByte();
    return stream;
  }

  public int Size
  {
    get {
      int result = 0;
      result += 2;
      result += 1 * 18;
      result += 1;
      result += 1;
      result += 1;
      result += 1;
      return result;
    }
  }
}

