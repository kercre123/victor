using UnityEngine;
using System.Collections;
using System.Collections.Generic;

[System.Serializable] 
public class ChallengeRequirements : ISerializationCallbackReceiver {
  // list of challenge ids that are required to be unlocked
  public string[] ChallengeGateKeys;

  public void OnBeforeSerialize() {
  }

  public void OnAfterDeserialize() {
  }

  public bool MeetsRequirements(IRobot robot, List<string> completedChallengeIds) {
    for (int i = 0; i < ChallengeGateKeys.Length; ++i) {
      if (completedChallengeIds.Contains(ChallengeGateKeys[i]) == false) {
        return false;
      }
    }
    return true;
  }
}
