using UnityEngine;
using System.Collections;

public class FriendshipProgressionConfig : ScriptableObject {

  // Contains all the Data for a single friendship level
  [System.Serializable] 
  public struct FriendshipLevelData {
    // Name of the Friendship Level
    public string FriendshipLevelName;
    // How many points are Required to get to this Friendship level
    public int PointsRequired;
    // BitMask for new ProgressionStat types that are
    // introduced at this level, additive with previous levels.
    // Currently no way to remove Stats from the pool.
    public StatBitMask StatsIntroduced;
    // Maximum number of Goals that can generated, assuming
    // at least one will.
    public int MaxGoals;
    // Minimum Target value of ProgressionStat to earn
    public int MinTarget;
    // Maximum Target value of ProgressionStat to earn
    public int MaxTarget;
  }

  [System.Serializable]
  public struct BonusMultData {
    public Sprite Background;
    public Sprite Fill;
    public Sprite Complete;
  }

  [SerializeField]
  public FriendshipLevelData[] FriendshipLevels;
  [SerializeField]
  public BonusMultData[] BonusMults;
}
