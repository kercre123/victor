using Anki.Cozmo.VizInterface;
using System.Collections.Generic;
using System.Runtime.InteropServices;

// this class is copy-pasta'ed from RobotDirectChannel with send functionality removed
// and some minor hacks to fit in with how VizManager currently utilizes VizUdpChannel,
// so as to mimic the same behavior it's currently getting
public class VizDirectChannel : ChannelBase<MessageVizWrapper, MessageVizWrapper> {

  private byte[] _ReceiveBytes;
  private System.IO.MemoryStream _ReceiveStream;
  private MessageVizWrapper _InMessage = new MessageVizWrapper();

  public VizDirectChannel() {
    IsConnected = false;
    _ReceiveBytes = new byte[(int)Anki.Cozmo.ExternalInterface.CommsConstants.kVizCommsBufferSize];
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

  public override void Send(MessageVizWrapper message) {
  }

  public override void Update() {
    if (!IsConnected) {
      return;
    }
    ReceiveMessages();
  }

  public override void FlushQueuedMessages() {
  }

  public void DumpReceiveBuffer() {
    // read and discard
    ReceiveMessages(true);
  }

  protected void ReceiveMessages(bool ignore = false) {
    // pull any queued messages from engine
    uint writtenCount = CozmoBinding.cozmo_transmit_viz_to_game(_ReceiveBytes, (System.UIntPtr)_ReceiveBytes.Length);
    if (writtenCount == 0) {
      return;
    }

    // reset receive stream and start reading messages
    _ReceiveStream.Position = 0;
    while (_ReceiveStream.Position < writtenCount) {
      _InMessage.Unpack(_ReceiveStream);

      if (ignore) {
        continue;
      }

      if (_InMessage.Message.GetTag() == Anki.Cozmo.VizInterface.MessageViz.Tag.INVALID) {
        DAS.Error("VizDirectChannel.ReceiveMessages", "error reading incoming buffer");
        break;
      }

      // dispatch message
      RaiseMessageReceived(_InMessage);
    }
  }
}
