using System;
using Anki.Cozmo.ExternalInterface;
using UnityEngine;

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

public class RobotUdpChannel : UdpChannel<RobotMessageIn, RobotMessageOut> {
  private readonly RobotMessageOut disconnectionMessage;


  public RobotUdpChannel() : base() {
    disconnectionMessage = new RobotMessageOut();
    disconnectionMessage.Message.DisconnectFromUiDevice = new DisconnectFromUiDevice( Anki.Cozmo.UiConnectionType.UI, 0);
  }


  protected override void BeforeConnect(byte deviceID) {
    disconnectionMessage.Message.DisconnectFromUiDevice.deviceID = deviceID;
  }
    
  protected override bool SendDisconnect() {
    SocketBufferState state = AllocateBufferState(mainServer, mainEndPoint);
    bool success = false;
    try {
      state.needsCloseWhenDone = true;

      try {
        state.length = disconnectionMessage.Size;
        disconnectionMessage.Pack(state.stream);
      }
      catch (Exception e) {
        DAS.Error(this, e.Message);
        return false;
      }

      if (mainSend == null) {
        SimpleSend(state);
      }
      else {
        mainSend.chainedSend = state;
      }

      success = true;
      return true;
    }
    catch (Exception e) {
      DAS.Error(this, e.Message);
      return false;
    }
    finally {
      if (!success) {
        state.Dispose();
      }
    }
  }
}

