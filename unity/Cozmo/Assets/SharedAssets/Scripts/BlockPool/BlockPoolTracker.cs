using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using U2G = Anki.Cozmo.ExternalInterface;

namespace Cozmo.BlockPool {

  public class BlockPoolTracker {
    public delegate void BlockStateHandler(BlockPoolData data);
    public event BlockStateHandler OnBlockDataUnavailable;
    public event BlockStateHandler OnBlockDataConnectionChanged;

    public delegate void BlockStateUpdateHandler(BlockPoolData data, bool wasCreated);
    public event BlockStateUpdateHandler OnBlockDataUpdated;

    public delegate void AutoBlockPoolEnabledChangedHandler(bool enabled);
    public event AutoBlockPoolEnabledChangedHandler OnAutoBlockPoolEnabledChanged;

    public delegate void NumAvailableBlocksChangedHandler(ObjectType type);
    public event NumAvailableBlocksChangedHandler OnNumberAvailableObjectsChanged;

    private U2G.BlockPoolEnabledMessage _BlockPoolEnabledMessage;
    private U2G.BlockSelectedMessage _BlockSelectedMessage;

    private const float kUpdateDelay_Secs = 1;
    private float _LastUpdateTime;

    private Dictionary<uint, BlockPoolData> _BlocksByID;
    private Dictionary<ObjectType, List<BlockPoolData>> _BlocksByType;

    public List<BlockPoolData> Blocks { get; private set; }

    private bool _autoBlockPoolEnabled;
    public bool AutoBlockPoolEnabled {
      get {
        return _autoBlockPoolEnabled;
      }

      private set {
        _autoBlockPoolEnabled = value;

        if (OnAutoBlockPoolEnabledChanged != null) {
          OnAutoBlockPoolEnabledChanged(value);
        }
      }
    }


    private RobotEngineManager _RobotEngineManager;

    public BlockPoolTracker(RobotEngineManager rem) {
      _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
      _BlockSelectedMessage = new U2G.BlockSelectedMessage();
      _BlocksByID = new Dictionary<uint, BlockPoolData>();
      Blocks = new List<BlockPoolData>();
      _BlocksByType = new Dictionary<ObjectType, List<BlockPoolData>>();

      _RobotEngineManager = rem;

      _RobotEngineManager.AddCallback<U2G.ObjectAvailable>(HandleObjectAvailableMsg);
      _RobotEngineManager.AddCallback<U2G.ObjectUnavailable>(HandleObjectUnavailableMsg);
      _RobotEngineManager.AddCallback<ObjectConnectionState>(HandleObjectConnectionState);
    }

    public void CleanUp() {
      _RobotEngineManager.RemoveCallback<U2G.BlockPoolDataMessage>(HandleBlockPoolData);
      _RobotEngineManager.RemoveCallback<ObjectConnectionState>(HandleObjectConnectionState);
      _RobotEngineManager.RemoveCallback<U2G.ObjectAvailable>(HandleObjectAvailableMsg);
      _RobotEngineManager.RemoveCallback<U2G.ObjectUnavailable>(HandleObjectUnavailableMsg);
      SendAvailableObjects(false, (byte)_RobotEngineManager.CurrentRobotID);
    }

    public void InitBlockPool() {
      _RobotEngineManager.AddCallback<U2G.BlockPoolDataMessage>(HandleBlockPoolData);

      // Gets back the InitBlockPool message to fill.
      _RobotEngineManager.Message.GetBlockPoolMessage = new G2U.GetBlockPoolMessage();
      _RobotEngineManager.SendMessage();
    }

    public void EnableBlockPool(bool val, float discoveryTimeSeconds = 0f) {
      AutoBlockPoolEnabled = val;
      _BlockPoolEnabledMessage.enabled = val;
      _BlockPoolEnabledMessage.discoveryTimeSecs = discoveryTimeSeconds;
      _RobotEngineManager.Message.BlockPoolEnabledMessage = _BlockPoolEnabledMessage;
      _RobotEngineManager.SendMessage();
    }

    public void ResetBlockPool() {
      foreach (KeyValuePair<uint, BlockPoolData> kvp in _BlocksByID) {
        if (kvp.Value.ConnectionState == BlockConnectionState.Connected) {
          UpdateBlockData(kvp.Value, null, null, BlockConnectionState.DisconnectInProgress, false);
        }
      }

      _RobotEngineManager.Message.BlockPoolResetMessage = new U2G.BlockPoolResetMessage();
      _RobotEngineManager.SendMessage();
    }

    public BlockPoolData GetObject(uint factoryID) {
      BlockPoolData data = null;
      _BlocksByID.TryGetValue(factoryID, out data);
      return data;
    }

    public BlockPoolData SetObjectInPool(uint factoryID, bool toConnect) {
      BlockPoolData data = null;
      if (_BlocksByID.TryGetValue(factoryID, out data)) {
        data = SetObjectInPoolInternal(data, toConnect);
      }
      return data;
    }

    private BlockPoolData SetObjectInPoolInternal(BlockPoolData data, bool toConnect) {
      if (toConnect) {
        UpdateBlockData(data, null, null, BlockConnectionState.ConnectInProgress, null);
        SendBlockSelectedMessage(data.FactoryID, data.ObjectType, true);
      }
      else {
        UpdateBlockData(data, null, null, BlockConnectionState.DisconnectInProgress, null);
        SendBlockSelectedMessage(data.FactoryID, data.ObjectType, false);
      }
      return data;
    }

    private void SendBlockSelectedMessage(uint factoryID, ObjectType type, bool isConnected) {
      _BlockSelectedMessage.factoryId = factoryID;
      _BlockSelectedMessage.objectType = type;
      _BlockSelectedMessage.selected = isConnected;
      _RobotEngineManager.Message.BlockSelectedMessage = _BlockSelectedMessage;
      _RobotEngineManager.SendMessage();
    }

    private void HandleBlockPoolData(U2G.BlockPoolDataMessage initMsg) {
      // Reset the isPersistent flag for all the objects and then set it to true to those on the
      // updated block pool
      for (int i = 0; i < Blocks.Count; ++i) {
        Blocks[i].IsPersistent = false;
      }

      for (int i = 0; i < initMsg.blockData.Length; ++i) {
        Anki.Cozmo.ExternalInterface.BlockPoolBlockData blockData = initMsg.blockData[i];
        AddOrUpdateBlockData(blockData.factoryID, blockData.objectType, ObservedObject.kInvalidObjectID, null, null, true);
      }
    }

    private void HandleObjectAvailableMsg(U2G.ObjectAvailable objAvailableMsg) {
      switch (objAvailableMsg.objectType) {
      case ObjectType.Block_LIGHTCUBE1:
      case ObjectType.Block_LIGHTCUBE2:
      case ObjectType.Block_LIGHTCUBE3:
      case ObjectType.Charger_Basic:
        AddOrUpdateBlockData(objAvailableMsg.factory_id, objAvailableMsg.objectType, null, objAvailableMsg.rssi, BlockConnectionState.Available, null);
        break;

      default:
        break;
      }
    }

    private void HandleObjectUnavailableMsg(U2G.ObjectUnavailable objUnAvailableMsg) {
      BlockPoolData data;
      if (_BlocksByID.TryGetValue(objUnAvailableMsg.factory_id, out data)) {
        UpdateBlockData(data, null, null, BlockConnectionState.Unavailable, null);

        if (OnBlockDataUnavailable != null) {
          OnBlockDataUnavailable(data);
        }
      }
    }

    private void HandleObjectConnectionState(ObjectConnectionState message) {
      ObjectType objectType = ObjectTypeFromActiveObjectType(message.device_type);

      AddOrUpdateBlockData(message.factoryID,
                      objectType, 
                      (int)message.objectID, 
                      null,
                      message.connected ? BlockConnectionState.Connected : BlockConnectionState.Unavailable,
                      null);
    }

    private ObjectType ObjectTypeFromActiveObjectType(ActiveObjectType activeObjectType) {
      // Get the ObjectType from ActiveObjectType
      ObjectType objectType = ObjectType.Invalid;
      switch (activeObjectType) {
      case ActiveObjectType.OBJECT_CUBE1:
        objectType = ObjectType.Block_LIGHTCUBE1;
        break;
      case ActiveObjectType.OBJECT_CUBE2:
        objectType = ObjectType.Block_LIGHTCUBE2;
        break;
      case ActiveObjectType.OBJECT_CUBE3:
        objectType = ObjectType.Block_LIGHTCUBE3;
        break;
      case ActiveObjectType.OBJECT_CHARGER:
        objectType = ObjectType.Charger_Basic;
        break;
      case ActiveObjectType.OBJECT_UNKNOWN:
        objectType = ObjectType.Unknown;
        break;
      }
      return objectType;
    }

    public void SendAvailableObjects(bool enable, byte robotId) {
      // Will get a series of "Object Available" messages that represent blocks cozmo has "Heard" to connect to
      G2U.SendAvailableObjects msg = new G2U.SendAvailableObjects();
      msg.enable = enable;
      msg.robotID = robotId;
      _RobotEngineManager.Message.SendAvailableObjects = msg;
      _RobotEngineManager.SendMessage();
    }

    private void AddOrUpdateBlockData(uint factoryID, 
                                      ObjectType type, 
                                      int? objectID,
                                      sbyte? signalStrength,
                                      BlockConnectionState? connectionState,
                                      bool? isPersistent) {

      BlockPoolData data = null;
      if (!_BlocksByID.TryGetValue(factoryID, out data)) {
        int oID = objectID.HasValue ? objectID.Value : ObservedObject.kInvalidObjectID;
        BlockConnectionState cState = connectionState.HasValue ? connectionState.Value : BlockConnectionState.Unavailable;
        data = new BlockPoolData(type, factoryID, signalStrength.GetValueOrDefault(), oID, cState, isPersistent.GetValueOrDefault());
        _BlocksByID.Add(factoryID, data);
        AddToBlocksByType(data);
        Blocks.Add(data);

        if (OnBlockDataUpdated != null) {
          OnBlockDataUpdated(data, true);
        }

        if (OnBlockDataConnectionChanged != null) {
          OnBlockDataConnectionChanged(data);
        }

//        LogBlockData();
      }
      else {
        UpdateBlockData(data, signalStrength, objectID, connectionState, isPersistent);
      }
    }

    private BlockPoolData UpdateBlockData(BlockPoolData data, 
                                          sbyte? signalStrength,
                                          int? objectId, 
                                          BlockConnectionState? connectionState,
                                          bool? isPersistent) {
      bool dataUpdated = false;
      if (signalStrength.HasValue && data.RSSI != signalStrength.Value) {
        data.RSSI = signalStrength.Value;
        dataUpdated = true;
      }

      if (objectId.HasValue && data.ObjectID != objectId.Value) {
        data.ObjectID = objectId.Value;
        dataUpdated = true;
      }

      if (connectionState.HasValue && data.ConnectionState != connectionState.Value) {
        BlockConnectionState oldState = data.ConnectionState;
        data.ConnectionState = connectionState.Value;
        dataUpdated = true;

        if (oldState == BlockConnectionState.Available || connectionState.Value == BlockConnectionState.Available) {
          if (OnNumberAvailableObjectsChanged != null) {
            OnNumberAvailableObjectsChanged(data.ObjectType);
          }
        }

        if (OnBlockDataConnectionChanged != null) {
          OnBlockDataConnectionChanged(data);
        }
      }

      if (isPersistent.HasValue && data.IsPersistent != isPersistent.Value) {
        data.IsPersistent = isPersistent.Value;
        dataUpdated = true;
      }

      if (dataUpdated) {
        if (OnBlockDataUpdated != null) {
          OnBlockDataUpdated(data, false);
        }

//        LogBlockData();
      }

      return data;
    }

    private void AddToBlocksByType(BlockPoolData data) {
      List<BlockPoolData> blocksOfObjectType;
      if (_BlocksByType.TryGetValue(data.ObjectType, out blocksOfObjectType)) {
        BlockPoolData matchingData = GetDataWithFactoryId(data.FactoryID, blocksOfObjectType);
        if (matchingData == null) {
          blocksOfObjectType.Add(data);

          // Send number changed event
          if (data.IsAvailable) {
            if (OnNumberAvailableObjectsChanged != null) {
              OnNumberAvailableObjectsChanged(data.ObjectType);
            }
          }
        }
      }
      else {
        blocksOfObjectType = new List<BlockPoolData>();
        blocksOfObjectType.Add(data);
        _BlocksByType.Add(data.ObjectType, blocksOfObjectType);

        // Send number changed event
        if (data.IsAvailable) {
          if (OnNumberAvailableObjectsChanged != null) {
            OnNumberAvailableObjectsChanged(data.ObjectType);
          }
        }
      }
    }

    private BlockPoolData GetDataWithFactoryId(uint factoryId, List<BlockPoolData> dataList) {
      BlockPoolData matchingData = null;
      foreach (var data in dataList) {
        if (data.FactoryID == factoryId) {
          matchingData = data;
          break;
        }
      }
      return matchingData;
    }

    public void SortBlockStatesByRSSI() {
      // Sort the list by RSSI
      Blocks.Sort((BlockPoolData x, BlockPoolData y) => {
        // In order to have a more stable sort that doesn't continuously switch around 
        // two elements with the same RSSI, if the RSSI are equal then we sort based
        // on the ID
        int comparison = x.RSSI.CompareTo(y.RSSI);
        return (comparison != 0) ? comparison : x.FactoryID.CompareTo(y.FactoryID);
      });
    }

    public List<uint> SortBlockStatesByRSSI(List<uint> factoryIds) {
      // Sort the list by RSSI
      factoryIds.Sort((uint a, uint b) => {
        // In order to have a more stable sort that doesn't continuously switch around 
        // two elements with the same RSSI, if the RSSI are equal then we sort based
        // on the ID
        BlockPoolData x, y;
        _BlocksByID.TryGetValue(a, out x);
        _BlocksByID.TryGetValue(b, out y);

        if (x == null) {
          return -1;
        }

        if (y == null) {
          return 1;
        }

        int comparison = x.RSSI.CompareTo(y.RSSI);
        return (comparison != 0) ? comparison : x.FactoryID.CompareTo(y.FactoryID);
      });

      return factoryIds;
    }

    public void ForEachObjectOfType(ObjectType type, System.Action<BlockPoolData> action) {
      List<BlockPoolData> blocks;
      if (_BlocksByType.TryGetValue(type, out blocks)) {
        foreach (BlockPoolData data in blocks) {
          action(data);
        }
      }
    }

    private void LogBlockData() {
      System.Text.StringBuilder str = new System.Text.StringBuilder("BlockPoolTracker:\n");
      for (int i = 0; i < Blocks.Count; ++i) {
        str.Append(Blocks[i] + "\n");
      }

      UnityEngine.Debug.Log(str.ToString());
    }
  }
}