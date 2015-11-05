using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

public partial class AdvertisementRegistrationMsg {
  public ushort port_;
  public byte[] ip_ = new byte[18];
  public byte id_;
  public byte protocol_;
  public byte enableAdvertisement_;
  public byte oneShot_;

  public AdvertisementRegistrationMsg() {
  }

  /**** Pack ****/
  public System.IO.Stream Pack(System.IO.Stream stream) {
    System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
    writer.Write(port_);
    writer.Write(ip_);
    writer.Write(id_);
    writer.Write(protocol_);
    writer.Write(enableAdvertisement_);
    writer.Write(oneShot_);
    return stream;
  }

  /**** Unpack ****/
  public System.IO.Stream Unpack(System.IO.Stream stream) {
    System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
    port_ = reader.ReadUInt16();
    for (int i = 0; i < ip_.Length; ++i) {
      ip_[i] = reader.ReadByte();
    }
    id_ = reader.ReadByte();
    protocol_ = reader.ReadByte();
    enableAdvertisement_ = reader.ReadByte();
    oneShot_ = reader.ReadByte();
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

