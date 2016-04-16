using UnityEngine;
using System.Collections;

// Singleton for games to interface with to get current
// skill level values.
// This system calculates when thresholds should be calculated and changed.
public class SkillSystem : UnityEngine.Object {

  private static SkillSystem _sInstance;

  public static void SetInstance(SkillSystem instance) {
    _sInstance = instance;
  }

  public static SkillSystem Instance {
    get { return _sInstance; }
  }

  /*StartGame(ChallengeData) // starts listening for when to update
  EndGame(ChallengeData) // stops listening
  GetSkillSet(ChallengeData) // gets whole map for curr level
  GetSkillVal(ChallengeData,skillname) // returns specific thing for level
  GetSkillLevel(ChallengeData)*/
}