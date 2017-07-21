using Cozmo.Challenge;
using UnityEngine;
using System;

public enum UnlockableType {
  Game,
  Action
}

[Serializable]
public class UnlockableInfo : ScriptableObject, IComparable {
  public SerializableUnlockIds Id;

  public UnlockableType UnlockableType;

  [SerializeField, Tooltip("Will never leave the 'locked' state and shows a 'Coming Soon' graphic")]
  private bool _ComingSoon;
  public bool ComingSoon { get { return _ComingSoon; } }

  public bool FeatureIsEnabled {
    get {
      if (FeatureGate.Instance.FeatureMap.ContainsKey(Id.Value.ToString().ToLower())) {
        return FeatureGate.Instance.IsFeatureEnabled(Id.Value.ToString().ToLower());
      }
      return true;
    }
  }

  public string DescriptionKey;

  public string TitleKey;

  public string SparkButtonDescription;

  public string SparkedStateDescription;

  public SparkedMusicStateWrapper SparkedMusicState;

  [Cozmo.ItemId]
  public string RequestTrickCostItemId;

  [SerializeField]
  private SerializableSparkableThings _SparkableThingID;

  public Anki.Cozmo.SparkableThings SparkableThingID {
    get { return _SparkableThingID.Value; }
  }

  public int RequestTrickCostAmount {
    get {
      return (int)Anki.Cozmo.EnumConcept.GetSparkCosts(_SparkableThingID.Value, 0);
    }
  }

  public Sprite CoreUpgradeIcon;

  public int CubesRequired = 1;

  public int SortOrder = 12345;

  [Serializable]
  public class SerializableUnlockIds : SerializableEnum<Anki.Cozmo.UnlockId> {

  }

  [Serializable]
  public class SerializableSparkableThings : SerializableEnum<Anki.Cozmo.SparkableThings> {

  }

  public int CompareTo(object obj) {
    if (obj != null) {
      UnlockableInfo otherEntry = obj as UnlockableInfo;
      if (otherEntry != null) {
        // if we're set equal, then alphabetize
        if (SortOrder == otherEntry.SortOrder) {
          return Localization.Get(TitleKey).CompareTo(Localization.Get(otherEntry.TitleKey));
        }
        // Lower is closer to front.
        return SortOrder - otherEntry.SortOrder;
      }
    }
    return 1;
  }

  public string AndroidReleaseVersion;
  public int AndroidSDKVersion;

  [SerializeField]
  private bool _CanVoiceActivate = false;
  public bool CanVoiceActivate { get { return _CanVoiceActivate; } }

  public string DASName {
    get { return Id.Value.ToString(); }
  }
}
