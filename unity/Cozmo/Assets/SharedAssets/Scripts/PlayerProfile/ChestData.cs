using UnityEngine;
using System.Collections;
using System;

public class ChestData : ScriptableObject {
  public Ladder[] GreenPointMaxLadders;
  public Ladder[] TreatRewardLadders;
  // TODO: Eventually replace this with Level, HexPeiceCount, HexValue when
  // we have multiple hex types.
  public Ladder[] HexRewardLadders;
}

[Serializable]
public class Ladder {
  public int Level;
  public int Value;
}
