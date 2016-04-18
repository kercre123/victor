using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [System.Serializable]
  public class Inventory {
    public delegate void InventoryValueChangedHandler(string itemId,int delta,int newCount);

    public event InventoryValueChangedHandler ItemAdded;
    public event InventoryValueChangedHandler ItemRemoved;
    public event InventoryValueChangedHandler ItemCountSet;

    private Dictionary<string, int> _ItemIdToCount;

    public void AddItem(string itemId, int count = 1) {
    }

    public void AddItems(IEnumerable<KeyValuePair<string, int>> itemsById) {
    }

    public void RemoveItem(string itemId, int count = 1) {
    }

    public void RemoveItems(IEnumerable<KeyValuePair<string, int>> itemsById) {
    }

    public bool CanRemove(string itemId, int count = 1) {
      return false;
    }

    public bool CanRemove(IEnumerable<KeyValuePair<string, int>> itemsById) {
      return false;
    }

    public void SetItemCount(string itemId, int count) {
    }

    public int GetItemCount(string itemId) {
      return 0;
    }
  }
}