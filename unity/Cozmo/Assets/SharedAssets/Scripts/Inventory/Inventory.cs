using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [System.Serializable]
  public class Inventory {
    // Delta is always a positive number
    public delegate void InventoryValueChangedHandler(string itemId,int delta,int newCount);

    public event InventoryValueChangedHandler ItemAdded;
    public event InventoryValueChangedHandler ItemRemoved;
    public event InventoryValueChangedHandler ItemCountSet;

    private Dictionary<string, int> _ItemIdToCount;

    public Inventory() {
      _ItemIdToCount = new Dictionary<string, int>();
    }

    public void AddItemAmount(string itemId, int count = 1) {
      if (_ItemIdToCount.ContainsKey(itemId)) {
        _ItemIdToCount[itemId] = _ItemIdToCount[itemId] + count;
      }
      else {
        _ItemIdToCount.Add(itemId, count);
      }

      if (ItemAdded != null) {
        ItemAdded(itemId, count, _ItemIdToCount[itemId]);
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
      if (count > 0) {
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
      if (CanRemoveItemAmount(itemsById)) {
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
    }

    public bool CanRemoveItemAmount(string itemId, int count = 1) {
      bool canRemove = false;
      if (count > 0) {
        int currentCount = GetItemAmount(itemId);
        canRemove = (currentCount >= count);
      }
      else {
        DAS.Error("Inventory.CanRemove", "CanRemove expects positive arguments. count=" + count);
      }
      return canRemove;
    }

    public bool CanRemoveItemAmount(IEnumerable<KeyValuePair<string, int>> itemsById) {
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
    }

    public int GetItemAmount(string itemId) {
      int count = 0;
      _ItemIdToCount.TryGetValue(itemId, out count);
      return count;
    }
  }
}