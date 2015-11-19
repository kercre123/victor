using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable] 
public class ChallengeRequirements : ISerializationCallbackReceiver {
  // list of challenge ids that are required to be unlocked
  public string[] ChallengeGateKeys;

  // list of stat requirements required to be unlocked
  public Dictionary<Anki.Cozmo.ProgressionStatType, uint> StatGateKeys = new Dictionary<Anki.Cozmo.ProgressionStatType, uint>();


  [System.Serializable] 
  public class StatGate {
    public Anki.Cozmo.ProgressionStatType StatType;
    public uint StatValue;
  }


  // this exists because unity doesn't know how to serialize dictionaries.
  [SerializeField]
  private StatGate[] StatGateArray;

  public void OnBeforeSerialize() {

  }

  public void OnAfterDeserialize() {
    StatGateKeys = new Dictionary<Anki.Cozmo.ProgressionStatType, uint>();
    for (int i = 0; i < StatGateArray.Length; ++i) {
      StatGateKeys.Add(StatGateArray[i].StatType, StatGateArray[i].StatValue);
    }
  }

  public bool MeetsRequirements(Robot robot, List<string> unlockedChallenges) {
    for (int i = 0; i < ChallengeGateKeys.Length; ++i) {
      if (unlockedChallenges.Contains(ChallengeGateKeys[i]) == false) {
        return false;
      }
    }

    for (Anki.Cozmo.ProgressionStatType i = 0; i < Anki.Cozmo.ProgressionStatType.Count; ++i) {
      uint statValue = 0;
      if (StatGateKeys.TryGetValue(i, out statValue)) {
        if (robot.ProgressionStats[(int)i] < statValue) {
          return false;
        }
      }
    }
    return true;
  }
}
