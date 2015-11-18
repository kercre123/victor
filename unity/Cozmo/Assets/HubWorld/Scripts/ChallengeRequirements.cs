using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class ChallengeRequirements {
  // list of challenge ids that are required to be unlocked
  public List<string> LevelLocks = new List<string>();

  // list of stat requirements required to be unlocked
  public Dictionary<string, uint> StatLocks = new Dictionary<string, uint>();

  public bool TryGetStatLock(Anki.Cozmo.ProgressionStatType type, out uint value) {
    if (StatLocks.TryGetValue(type.ToString(), out value)) {
      return true;
    }
    return false;
  }

  public Anki.Cozmo.ProgressionStatType GetProgressionType(string key) {
    return (Anki.Cozmo.ProgressionStatType)System.Enum.Parse(typeof(Anki.Cozmo.ProgressionStatType), key);
  }

  public bool MeetsRequirements(Robot robot, List<string> unlockedLevels) {
    for (int i = 0; i < LevelLocks.Count; ++i) {
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
