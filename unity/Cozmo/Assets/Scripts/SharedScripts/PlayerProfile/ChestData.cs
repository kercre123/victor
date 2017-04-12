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

  public ChestRequirementData Requirement;
  public ChestRewardData[] RewardList;
}


[Serializable]
public class ChestRequirementData {
  [Cozmo.ItemId]
  public string ItemId;

  public int TargetPoints;
}

[Serializable]
public class ChestRewardData {
  [Cozmo.ItemId]
  public string ItemId;

  public int MinAmount;
  public int MaxAmount;
}
