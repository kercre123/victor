using UnityEngine;
using System.Collections;
using System;

public enum UnlockableType {
  Game,
  Action
}

[Serializable]
public class UnlockableInfo : ScriptableObject, IComparable {
  public SerializableUnlockIds Id;

  public UnlockableType UnlockableType;

  // used for serializing enums as strings instead of integers.
  public SerializableUnlockIds[] Prerequisites;

  [Tooltip("If true, any prereq filled will make this unlock available. If false, all prereqs must be filled to make this unlock available.")]
  public bool AnyPrereqUnlock;

  [Tooltip("Explicitly viewable and unlockable from the unlocks menu in Cozmo Tab.")]
  public bool ExplicitUnlock;

  [Tooltip("Will never leave the 'locked' state")]
  public bool NeverAvailable;

  public string DescriptionKey;

  public string TitleKey;

  [Cozmo.ItemId]
  public string UpgradeCostItemId;
  public int UpgradeCostAmountNeeded;

  [Cozmo.ItemId]
  public string RequestTrickCostItemId;
  public int RequestTrickCostAmountNeeded;

  [Cozmo.UI.CoreUpgradeTintName]
  public string CoreUpgradeTintColorName;

  public Sprite CoreUpgradeIcon;

  public int CubesRequired = 1;

  public int SortOrder = 100;

  [SerializeField]
  private float _TimeSparkedSec = 60.0f;
  public float TimeSparkedSec { get { return _TimeSparkedSec; } }

  [Serializable]
  public class SerializableUnlockIds : SerializableEnum<Anki.Cozmo.UnlockId> {

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

}
