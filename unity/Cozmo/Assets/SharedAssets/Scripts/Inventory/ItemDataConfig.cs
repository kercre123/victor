using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
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

    public string GetName() {
      return Localization.Get(LocKey);
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

    private Dictionary<string, ItemData> _IdToData;

    private void PopulateDictionary() {
      _IdToData = new Dictionary<string, ItemData>();
      foreach (ItemData data in _ItemMap) {
        if (!_IdToData.ContainsKey(data.ID)) {
          _IdToData.Add(data.ID, data);
        }
        else {
          DAS.Error("ItemDataMap.PopulateDictionary", "Trying to add item to dictionary, but the item already exists! item=" + data.ID);
        }
      }
    }

    public ItemData GetData(string itemId) {
      ItemData data = null;
      if (!_IdToData.TryGetValue(itemId, out data)) {
        DAS.Error("ItemDataMap.GetData", "Could not find item='" + itemId + "' in dictionary!");
      }
      return data;
    }
  }
}