using UnityEngine;
using System.Collections;
using System;

public class ChestData : ScriptableObject {
  private static ChestData _sInstance;

  public static void SetInstance(ChestData instance) {
    _sInstance = instance;
  }

  public static ChestData Instance {
    get { return _sInstance; }
  }

  public Ladder RequirementLadder;
  public Ladder[] RewardLadders;
}

[Serializable]
public class Ladder {
  // TODO: Add validation (does it exist in ItemDataConfig or HexItemList
  public string ItemId;
  public LadderLevel[] LadderLevels;
}

[Serializable]
public class LadderLevel {
  public int Level;
  public int Value;
}
