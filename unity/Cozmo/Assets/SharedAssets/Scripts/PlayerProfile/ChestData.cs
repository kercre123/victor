using UnityEngine;
using System.Collections;
using System;

public class ChestData : ScriptableObject {
  public LadderLevel[] GreenPointMaxLadder;
  public LadderLevel[] TreatRewardLadder;
  // TODO: Eventually replace this with Level, HexPeiceCount, HexValue when
  // we have multiple hex types.
  public LadderLevel[] HexRewardLadder;
}

[Serializable]
public class LadderLevel {
  public int Level;
  public int Value;
}
