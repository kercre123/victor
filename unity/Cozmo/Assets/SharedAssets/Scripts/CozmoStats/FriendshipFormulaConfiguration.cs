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

  public float CalculateFriendshipScore(StatContainer stats) {
    float total = 0f;
    for (int i = 0; i < _Multipliers.Length; i++) {
      total += _Multipliers[i] * stats[(Anki.Cozmo.ProgressionStatType)i];
    }
    return total;
  }

  public float CalculateFriendshipProgress(StatContainer progress, StatContainer goal) {
    float totalProgress = 0f, totalGoal = 0f;
    for (int i = 0; i < _Multipliers.Length; i++) {
      var stat = (Anki.Cozmo.ProgressionStatType)i;

      int p = Mathf.Min(progress[stat], goal[stat]);

      totalProgress += p * _Multipliers[i];
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
