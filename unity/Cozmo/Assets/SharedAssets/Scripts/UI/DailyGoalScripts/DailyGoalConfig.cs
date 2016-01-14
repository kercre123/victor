using UnityEngine;
using System.Collections;

public class DailyGoalConfig : ScriptableObject {

  [System.Serializable] 
  public struct DailyGoalData {
    public Anki.Cozmo.ProgressionStatType[] StatsIncluded;
    public string FriendshipLevelName;
    public int MaxGoals;
  }

  [SerializeField]
  [Range(1,10)]
  public int MinDailyGoalTarget;
  [SerializeField]
  [Range(1,10)]
  public int MaxDailyGoalTarget;

  [SerializeField]
  public DailyGoalData[] FriendshipLevels;
}
