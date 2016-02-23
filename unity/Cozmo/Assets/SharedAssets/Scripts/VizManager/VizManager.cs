using System;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.VizInterface;

namespace Anki.Cozmo {
  public class VizManager : MonoBehaviour {

    private class MessageVizWrapper : IMessageWrapper {
      public readonly MessageViz Message = new MessageViz();

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
          return Message.GetTag() != MessageViz.Tag.INVALID;
        }
      }

      #endregion
    }

    private ChannelBase<MessageVizWrapper, MessageVizWrapper> _Channel = null;

    private DisconnectionReason _LastDisconnectionReason;

    public event Action<string> ConnectedToClient;
    public event Action<DisconnectionReason> DisconnectedFromClient;

    private const int _UIDeviceID = 1;
    private const int _UIAdvertisingRegistrationPort = 6103;
    // 0 means random unused port
    // Used to be 5106
    private const int _UILocalPort = 0;

  
    public static VizManager Instance { get; private set; }
      
    private void OnEnable() {
      DAS.Info("VizManager", "Enabling Robot Engine Manager");
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
        DontDestroyOnLoad(gameObject);
      }
        
      _Channel = new UdpChannel<MessageVizWrapper, MessageVizWrapper>();
      _Channel.ConnectedToClient += Connected;
      _Channel.DisconnectedFromClient += Disconnected;
      _Channel.MessageReceived += ReceivedMessage;
    }
  
    public void Connect(string engineIP) {
      _Channel.Connect(_UIDeviceID, _UILocalPort, engineIP, _UIAdvertisingRegistrationPort);
    }

    public void Disconnect() {
      if (_Channel != null) {
        _Channel.Disconnect();

        // only really needed in editor in case unhitting play
        #if UNITY_EDITOR
        float limit = Time.realtimeSinceStartup + 2.0f;
        while (_Channel.HasPendingOperations) {
          if (limit < Time.realtimeSinceStartup) {
            DAS.Warn("VizManager", "Not waiting for disconnect to finish sending.");
            break;
          }
          System.Threading.Thread.Sleep(500);
        }
        #endif
      }
    }

    public DisconnectionReason GetLastDisconnectionReason() {
      DisconnectionReason reason = _LastDisconnectionReason;
      _LastDisconnectionReason = DisconnectionReason.None;
      return reason;
    }

    private void Connected(string connectionIdentifier) {
      if (ConnectedToClient != null) {
        ConnectedToClient(connectionIdentifier);
      }
    }

    private void Disconnected(DisconnectionReason reason) {
      DAS.Debug("VizManager", "Disconnected: " + reason.ToString());

      _LastDisconnectionReason = reason;
      if (DisconnectedFromClient != null) {
        DisconnectedFromClient(reason);
      }
    }

    private void ReceivedMessage(MessageVizWrapper messageWrapper) {
      var message = messageWrapper.Message;
      switch (message.GetTag()) {
      case MessageViz.Tag.Quad:

        break;

      }
    }
  }
}

