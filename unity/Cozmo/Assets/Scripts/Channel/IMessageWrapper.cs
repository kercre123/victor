using System;

public interface IMessageWrapper {

  void Unpack(System.IO.Stream stream);

  void Unpack(System.IO.BinaryReader reader);

  void Pack(System.IO.Stream stream);

  void Pack(System.IO.BinaryWriter writer);

  int Size { get; }

  bool IsValid { get; }

  string GetTag();
}

