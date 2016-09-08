using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;
using U2G = Anki.Cozmo.ExternalInterface;

namespace Cozmo.BlockPool {
  public class BlockPoolData {
    public BlockPoolData(ObjectType type, uint factoryID, sbyte rssi, int objectId, bool isConnected) {
      this.ObjectType = type;
      this.FactoryID = factoryID;
      this.RSSI = rssi;
      this.ObjectID = objectId;
      this.IsConnected = isConnected;
    }

    public ObjectType ObjectType { get; set; }

    public uint FactoryID { get; set; }

    public sbyte RSSI { get; set; }

    public int ObjectID { get; set; }
    public bool IsConnected { get; set; }

    public override string ToString() {
      return "BlockPoolData: factoryId=" + FactoryID.ToString("X") + " objectType=" + ObjectType + " objectID=" + ObjectID + " rssi=" + RSSI + " connected=" + IsConnected;
    }
  }

  public class BlockPoolTracker {
    public delegate void BlockStateHandler(BlockPoolData data);
    public event BlockStateHandler OnBlockDataUnavailable;
    public event BlockStateHandler OnBlockDataConnectionChanged;

    public delegate void BlockStateUpdateHandler(BlockPoolData data, bool wasCreated);
    public event BlockStateUpdateHandler OnBlockDataUpdated;

    public delegate void AutoBlockPoolEnabledChangedHandler(bool enabled);
    public event AutoBlockPoolEnabledChangedHandler OnAutoBlockPoolEnabledChanged;

    private U2G.BlockPoolEnabledMessage _BlockPoolEnabledMessage;
    private U2G.BlockSelectedMessage _BlockSelectedMessage;

    private const float kUpdateDelay_Secs = 1;
    private float _LastUpdateTime;

    private Dictionary<uint, BlockPoolData> _BlockStatesById;
    public List<BlockPoolData> BlockStates { get; private set; }

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

    // TODO: Bucket BlockData by ObjectType or provide getter functions
    // private Dictionary<ObjectType, List<BlockData>> _BlockStatesByType;

    private RobotEngineManager _RobotEngineManager;

    public BlockPoolTracker(RobotEngineManager rem) {
      _BlockPoolEnabledMessage = new U2G.BlockPoolEnabledMessage();
      _BlockSelectedMessage = new U2G.BlockSelectedMessage();
      _BlockStatesById = new Dictionary<uint, BlockPoolData>();
      BlockStates = new List<BlockPoolData>();

      _RobotEngineManager = rem;
    }

    public void CleanUp() {
      _RobotEngineManager.RemoveCallback<U2G.InitBlockPoolMessage>(HandleInitBlockPool);
      _RobotEngineManager.RemoveCallback<ObjectConnectionState>(HandleObjectConnectionState);
      _RobotEngineManager.RemoveCallback<U2G.ObjectAvailable>(HandleObjectAvailableMsg);
      _RobotEngineManager.RemoveCallback<U2G.ObjectUnavailable>(HandleObjectUnavailableMsg);
      SendAvailableObjects(false, (byte)_RobotEngineManager.CurrentRobotID);
    }

    public void InitBlockPool() {
      _RobotEngineManager.AddCallback<U2G.InitBlockPoolMessage>(HandleInitBlockPool);

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
      foreach (KeyValuePair<uint, BlockPoolData> kvp in _BlockStatesById) {
        if (kvp.Value.IsConnected) {
          kvp.Value.IsConnected = false;
        }
      }

      _RobotEngineManager.Message.BlockPoolResetMessage = new U2G.BlockPoolResetMessage();
      _RobotEngineManager.SendMessage();
    }

    public BlockPoolData ToggleObjectInPool(uint factoryID) {
      BlockPoolData data = null;
      if (_BlockStatesById.TryGetValue(factoryID, out data)) {
        data.IsConnected = !data.IsConnected;

        // TODO: What happens if you enable a block with an id that already exists?
        // Send the message to engine
        _BlockSelectedMessage.factoryId = factoryID;
        _BlockSelectedMessage.objectType = data.ObjectType;
        _BlockSelectedMessage.selected = data.IsConnected;
        _RobotEngineManager.Message.BlockSelectedMessage = _BlockSelectedMessage;
        _RobotEngineManager.SendMessage();
      }
      return data;
    }

    private void HandleInitBlockPool(U2G.InitBlockPoolMessage initMsg) {
      BlockPoolData blockPoolData;
      for (int i = 0; i < initMsg.blockData.Length; ++i) {
        blockPoolData = AddBlockData(initMsg.blockData[i].factory_id, initMsg.blockData[i].objectType,
                             sbyte.MaxValue, -1, false);
        if (blockPoolData == null) {
          blockPoolData = UpdateBlockData(initMsg.blockData[i].factory_id, initMsg.blockData[i].objectType,
                               null, null, null);
        }
      }

      // The first one gets previous ones serialized that may or may exist, this message gets the one we see.
      _RobotEngineManager.AddCallback<U2G.ObjectAvailable>(HandleObjectAvailableMsg);
      _RobotEngineManager.AddCallback<U2G.ObjectUnavailable>(HandleObjectUnavailableMsg);
      _RobotEngineManager.AddCallback<ObjectConnectionState>(HandleObjectConnectionState);
    }

    private void HandleObjectAvailableMsg(U2G.ObjectAvailable objAvailableMsg) {
      switch (objAvailableMsg.objectType) {
      case ObjectType.Block_LIGHTCUBE1:
      case ObjectType.Block_LIGHTCUBE2:
      case ObjectType.Block_LIGHTCUBE3:
      case ObjectType.Charger_Basic:
        BlockPoolData blockPoolData = UpdateBlockData(objAvailableMsg.factory_id, objAvailableMsg.objectType,
                                                      objAvailableMsg.rssi, null, null);
        if (blockPoolData == null) {
          blockPoolData = AddBlockData(objAvailableMsg.factory_id, objAvailableMsg.objectType,
                                       objAvailableMsg.rssi, -1, false);
        }
        break;

      default:
        break;
      }
    }

    private void HandleObjectUnavailableMsg(U2G.ObjectUnavailable objUnAvailableMsg) {
      BlockPoolData data;
      if (_BlockStatesById.TryGetValue(objUnAvailableMsg.factory_id, out data)) {
        // TODO: Remove from object type dictionaries
        BlockStates.Remove(data);
        _BlockStatesById.Remove(objUnAvailableMsg.factory_id);

        if (data != null && OnBlockDataUnavailable != null) {
          OnBlockDataUnavailable(data);
        }
      }
    }

    private void HandleObjectConnectionState(ObjectConnectionState message) {
      ObjectType objectType = ObjectTypeFromActiveObjectType(message.device_type);
      BlockPoolData blockPoolData = UpdateBlockData(message.factoryID, objectType,
                                                    null, (int)message.objectID, message.connected);
      if (blockPoolData == null) {
        blockPoolData = AddBlockData(message.factoryID, objectType,
                                     sbyte.MaxValue, (int)message.objectID, message.connected);
      }

      if (OnBlockDataConnectionChanged != null) {
        OnBlockDataConnectionChanged(blockPoolData);
      }
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

    private BlockPoolData AddBlockData(uint factoryID, ObjectType type, sbyte signal_strength,
                                               int objectID, bool isConnected) {
      BlockPoolData data = null;
      if (!_BlockStatesById.TryGetValue(factoryID, out data)) {
        data = new BlockPoolData(type, factoryID, signal_strength, objectID, isConnected);
        _BlockStatesById.Add(factoryID, data);
        BlockStates.Add(data);

        // TODO Add to dictionary by object type

        if (OnBlockDataUpdated != null) {
          OnBlockDataUpdated(data, true);
        }
      }
      return data;
    }

    private BlockPoolData UpdateBlockData(uint factoryId, ObjectType? type, sbyte? signalStrength,
                                          int? objectId, bool? isConnected) {
      BlockPoolData data = null;
      bool dataUpdated = false;
      if (_BlockStatesById.TryGetValue(factoryId, out data)) {
        if (signalStrength.HasValue && data.RSSI != signalStrength.Value) {
          data.RSSI = signalStrength.Value;
          dataUpdated = true;
        }

        if (objectId.HasValue && data.ObjectID != objectId.Value) {
          data.ObjectID = objectId.Value;
          dataUpdated = true;
        }

        if (isConnected.HasValue && data.IsConnected != isConnected.Value) {
          data.IsConnected = isConnected.Value;
          dataUpdated = true;
        }

        if (type.HasValue && data.ObjectType != type.Value) {
          data.ObjectType = type.Value;
          dataUpdated = true;

          // TODO Add to dictionary by object type
        }

        if (dataUpdated && OnBlockDataUpdated != null) {
          OnBlockDataUpdated(data, false);
        }
      }
      return data;
    }

    public void SortBlockStatesByRSSI() {
      // Sort the list by RSSI
      BlockStates.Sort((BlockPoolData x, BlockPoolData y) => {
        // In order to have a more stable sort that doesn't continuously switch around 
        // two elements with the same RSSI, if the RSSI are equal then we sort based
        // on the ID
        int comparison = x.RSSI.CompareTo(y.RSSI);
        return (comparison != 0) ? comparison : x.FactoryID.CompareTo(y.FactoryID);
      });
    }
  }
}