using UnityEngine;
using System.Collections;
using Anki.Cozmo;

namespace Cozmo.BlockPool {
    
  public enum BlockConnectionState {
    Connected,
    ConnectInProgress,
    DisconnectInProgress,
    Available,
    Unavailable
  }

  public class BlockPoolData {

    public BlockPoolData(ObjectType type, uint factoryID, sbyte rssi, int objectId, BlockConnectionState connectionState, bool isPersistent) {
      this.ObjectType = type;
      this.FactoryID = factoryID;
      this.RSSI = rssi;
      this.ObjectID = objectId;
      this.ConnectionState = connectionState;
      this.IsPersistent = isPersistent;
    }

    public ObjectType ObjectType { get; set; }

    public uint FactoryID { get; set; }

    public sbyte RSSI { get; set; }

    public int ObjectID { get; set; }

    public BlockConnectionState ConnectionState { get; set; }

    public bool IsPersistent { get; set; }

    public bool IsAvailable {
      get {
        return ConnectionState == BlockConnectionState.Available;
      }
    }

    public bool IsConnected {
      get {
        return ConnectionState == BlockConnectionState.Connected;
      }
    }

    public bool IsConnecting {
      get {
        return ConnectionState == BlockConnectionState.ConnectInProgress;
      }
    }

    public bool IsDisconnecting {
      get {
        return ConnectionState == BlockConnectionState.DisconnectInProgress;
      }
    }

    public override string ToString() {
      return "BlockPoolData: factoryId=" + FactoryID.ToString("X") + " objectType=" + ObjectType
        + " connectedState=" + ConnectionState + " objectID=" + ObjectID + " rssi=" + RSSI + " persistent=" + IsPersistent;
    }
  }
}

