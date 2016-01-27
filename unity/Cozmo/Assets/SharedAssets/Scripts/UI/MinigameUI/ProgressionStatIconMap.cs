using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {
    public class ProgressionStatIconMap : ScriptableObject {

      private static ProgressionStatIconMap _Instance;

      public static ProgressionStatIconMap Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<ProgressionStatIconMap>("Prefabs/UI/ProgressionStatIconMap");
            _Instance.TranslateStatsToDict();
          }
          return _Instance;
        }
      }

      [SerializeField]
      private Sprite _DefaultStatIcon;

      [SerializeField]
      private IconToStat[] _StatToIconList;

      private Dictionary<ProgressionStatType, Sprite> _StatToIconMap;

      private void TranslateStatsToDict() {
        _StatToIconMap = new Dictionary<ProgressionStatType, Sprite>();
        foreach (var icon in _StatToIconList) {
          if (!_StatToIconMap.ContainsKey(icon.StatType)) {
            _StatToIconMap.Add(icon.StatType, icon.StatIcon);
          }
          else {
            DAS.Error(this, string.Format("Tried to add a duplicate sprite for stat: {0}! ProgressionStatIconMap.asset!", icon.StatType));
          }
        }
      }

      public Sprite GetIconForStat(ProgressionStatType statType) {
        Sprite statIcon = _DefaultStatIcon;
        if (!_StatToIconMap.TryGetValue(statType, out statIcon)) {
          DAS.Error(this, string.Format("No sprite exists for stat: {0}! Check the ProgressionStatIconMap.asset!", statType));
        }
        return statIcon;
      }
    }

    [System.Serializable]
    public class IconToStat {
      public ProgressionStatType StatType;
      public Sprite StatIcon;
    }
  }
}