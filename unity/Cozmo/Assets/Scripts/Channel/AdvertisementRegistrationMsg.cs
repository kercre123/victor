using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public partial class AdvertisementRegistrationMsg {
  public ushort Port;
  public byte[] Ip = new byte[18];
  public byte Id;
  public byte Protocol;
  public byte EnableAdvertisement;
  public byte OneShot;

  public AdvertisementRegistrationMsg() {
  }

  /**** Pack ****/
  public System.IO.Stream Pack(System.IO.Stream stream) {
    System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
    writer.Write(Port);
    writer.Write(Ip);
    writer.Write(Id);
    writer.Write(Protocol);
    writer.Write(EnableAdvertisement);
    writer.Write(OneShot);
    return stream;
  }

  /**** Unpack ****/
  public System.IO.Stream Unpack(System.IO.Stream stream) {
    System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
    Port = reader.ReadUInt16();
    for (int i = 0; i < Ip.Length; ++i) {
      Ip[i] = reader.ReadByte();
    }
    Id = reader.ReadByte();
    Protocol = reader.ReadByte();
    EnableAdvertisement = reader.ReadByte();
    OneShot = reader.ReadByte();
    return stream;
  }

  public int Size {
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

