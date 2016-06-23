using System;
using Anki.Cozmo.ExternalInterface;
using UnityEngine;

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

