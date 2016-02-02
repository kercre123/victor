using UnityEngine;
using System.Collections;

public class FriendshipFormulaConfiguration : ScriptableObject {

  [SerializeField]
  [HideInInspector]
  private float[] _Multipliers = new float[(int)Anki.Cozmo.ProgressionStatType.Count];

  public float[] Multipliers { 
    get { 
      if (_Multipliers.Length < (int)Anki.Cozmo.ProgressionStatType.Count) {
        var old = _Multipliers;
        _Multipliers = new float[(int)Anki.Cozmo.ProgressionStatType.Count];
        old.CopyTo(_Multipliers, 0);
      }
      return _Multipliers; 
    } 
  }

  // Returns the friendship score value earned, including the multiplier from exceeding the goal
  public float CalculateFriendshipScore(StatContainer stats, StatContainer goal) {
    float statPointsEarned = 0f;
    float statPointsNeeded = 0f;
    for (int i = 0; i < _Multipliers.Length; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;
      statPointsEarned += _Multipliers[i] * stats[stat];
      statPointsNeeded += goal[stat] * _Multipliers[i];
    }
    float mult = Mathf.Ceil(statPointsEarned / statPointsNeeded);
    statPointsEarned *= mult;
    return statPointsEarned;
  }

  // Returns the % progression towards your daily goals
  public float CalculateDailyGoalProgress(StatContainer progress, StatContainer goal) {
    float totalProgress = 0f, totalGoal = 0f;
    for (int i = 0; i < _Multipliers.Length; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;

      totalProgress += progress[stat] * _Multipliers[i];
      totalGoal += goal[stat] * _Multipliers[i];
    }
    if (totalGoal > 0f) {
      return totalProgress / totalGoal;
    }
    else {
      return 0f;
    }
  }

}
