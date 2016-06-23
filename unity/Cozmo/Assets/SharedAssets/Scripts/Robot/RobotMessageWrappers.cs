using Anki.Cozmo.ExternalInterface;

public class RobotMessageOut : IMessageWrapper {
  public readonly MessageGameToEngine Message = new MessageGameToEngine();

  #region IMessageWrapper implementation

  public void Unpack(System.IO.Stream stream) {
    Message.Unpack(stream);
  }

  public void Unpack(System.IO.BinaryReader reader) {
    Message.Unpack(reader);
  }

  public void Pack(System.IO.Stream stream) {
    Message.Pack(stream);
  }

  public void Pack(System.IO.BinaryWriter writer) {
    Message.Pack(writer);
  }

  public string GetTag() {
    return Message.GetTag().ToString();
  }

  public int Size {
    get {
      return Message.Size;
    }
  }

  public bool IsValid { 
    get {
      return Message.GetTag() != MessageGameToEngine.Tag.INVALID;
    }
  }

  #endregion
}

public class RobotMessageIn : IMessageWrapper {
  public readonly MessageEngineToGame Message = new MessageEngineToGame();

  #region IMessageWrapper implementation

  public void Unpack(System.IO.Stream stream) {
    Message.Unpack(stream);
  }

  public void Unpack(System.IO.BinaryReader reader) {
    Message.Unpack(reader);
  }

  public void Pack(System.IO.Stream stream) {
    Message.Pack(stream);
  }

  public void Pack(System.IO.BinaryWriter writer) {
    Message.Pack(writer);
  }

  public string GetTag() {
    return Message.GetTag().ToString();
  }

  public int Size {
    get {
      return Message.Size;
    }
  }

  public bool IsValid { 
    get {
      return Message.GetTag() != MessageEngineToGame.Tag.INVALID;
    }
  }

  #endregion
}
