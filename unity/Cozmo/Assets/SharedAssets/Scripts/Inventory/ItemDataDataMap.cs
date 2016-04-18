using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo {
  [System.Serializable]
  public class ItemData {
    public string ID;
    public string LocKey;
    public Sprite Icon;

    public string GetName() {
      return Localization.Get(LocKey);
    }

    public override string ToString() {
      return ID;
    }
  }

  public class ItemDataMap : ScriptableObject {

    private static ItemDataMap _sInstance;

    public static void SetInstance(ItemDataMap instance) {
      _sInstance = instance;
      _sInstance.PopulateDictionary();
    }

    public static ItemDataMap Instance {
      get { 
        return _sInstance; 
      }
    }

    [SerializeField]
    private ItemData[] _ItemMap;

    private Dictionary<string, ItemData> _IdToData;

    private void PopulateDictionary() {
    }

    public ItemData GetData(string itemId) {
      return null;
    }
  }
}