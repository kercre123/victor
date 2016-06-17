using UnityEngine;
using System.Collections;
using System;

public enum UnlockableType {
  Game,
  Action
}

[Serializable]
public class UnlockableInfo : ScriptableObject {
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

  [Serializable]
  public class SerializableUnlockIds : SerializableEnum<Anki.Cozmo.UnlockId> {

  }

}
