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

  // true = any prereq filled will make this unlock available
  // false = all prereqs must be filled to make this unlock available.
  public bool AnyPrereqUnlock;

  // explicitly viewable and unlockable from the unlocks menu in Cozmo Tab.
  public bool ExplicitUnlock;

  // unlocked by default if the robot profile is new.
  public bool DefaultUnlock;

  public string DescriptionKey;

  public string TitleKey;

  // TODO: Add validation (does it exist in ItemDataConfig or HexItemList
  public string CostItemId;
  public int CostAmountNeeded;

  [Serializable]
  public class SerializableUnlockIds : SerializableEnum<Anki.Cozmo.UnlockId> {

  }

}
