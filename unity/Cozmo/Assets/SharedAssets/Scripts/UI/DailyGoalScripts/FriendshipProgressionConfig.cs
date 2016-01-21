using UnityEngine;
using System.Collections;

public class FriendshipProgressionConfig : ScriptableObject {

  [System.Serializable] 
  public struct DailyGoalData {
	  public string FriendshipLevelName;
    public StatBitMask StatsIntroduced;
    public int MaxGoals;
    public int MinTarget;
    public int MaxTarget;
    public int PointsRequired;
  }

  [SerializeField]
  public DailyGoalData[] FriendshipLevels;
}
