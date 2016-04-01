using UnityEngine;
using System.Collections;
using System;

public enum UnlockableType {
  Game,
  Action
}

[Serializable]
public class UnlockableInfo : ScriptableObject {
  public Anki.Cozmo.UnlockIds Id;

  public UnlockableType UnlockableType;

  public Anki.Cozmo.UnlockIds[] Prerequisites;

  // true = any prereq filled will make this unlock available
  // false = all prereqs must be filled to make this unlock available.
  public bool AnyPrereqUnlock;

  public int CubesRequired;

  // explicitly viewable and unlockable from the unlocks menu in Cozmo Tab.
  public bool ExplicitUnlock;

  // unlocked by default if the robot profile is new.
  public bool DefaultUnlock;

  public int TreatCost;
}
