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
  private const float PingTick = 0.1f;
  private float lastPingTime = 0;


  private readonly RobotMessageOut pingMessage;
  private readonly RobotMessageOut disconnectionMessage;


  public RobotUdpChannel() : base() {
    pingMessage = new RobotMessageOut();
    pingMessage.Message.Ping = new Anki.Cozmo.ExternalInterface.Ping();
    disconnectionMessage = new RobotMessageOut();
    disconnectionMessage.Message.DisconnectFromUiDevice = new DisconnectFromUiDevice();
  }


  protected override void BeforeConnect(byte deviceID) {
    pingMessage.Message.Ping.counter = 0;
    disconnectionMessage.Message.DisconnectFromUiDevice.deviceID = deviceID;
  }

  protected override void UpdatePing(float lastUpdateTime) {
    if (lastPingTime + PingTick < lastUpdateTime) {
      lastPingTime = lastUpdateTime;
      SendInternal(pingMessage);
      pingMessage.Message.Ping.counter++;
    }
  }

  protected override void UpdateLastPingTime(float lastUpdateTime) {
    lastPingTime = lastUpdateTime - PingTick * 2;
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

