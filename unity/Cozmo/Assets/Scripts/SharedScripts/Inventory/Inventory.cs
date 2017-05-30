using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [System.Serializable]
  public class Inventory {
    // Delta is always a positive number
    public delegate void InventoryValueChangedHandler(string itemId, int delta, int newCount);

    public event InventoryValueChangedHandler ItemAdded;
    public event InventoryValueChangedHandler ItemRemoved;
    public event InventoryValueChangedHandler ItemCountSet;
    public event InventoryValueChangedHandler ItemCountUpdated;

    public int InventoryCap {
      get {
        return ItemDataConfig.Instance.MaxInventoryLimit;
      }
    }
    /// <summary>
    /// DO NOT MODIFY OUTSIDE OF CLASS.
    /// Public so that serialization for saving works.
    /// </summary>
    public Dictionary<string, int> _ItemIdToCount;

    public Inventory() {
      _ItemIdToCount = new Dictionary<string, int>();
    }

    public void InitInventory() {
      // Setup listeners for inventory still stored in engine...
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.InventoryStatus>(HandleEngineInventoryUpdated);
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.InventoryStatus>(HandleEngineInventoryUpdated);
    }

    public void HandleEngineInventoryUpdated(Anki.Cozmo.ExternalInterface.InventoryStatus message) {
      int sparksAmount = message.allInventory.inventoryItemAmount[(int)Anki.Cozmo.InventoryType.Sparks];

      // Sparks is the only currency engine is tracking
      if (!_ItemIdToCount.ContainsKey(RewardedActionManager.Instance.SparkID)) {
        _ItemIdToCount.Add(RewardedActionManager.Instance.SparkID, 0);
      }
      if (sparksAmount != _ItemIdToCount[RewardedActionManager.Instance.SparkID]) {
        int delta = sparksAmount - _ItemIdToCount[RewardedActionManager.Instance.SparkID];
        _ItemIdToCount[RewardedActionManager.Instance.SparkID] = sparksAmount;
        if (ItemCountUpdated != null) {
          ItemCountUpdated(RewardedActionManager.Instance.SparkID, delta, sparksAmount);
        }
      }
    }

    // Will send back a InventoryStatus message
    public void ForceEngineValueSync() {
      RobotEngineManager.Instance.Message.InventoryRequestGet =
          Singleton<Anki.Cozmo.ExternalInterface.InventoryRequestGet>.Instance;
      RobotEngineManager.Instance.SendMessage();
    }

    public void AddItemAmount(string itemId, int count = 1) {
      if (_ItemIdToCount.ContainsKey(itemId)) {
        // If already at inventory cap just don't add anything or trigger events
        if (_ItemIdToCount[itemId] >= InventoryCap) {
          return;
        }
        // If would hit cap or exceed it, set to cap but still fire appropriate events.
        if (CanAddItemAmount(itemId, count)) {
          _ItemIdToCount[itemId] = _ItemIdToCount[itemId] + count;
        }
        else {
          _ItemIdToCount[itemId] = InventoryCap;
        }
      }
      else {
        if (count > InventoryCap) { count = InventoryCap; }
        _ItemIdToCount.Add(itemId, count);
      }

      DAS.Event("meta.inventory.change", itemId, DASUtil.FormatExtraData(count.ToString()));
      DAS.Event("meta.inventory.balance", itemId, DASUtil.FormatExtraData(_ItemIdToCount[itemId].ToString()));
      if (ItemAdded != null) {
        ItemAdded(itemId, count, _ItemIdToCount[itemId]);
      }
      if (itemId == RewardedActionManager.Instance.SparkID) {
        RobotEngineManager.Instance.Message.InventoryRequestAdd =
                  Singleton<Anki.Cozmo.ExternalInterface.InventoryRequestAdd>.Instance.Initialize(Anki.Cozmo.InventoryType.Sparks, count);
        RobotEngineManager.Instance.SendMessage();
      }
    }

    public void AddItemAmounts(IEnumerable<KeyValuePair<string, int>> itemsById) {
      foreach (var kvp in itemsById) {
        AddItemAmount(kvp.Key, kvp.Value);
      }
    }

    /// <summary>
    /// RemoveItem expects a positive count.
    /// </summary>
    public bool RemoveItemAmount(string itemId, int count = 1) {
      bool removedItem = false;
      if (count == 0) {
        removedItem = true;
      }
      else if (count > 0) {
        if (CanRemoveItemAmount(itemId, count)) {
          removedItem = true;
          RemoveItemInternal(itemId, count);
        }
      }
      else {
        DAS.Error("Inventory.RemoveItem", "RemoveItem expects positive arguments. count=" + count);
      }
      return removedItem;
    }

    public bool RemoveItemAmounts(IEnumerable<KeyValuePair<string, int>> itemsById) {
      bool removedItems = false;
      if (CanRemoveItemAmounts(itemsById)) {
        foreach (var kvp in itemsById) {
          RemoveItemInternal(kvp.Key, kvp.Value);
        }
        removedItems = true;
      }
      return removedItems;
    }

    private void RemoveItemInternal(string itemId, int count = 1) {
      // Other methods already make sure the key exists
      int oldValue = _ItemIdToCount[itemId];
      _ItemIdToCount[itemId] = oldValue - count;

      if (ItemRemoved != null) {
        ItemRemoved(itemId, count, _ItemIdToCount[itemId]);
      }
      int delta = -count;
      DAS.Event("meta.inventory.change", itemId, DASUtil.FormatExtraData(delta.ToString()));
      DAS.Event("meta.inventory.balance", itemId, DASUtil.FormatExtraData(_ItemIdToCount[itemId].ToString()));
      DataPersistence.DataPersistenceManager.Instance.Save();

      // Update the engine value to be confirmed.
      if (itemId == RewardedActionManager.Instance.SparkID) {
        RobotEngineManager.Instance.Message.InventoryRequestAdd =
                  Singleton<Anki.Cozmo.ExternalInterface.InventoryRequestAdd>.Instance.Initialize(Anki.Cozmo.InventoryType.Sparks, -count);
        RobotEngineManager.Instance.SendMessage();
      }
    }

    public bool CanAddItemAmount(string itemId, int count = 1) {
      bool canAdd = false;
      if (count == 0) {
        canAdd = true;
      }
      else if (count > 0) {
        int currentCount = GetItemAmount(itemId);
        canAdd = (currentCount + count <= InventoryCap);
      }
      else {
        DAS.Error("Inventory.CanAdd", "CanAdd expects positive arguments. count=" + count);
      }
      return canAdd;
    }

    public bool CanRemoveItemAmount(string itemId, int count = 1) {
      bool canRemove = false;
      if (count == 0) {
        canRemove = true;
      }
      else if (count > 0) {
        int currentCount = GetItemAmount(itemId);
        canRemove = (currentCount >= count);
      }
      else {
        DAS.Error("Inventory.CanRemove", "CanRemove expects positive arguments. count=" + count);
      }
      return canRemove;
    }

    public bool CanRemoveItemAmounts(IEnumerable<KeyValuePair<string, int>> itemsById) {
      bool canRemove = true;
      foreach (var kvp in itemsById) {
        if (!CanRemoveItemAmount(kvp.Key, kvp.Value)) {
          canRemove = false;
          break;
        }
      }
      return canRemove;
    }

    public void SetItemAmount(string itemId, int count) {
      int oldValue = 0;
      if (_ItemIdToCount.ContainsKey(itemId)) {
        oldValue = _ItemIdToCount[itemId];
        _ItemIdToCount[itemId] = count;
      }
      else {
        _ItemIdToCount.Add(itemId, count);
      }

      if (ItemCountSet != null) {
        ItemCountSet(itemId, count - oldValue, count);
      }
      DataPersistence.DataPersistenceManager.Instance.Save();

      // This will cause an eventual InventoryStatus message that could set back to a reliable value
      // But keep the most up to date value...
      if (itemId == RewardedActionManager.Instance.SparkID) {
        RobotEngineManager.Instance.Message.InventoryRequestSet =
                  Singleton<Anki.Cozmo.ExternalInterface.InventoryRequestSet>.Instance.Initialize(Anki.Cozmo.InventoryType.Sparks, count);
        RobotEngineManager.Instance.SendMessage();
      }
    }

    public int GetItemAmount(string itemId) {
      int count = 0;
      _ItemIdToCount.TryGetValue(itemId, out count);
      return count;
    }
  }
}