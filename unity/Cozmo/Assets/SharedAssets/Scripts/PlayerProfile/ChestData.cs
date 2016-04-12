using UnityEngine;
using System.Collections;
using System;

public class ChestData : ScriptableObject {
  public Ladder[] GreenPointMaxLadders;
  public Ladder[] TreatRewardLadders;
}

[Serializable]
public class Ladder {
  public int Level;
  public int Value;
}
