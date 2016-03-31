using UnityEngine;
using System.Collections;

public enum UnlockableType {
  Game,
  Action
}

public class UnlockableInfo {
  public readonly uint _Id;

  public readonly UnlockableType _UnlockableType;

  public readonly uint[] _Prerequisites;

  // true = any prereq filled will make this unlock available
  // false = all prereqs must be filled to make this unlock available.
  public readonly bool _AnyPrereqUnlock;

  public readonly int _CubesRequired;

  public readonly int _TreatCost;
}
