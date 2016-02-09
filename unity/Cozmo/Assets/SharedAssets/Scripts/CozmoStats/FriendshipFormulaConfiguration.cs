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

  // Returns the % progression towards your daily goals
  // If isPercentProg is false, returns the flat amount of points earned
  public float CalculateDailyGoalProgress(StatContainer progress, StatContainer goal, bool isPercentProg = true) {
    float totalProgress = 0f, totalGoal = 0f;
    StatContainer overflow = new StatContainer();
    bool dailyGoalComplete = true;
    for (int i = 0; i < _Multipliers.Length; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;
      if (dailyGoalComplete) {
        dailyGoalComplete = (progress[stat] >= goal[stat]);
      }
      float cappedProg = (float)Mathf.Min(progress[stat], goal[stat]);
      totalProgress += cappedProg * _Multipliers[i];
      overflow[stat] = (int)(progress[stat] - cappedProg);
      totalGoal += goal[stat] * _Multipliers[i];
    }
    // If the conditions for all daily goals have been met, factor in overflow stats for Bonus Mult.
    // This currently means that if you make tons of extra progress for a stat, it will
    // cap any point gains from that stat at the goal until you have completed all daily goals.
    // Then it will factor in overflow points.
    float mult = 1.0f;
    if (dailyGoalComplete) {
      for (int i = 0; i < _Multipliers.Length; i++) {
        var stat = (Anki.Cozmo.ProgressionStatType)i;
        totalProgress += overflow[stat] * _Multipliers[i];
      }
      mult = Mathf.Ceil(totalProgress / totalGoal);
    }
    if (totalGoal > 0f) {
      if (isPercentProg) {
        return totalProgress / totalGoal;
      }
      else {
        totalProgress *= mult;
        return totalProgress;
      }
    }
    else {
      return 0f;
    }
  }

}
