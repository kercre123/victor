using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    public class ProgressionStatConfig : ScriptableObject {

      private static ProgressionStatConfig _Instance;

      public static ProgressionStatConfig Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<ProgressionStatConfig>("Prefabs/UI/ProgressionStatConfig");
            _Instance.TranslateStatsToDict();
          }
          return _Instance;
        }
      }

      [SerializeField]
      private Sprite _DefaultStatIcon;

      [SerializeField]
      private string _DefaultLocKey;

      [SerializeField]
      private StatMap[] _StatMapList;

      private Dictionary<ProgressionStatType, Sprite> _StatToIconMap;
      private Dictionary<ProgressionStatType, string> _StatToLocKeyMap;

      private void TranslateStatsToDict() {
        _StatToIconMap = new Dictionary<ProgressionStatType, Sprite>();
        _StatToLocKeyMap = new Dictionary<ProgressionStatType, string>();
        foreach (var statKey in _StatMapList) {
          if (!_StatToIconMap.ContainsKey(statKey.StatType)) {
            _StatToIconMap.Add(statKey.StatType, statKey.StatIcon);
          }
          else {
            DAS.Error(this, string.Format("Tried to add a duplicate sprite for stat: {0}! ProgressionStatConfig.asset!", statKey.StatType));
          }
          if (!_StatToLocKeyMap.ContainsKey(statKey.StatType)) {
            _StatToLocKeyMap.Add(statKey.StatType, statKey.StatLocKey);
          }
          else {
            DAS.Error(this, string.Format("Tried to add a duplicate sprite for stat: {0}! ProgressionStatConfig.asset!", statKey.StatType));
          }
        }
      }

      public string GetLocNameForStat(ProgressionStatType statType) {
        string statKey = _DefaultLocKey;
        if (!_StatToLocKeyMap.TryGetValue(statType, out statKey)) {
          DAS.Error(this, string.Format("No locKey exists for stat: {0}! Check the ProgressionStatConfig.asset!", statType));
        }
        statKey = Localization.Get(statKey);
        return statKey;
      }

      public Sprite GetIconForStat(ProgressionStatType statType) {
        Sprite statIcon = _DefaultStatIcon;
        if (!_StatToIconMap.TryGetValue(statType, out statIcon)) {
          DAS.Error(this, string.Format("No sprite exists for stat: {0}! Check the ProgressionStatConfig.asset!", statType));
        }
        return statIcon;
      }
    }

    [System.Serializable]
    public class StatMap {
      public ProgressionStatType StatType;
      public Sprite StatIcon;
      public string StatLocKey;
    }
  }
}