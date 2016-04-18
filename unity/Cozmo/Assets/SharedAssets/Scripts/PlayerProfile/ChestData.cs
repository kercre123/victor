using UnityEngine;
using System.Collections;
using System;

public class ChestData : ScriptableObject {
  // INGO TODO delete this
  public LadderLevel[] GreenPointMaxLadder;
  public LadderLevel[] TreatRewardLadder;
  // TODO: Eventually replace this with Level, HexPeiceCount, HexValue when
  // we have multiple hex types.
  public LadderLevel[] HexRewardLadder;


  // INGO
  public Ladder RequirementLadder;
  public Ladder[] RewardLadders;
}

[Serializable]
public class Ladder {
  public string ItemId;
  public LadderLevel[] LadderLevels;
}

[Serializable]
public class LadderLevel {
  public int Level;
  public int Value;
}
