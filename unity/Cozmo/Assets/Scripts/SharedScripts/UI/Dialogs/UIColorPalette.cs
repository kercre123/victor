using UnityEngine;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    public class CoreUpgradeTintNameAttribute : PropertyAttribute {
      public CoreUpgradeTintNameAttribute() {
      }
    }

    [System.Serializable]
    public class CoreUpgradeTint {
      [SerializeField]
      private string _TintName;

      public string TintName {
        get { return _TintName; }
      }

      [SerializeField]
      private Color _TintColor;

      public Color TintColor {
        get { return _TintColor; }
      }
    }

    public class UIColorPalette : ScriptableObject {

      private static UIColorPalette _sInstance;

      public static void SetInstance(UIColorPalette instance) {
        _sInstance = instance;
        _sInstance.PopulateDictionary();
      }

      public static UIColorPalette Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private Color _GeneralBackgroundColor;

      public static Color GeneralBackgroundColor {
        get { return Instance._GeneralBackgroundColor; }
      }

      [SerializeField]
      private Color _CompleteTextColor;

      public static Color CompleteTextColor {
        get { return Instance._CompleteTextColor; }
      }

      [SerializeField]
      private Color _EnergyTextColor;

      public static Color EnergyTextColor {
        get { return Instance._EnergyTextColor; }
      }

      [SerializeField]
      private Color _NeutralTextColor;

      public static Color NeutralTextColor {
        get { return Instance._NeutralTextColor; }
      }

      [SerializeField]
      private Color _SetupTextColor;

      public static Color SetupTextColor {
        get { return Instance._SetupTextColor; }
      }

      [SerializeField]
      private Color _MinigameTextColor;

      public static Color MinigameTextColor {
        get { return Instance._MinigameTextColor; }
      }

      [SerializeField]
      private Color _GameSetupColor;

      public static Color GameSetupColor {
        get { return Instance._GameSetupColor; }
      }

      [SerializeField]
      private Color _GameBackgroundColor;

      public static Color GameBackgroundColor {
        get { return Instance._GameBackgroundColor; }
      }

      [SerializeField]
      private Color _ActivityBackgroundColor;

      public static Color ActivityBackgroundColor {
        get { return Instance._ActivityBackgroundColor; }
      }

      [SerializeField]
      private Color _FreeplayBehaviorDefaultColor;

      public static Color FreeplayBehaviorDefaultColor {
        get { return Instance._FreeplayBehaviorDefaultColor; }
      }

      [SerializeField]
      private Color _FreeplayBehaviorRewardColor;

      public static Color FreeplayBehaviorRewardColor {
        get { return Instance._FreeplayBehaviorRewardColor; }
      }

      [SerializeField]
      private CoreUpgradeTint[] _CoreUpgradeTints;

      public static CoreUpgradeTint[] CoreUpgradeTints {
        get { return Instance._CoreUpgradeTints; }
      }

      private Dictionary<string, CoreUpgradeTint> _NameToCoreUpgradeTint;

      private void PopulateDictionary() {
        _NameToCoreUpgradeTint = new Dictionary<string, CoreUpgradeTint>();
        foreach (CoreUpgradeTint data in _CoreUpgradeTints) {
          if (!_NameToCoreUpgradeTint.ContainsKey(data.TintName)) {
            _NameToCoreUpgradeTint.Add(data.TintName, data);
          }
          else {
            DAS.Error("UIColorPalette.PopulateDictionary", "Trying to add item to dictionary, but the item already exists! item=" + data.TintName);
          }
        }
      }

      public static CoreUpgradeTint GetUpgradeTintData(string coreUpgradeTintName) {
        CoreUpgradeTint data = null;
        if (!_sInstance._NameToCoreUpgradeTint.TryGetValue(coreUpgradeTintName, out data)) {
          DAS.Error("UIColorPalette.GetUpgradeTintData", "Could not find tintName='" + coreUpgradeTintName + "' in dictionary!");
        }
        return data;
      }

#if UNITY_EDITOR
      public IEnumerable<string> EditorGetItemIds() {
        List<string> itemIds = new List<string>();
        foreach (var data in _CoreUpgradeTints) {
          itemIds.Add(data.TintName);
        }
        return itemIds;
      }
#endif
    }
  }
}