using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable] 
public class ChallengeRequirements : ISerializationCallbackReceiver {
  // list of challenge ids that are required to be unlocked
  public string[] LevelLocks;

  // list of stat requirements required to be unlocked
  public Dictionary<Anki.Cozmo.ProgressionStatType, uint> StatLocks = new Dictionary<Anki.Cozmo.ProgressionStatType, uint>();

  // this exists because unity doesn't know how to serialize dictionaries.
  public Anki.Cozmo.ProgressionStatType[] StatKeys;
  public uint[] StatValues;

  public void OnBeforeSerialize() {

  }

  public void OnAfterDeserialize() {
    StatLocks = new Dictionary<Anki.Cozmo.ProgressionStatType, uint>();
    for (int i = 0; i != System.Math.Min(StatKeys.Length, StatValues.Length); ++i) {
      StatLocks.Add(StatKeys[i], StatValues[i]);
    }
  }

  public bool TryGetStatLock(Anki.Cozmo.ProgressionStatType type, out uint value) {
    if (StatLocks.TryGetValue(type, out value)) {
      return true;
    }
    return false;
  }

  public bool MeetsRequirements(Robot robot, List<string> unlockedLevels) {
    for (int i = 0; i < LevelLocks.Length; ++i) {
      if (unlockedLevels.Contains(LevelLocks[i]) == false) {
        return false;
      }
    }

    for (Anki.Cozmo.ProgressionStatType i = 0; i < Anki.Cozmo.ProgressionStatType.Count; ++i) {
      uint statValue = 0;
      if (TryGetStatLock(i, out statValue)) {
        if (robot.ProgressionStats[(int)i] < statValue) {
          return false;
        }
      }
    }
    return true;
  }
}
