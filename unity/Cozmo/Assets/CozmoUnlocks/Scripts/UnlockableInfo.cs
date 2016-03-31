using UnityEngine;
using System.Collections;

public enum UnlockableType {
  Game,
  Action
}

public class UnlockableInfo : ScriptableObject {
  public readonly uint Id;

  public readonly UnlockableType UnlockableType;

  public readonly uint[] Prerequisites;

  // true = any prereq filled will make this unlock available
  // false = all prereqs must be filled to make this unlock available.
  public readonly bool AnyPrereqUnlock;

  public readonly int CubesRequired;

  public readonly int TreatCost;
}
