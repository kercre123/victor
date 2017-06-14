using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  public class ItemIdAttribute : PropertyAttribute {
    public ItemIdAttribute() {
    }
  }

  [System.Serializable]
  public class ItemData {

    [SerializeField]
    private string _ItemId;

    public string ID {
      get { return _ItemId; }
    }

    [SerializeField]
    private string _LocKey;

    public string LocKey {
      get { return _LocKey; }
    }

    [SerializeField]
    private Sprite _ItemIcon;

    public Sprite Icon {
      get { return _ItemIcon; }
    }

    [SerializeField]
    private int _StartingAmount;

    [SerializeField]
    private int _DemoStartingAmount = -1;

    public int StartingAmount {
      get {
        // Only sparks has a specific demo amount, just use the normal default amount otherwise
        if (DebugMenuManager.Instance.DemoMode && _DemoStartingAmount >= 0) {
          return _DemoStartingAmount;
        }
        return _StartingAmount;
      }
    }

    public string GetAmountName(int amount) {
      return (amount == 1) ? GetSingularName() : GetPluralName();
    }

    public string GetSingularName() {
      return Localization.Get(LocKey + ".singular");
    }

    public string GetPluralName() {
      return Localization.Get(LocKey + ".plural");
    }

    public override string ToString() {
      return ID;
    }
  }

  public class ItemDataConfig : ScriptableObject {

    private static ItemDataConfig _sInstance;

    public static void SetInstance(ItemDataConfig instance) {
      _sInstance = instance;
      _sInstance.PopulateDictionary();
    }

    public static ItemDataConfig Instance {
      get {
        return _sInstance;
      }
    }

    [SerializeField]
    private ItemData[] _ItemMap;

    [SerializeField]
    private ItemData _GenericCubeItemData;

    private Dictionary<string, ItemData> _IdToData;

    [SerializeField]
    private int _MaxInventoryLimit;
    public int MaxInventoryLimit {
      get {
        return _MaxInventoryLimit;
      }
    }

    private void PopulateDictionary() {
      _IdToData = new Dictionary<string, ItemData>();
      foreach (ItemData data in _ItemMap) {
        if (!_IdToData.ContainsKey(data.ID)) {
          _IdToData.Add(data.ID, data);
        }
        else {
          DAS.Error("ItemDataConfig.PopulateDictionary", "Trying to add item to dictionary, but the item already exists! item=" + data.ID);
        }
      }
    }

    public static ItemData GetData(string itemId) {
      ItemData data = null;
      if (!_sInstance._IdToData.TryGetValue(itemId, out data)) {
        DAS.Error("ItemDataConfig.GetData", "Could not find item='" + itemId + "' in dictionary!");
      }
      return data;
    }

    public static ItemData GetCubeData() {
      return _sInstance._GenericCubeItemData;
    }

    public static List<string> GetAllItemIds() {
      List<string> allItemIds = new List<string>();
      allItemIds.AddRange(GetItemIds());
      return allItemIds;
    }

    public static IEnumerable<string> GetItemIds() {
      return _sInstance._IdToData.Keys;
    }

#if UNITY_EDITOR
    public IEnumerable<string> EditorGetItemIds() {
      List<string> itemIds = new List<string>();
      foreach (var data in _ItemMap) {
        itemIds.Add(data.ID);
      }
      return itemIds;
    }
#endif
  }
}