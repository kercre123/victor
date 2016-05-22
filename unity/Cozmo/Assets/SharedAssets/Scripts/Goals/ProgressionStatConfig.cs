using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    public class ProgressionStatConfig : ScriptableObject {

      private static ProgressionStatConfig _sInstance;

      public static void SetInstance(ProgressionStatConfig instance) {
        _sInstance = instance;
      }

      public static ProgressionStatConfig Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private Sprite _DefaultStatIcon;

      [SerializeField]
      private string _DefaultLocKey;

      [SerializeField]
      private StatMap[] _StatMapList;

      private Dictionary<ProgressionStatType, Sprite> _StatToIconMap;
      private Dictionary<ProgressionStatType, string> _StatToLocKeyMap;

      public void Initialize() {
        TranslateStatsToDict();
      }

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
        string statKey;
        if (!_StatToLocKeyMap.TryGetValue(statType, out statKey)) {
          statKey = _DefaultLocKey;
          DAS.Error(this, string.Format("No locKey exists for stat: {0}! Check the ProgressionStatConfig.asset!", statType));
        }
        statKey = Localization.Get(statKey);
        return statKey;
      }

      public Sprite GetIconForStat(ProgressionStatType statType) {
        Sprite statIcon;
        if (!_StatToIconMap.TryGetValue(statType, out statIcon)) {
          statIcon = _DefaultStatIcon;
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