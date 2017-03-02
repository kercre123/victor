using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class RobotDirectChannel : ChannelBase<RobotMessageIn, RobotMessageOut> {

  private byte[] _ReceiveBytes;
  private byte[] _SendBytes;
  private System.IO.MemoryStream _SendStream;
  private System.IO.MemoryStream _ReceiveStream;
  private RobotMessageIn _InMessage = new RobotMessageIn();

  public RobotDirectChannel() {
    IsConnected = false;
    _SendBytes = new byte[(int)CommsConstants.kDirectCommsBufferSize];
    _SendStream = new System.IO.MemoryStream(_SendBytes);
    _ReceiveBytes = new byte[(int)CommsConstants.kDirectCommsBufferSize];
    _ReceiveStream = new System.IO.MemoryStream(_ReceiveBytes);
  }

  public override bool HasPendingOperations {
    get {
      return false;
    }
  }

  public override void Connect(int deviceID, int localPort, string advertisingIP, int advertisingPort) {
    Connect(deviceID, localPort);
  }

  public override void Connect(int deviceID, int localPort) {
    if (IsConnected) {
      return;
    }
    IsConnected = true;
    RaiseConnectedToClient("");
  }

  public override void Disconnect() {
    if (!IsConnected) {
      return;
    }
    IsConnected = false;
    RaiseDisconnectedFromClient(DisconnectionReason.ConnectionLost);
  }

  public override void Send(RobotMessageOut message) {
    LogMessage(message, "RobotDirectChannel.QueueMessage");
    message.Pack(_SendStream);
  }

  public override void Update() {
    if (!IsConnected) {
      return;
    }
    ReceiveMessages();
    SendMessages();
  }

  public override void FlushQueuedMessages() {
    SendMessages();
  }

  protected void SendMessages() {
    if (_SendStream.Position == 0) {
      // nothing to send
      return;
    }

    CozmoBinding.cozmo_transmit_game_to_engine(_SendBytes, (System.UIntPtr)_SendStream.Position);

    // reset stream head
    _SendStream.Position = 0;
  }

  protected void ReceiveMessages() {
    // pull any queued messages from engine
    uint writtenCount = CozmoBinding.cozmo_transmit_engine_to_game(_ReceiveBytes, (System.UIntPtr)_ReceiveBytes.Length);
    if (writtenCount == 0) {
      return;
    }

    // reset receive stream and start reading messages
    _ReceiveStream.Position = 0;
    while (_ReceiveStream.Position < writtenCount) {
      _InMessage.Unpack(_ReceiveStream);
      if (_InMessage.Message.GetTag() == MessageEngineToGame.Tag.INVALID) {
        DAS.Error("RobotDirectChannel.ReceiveMessages", "error reading incoming buffer");
        break;
      }

      // dispatch message
      LogMessage(_InMessage, "RobotDirectChannel.ReceiveMessages");
      RaiseMessageReceived(_InMessage);
    }
  }

  private void LogMessage(IMessageWrapper message, string channel) {
    switch (message.GetTag()) {
    case "Ping":
    case "LatencyMessage":
    case "DebugLatencyMessage":
    case "RobotProcessedImage":
    case "RobotObservedObject":
    case "ImageChunk":
    case "DebugString":
    case "DebugAppendConsoleLogLine":
    case "RobotState":
    case "MoodState":
    case "ObjectProjectsIntoFOV":
      // don't log these messages
      return;
    }
    DAS.Debug(channel, "Tag=" + message.GetTag());
  }

}
