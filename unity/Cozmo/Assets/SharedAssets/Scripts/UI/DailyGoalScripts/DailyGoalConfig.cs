using UnityEngine;
using System.Collections;

public class DailyGoalConfig : ScriptableObject {

  [System.Serializable] 
  public struct DailyGoalData {
    public StatBitMask StatsIntroduced;
    public string FriendshipLevelName;
    public int MaxGoals;
    public int MinTarget;
    public int MaxTarget;
  }

  [SerializeField]
  public DailyGoalData[] FriendshipLevels;
}
